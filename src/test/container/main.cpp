#include <TfTest/TfTest>
#include "tstack.h"
#include "tlockstack.h"
#include "tlockqueue.h"
#include <atomic>
#include <random>
#include <deque>
#include <vector>
#include <thread>
#include <cassert>
#include <print>


template <typename T>
class FCQueue {
public:
    enum class State : uint8_t { Empty, Pending, Applied };
    enum class Op : uint8_t { Push, Pop };

    struct Request {
        std::atomic<FCQueue::State> state {FCQueue::State::Empty};
        Op op {FCQueue::Op::Push};
        std::optional<T> value;
    };

    explicit FCQueue() // : reqs_{128}
    {
        // while (reqs_.size() < 128) {
        //     reqs_.push_back(new Request{});
        // }
    }

    ~FCQueue()
    {
        // for (auto p : reqs_) {
        //     delete p;
        // }
    }

    void push(T v)
    {
        if (try_push(v)) {
            return;
        }

        size_t id = acquireSlot();
        auto& r = reqs_[id];

        // State s = r.state.load(std::memory_order_acquire);
        // if (s != State::Empty) {
        //     throw std::runtime_error("FCQueue: push logic error");
        // }

        r.value = std::move(v);
        r.op = Op::Push;
        r.state.store(State::Pending, std::memory_order_release);
        combineCounter.fetch_add(1, std::memory_order_relaxed);

        while (r.state.load(std::memory_order_acquire) != State::Applied) {
            std::this_thread::yield();
            combine();
        }
        r.state.store(State::Empty, std::memory_order_release);
    }

    std::optional<T> pop()
    {
        std::optional<T> val;

        if (try_pop(val)) {
            return val;
        }

        size_t id = acquireSlot();
        auto& r = reqs_[id];

        r.op = Op::Pop;
        r.state.store(State::Pending, std::memory_order_release);
        combineCounter.fetch_add(1, std::memory_order_relaxed);

        while (r.state.load(std::memory_order_acquire) != State::Applied) {
            std::this_thread::yield();
            combine();
        }

        val = std::move(r.value);
        r.state.store(State::Empty, std::memory_order_release);
        return val;
    }

private:
    bool try_push(T v)
    {
        if (!combineLock_.try_lock()) {
            return false;
        }

        q_.push(std::move(v));
        combineLock_.unlock();
        return true;
    }

    bool try_pop(std::optional<T> &v)
    {
        if (!combineLock_.try_lock()) {
            return false;
        }

        if (q_.empty()) {
            v = std::nullopt;
        } else {
            v = std::move(q_.front());
            q_.pop();
        }

        combineLock_.unlock();
        return true;
    }

    size_t acquireSlot()
    {
        static thread_local size_t mySlot = SIZE_MAX;

        if (mySlot != SIZE_MAX) [[likely]] {
            return mySlot;
        }

        mySlot = slotCounter_.load(std::memory_order_acquire) + 1;
        if (mySlot >= reqs_.size()) {
            ensure_slot(mySlot);
        }
        mySlot = slotCounter_.fetch_add(1, std::memory_order_relaxed);
        //std::print("mySlot: {}\n", mySlot);
        return mySlot;
    }

    void ensure_slot(size_t idx)
    {
        throw std::runtime_error("FCQueue: ensure_slot error");
        // std::lock_guard<std::mutex> lock(combineLock_);

        // size_t extend = reqs_.size();
        // while (extend <= idx) extend *= 2;
        // while (reqs_.size() < extend) {
        //     reqs_.push_back(new Request{});
        // }

    }

    void combine()
    {
        if (!combineLock_.try_lock()) {
            return;
        }

        static size_t idx = 0;
        size_t combcnt = combineCounter.exchange(0, std::memory_order_acq_rel);
        size_t cnt = 0;
        size_t maxslot = slotCounter_.load(std::memory_order_acquire);

        while (cnt < combcnt) [[likely]] {
            if (++idx >= maxslot) [[unlikely]] {
                idx = 0;
                maxslot = slotCounter_.load(std::memory_order_relaxed);
            }

            auto &r = reqs_[idx];
            State s = r.state.load(std::memory_order_acquire);

            if (s != State::Pending) [[unlikely]] {
                continue;   // Empty or Applied は combiner が触らない
            }

            switch (r.op) {
            case Op::Push:
                q_.push(std::move(r.value.value()));
                break;

            case Op::Pop:
                if (q_.empty()) {
                    r.value = std::nullopt;
                } else {
                    r.value = std::move(q_.front());
                    q_.pop();
                }
                break;

            default:
                break;
            }
            r.state.store(State::Applied, std::memory_order_release);
            cnt++;
        }

        combineLock_.unlock();
    }

private:
    std::mutex combineLock_;
    std::atomic<size_t> combineCounter {0};
    //std::deque<Request*> reqs_;
    std::array<Request, 128> reqs_;
    //std::vector<Request> reqs_;
    std::queue<T> q_;
    std::atomic<size_t> slotCounter_ {0};
};


static const int THREADS = std::max((int)(std::thread::hardware_concurrency() * 0.8), 1);
static constexpr int OPS_PER_THREAD = 200000;
static const int TOTAL = THREADS * OPS_PER_THREAD;


class TestQueue : public QObject {
    Q_OBJECT
private:
    template<class Qu, auto Enqueue, auto Dequeue>
    void correctnessTest()
    {
        for (int i = 0; i < 20; i++) {
            Qu q;

            // Duplicate check flag
            std::vector<std::atomic<bool>> seen(TOTAL);
            for (auto &v : seen) v.store(false);

            std::atomic<int> consumedCount{0};

            // -------- Producer --------
            auto producer = [&](int tid) {
                std::mt19937_64 rng(std::random_device{}());
                int base = tid * OPS_PER_THREAD;
                for (int i = 0; i < OPS_PER_THREAD; i++) {
                    (q.*Enqueue)(base + i);   // unique value
                    if ((rng() & 0xF) == 0) {
                        std::this_thread::yield();
                        QThread::usleep(10);
                    }
                    //std::cout << "tid:" << tid << " i:" << i << std::endl;
                }
            };

            // -------- Consumer --------
            auto consumer = [&]() {
                //int get_counter = 0;
                std::mt19937_64 rng(std::random_device{}());
                while (consumedCount.load() < TOTAL) {
                    //std::print("consumedCount remain:{}\n", TOTAL - consumedCount.load());

                    auto r = (q.*Dequeue)();
                    if (!r) {
                        continue;
                    } else {
                        //std::print("consumer get_counter:{} \n", ++get_counter);
                        if ((rng() & 0xF) == 0) {
                            std::this_thread::yield();
                            QThread::usleep(10);
                        }
                    }

                    int x = *r;

                    if (x < 0 || x >= TOTAL) {
                        qFatal("Out-of-range value detected: %d", x);
                    }

                    bool expected = false;
                    if (!seen[x].compare_exchange_strong(expected, true)) {
                        qFatal("Duplicate detected: %d", x);
                    }

                    consumedCount++;
                }
            };

            // ---- Launch threads ----
            std::vector<std::thread> producers, consumers;

            for (int i = 0; i < THREADS; i++) {
                producers.emplace_back(producer, i);
                consumers.emplace_back(consumer);
            }

            for (auto &t : producers) {
                t.join();
            }

            for (auto &t : consumers) {
                t.join();
            }

            // ---- Verify all flags true ----
            for (int i = 0; i < TOTAL; i++) {
                QVERIFY2(seen[i].load(), "Missing value detected");
            }

            qDebug("correctness test %d OK", i + 1);
        }
    }

    template<class Qu, auto Enqueue, auto Dequeue>
    void stressTest()
    {
        Qu q;

        std::atomic<bool> stop{false};
        std::atomic<int> counter{0};

        auto producer = [&]() {
            std::mt19937_64 rng(std::random_device{}());
            while (!stop.load(std::memory_order_acquire)) {
                int v = rng();
                (q.*Enqueue)(v);
                counter++;
                if ((rng() & 0xF) == 0) {
                    std::this_thread::yield();
                    QThread::usleep(10);
                }
            }
        };

        auto consumer = [&]() {
            std::mt19937_64 rng(std::random_device{}());
            while (!stop.load(std::memory_order_acquire)) {
                auto r = (q.*Dequeue)();
                if (r) counter++;

                if ((rng() & 0xF) == 0) {
                    std::this_thread::yield();
                    QThread::usleep(10);
                }
            }
        };

        std::vector<std::thread> threads;

        // 16 producer + 16 consumer
        for (int i = 0; i < THREADS; i++) {
            threads.emplace_back(producer);
            threads.emplace_back(consumer);
        }

        constexpr int msecs = 30000;
        QTest::qSleep(msecs); // 30 seconds
        stop.store(true);

        for (auto &t : threads) {
            t.join();
        }

        qDebug("ops %d  (%d ops/msec)", counter.load(), counter.load() / msecs);
        QVERIFY(counter.load() > msecs * 20);
    }

private slots:
    void correctnessTest_FCQueue()
    {
        correctnessTest<FCQueue<int64_t>, &FCQueue<int64_t>::push, &FCQueue<int64_t>::pop>();
    }

    // void correctnessTest_TLockQueue()
    // {
    //     correctnessTest<TLockQueue<int64_t>, &TLockQueue<int64_t>::push, &TLockQueue<int64_t>::pop>();
    // }

    // void correctnessTest_TLockStack()
    // {
    //     correctnessTest<TLockStack<int64_t>, &TLockStack<int64_t>::push, &TLockStack<int64_t>::pop>();
    // }

    void stressTest_FCQueue()
    {
        stressTest<FCQueue<int64_t>, &FCQueue<int64_t>::push, &FCQueue<int64_t>::pop>();
    }

    void stressTest_TLockQueue()
    {
        using PushFunc = void (TLockQueue<int64_t>::*)(const int64_t&);
        stressTest<TLockQueue<int64_t>, static_cast<PushFunc>(&TLockQueue<int64_t>::push), &TLockQueue<int64_t>::pop>();
    }

    void stressTest_TLockStack()
    {
        using PushFunc = void (TLockStack<int64_t>::*)(const int64_t&);
        stressTest<TLockStack<int64_t>, static_cast<PushFunc>(&TLockStack<int64_t>::push), &TLockStack<int64_t>::pop>();
    }
};


TF_TEST_SQLLESS_MAIN(TestQueue)
#include "main.moc"
