#ifndef TATOMIC_H
#define TATOMIC_H

#include <atomic>
#include <TGlobal>


template<typename T>
class TAtomic : public std::atomic<T>
{
public:
    TAtomic() noexcept = default;
    ~TAtomic() noexcept = default;
    TAtomic(T item) noexcept : std::atomic<T>(item) { }

    operator T() const noexcept { return load(); }
    T operator=(T item) noexcept { store(item); return item; }
    T load() const noexcept { return std::atomic<T>::load(std::memory_order_acquire); }
    void store(T item) noexcept { std::atomic<T>::store(item, std::memory_order_release); }
    T exchange(T item) noexcept { return std::atomic<T>::exchange(item, std::memory_order_acq_rel); }

    T fetchAdd(T item) noexcept { return std::atomic<T>::fetch_add(item, std::memory_order_acq_rel); }
    T fetchSub(T item) noexcept { return std::atomic<T>::fetch_sub(item, std::memory_order_acq_rel); }
    T operator++() noexcept { return fetchAdd(1) + 1; } // Prefix increment operator.
    T operator++(int) noexcept { return fetchAdd(1); }  // Postfix increment operator.
    T operator--() noexcept { return fetchSub(1) - 1; } // Prefix decrement operator.
    T operator--(int) noexcept { return fetchSub(1); }  // Postfix decrement operator.

    bool compareExchange(T &expected, T newValue) noexcept
    {
        return std::atomic<T>::compare_exchange_weak(expected, newValue, std::memory_order_acq_rel);
    }

    bool compareExchangeStrong(T &expected, T newValue) noexcept
    {
        return std::atomic<T>::compare_exchange_strong(expected, newValue, std::memory_order_acq_rel);
    }

    T_DISABLE_COPY(TAtomic)
    T_DISABLE_MOVE(TAtomic)
};

#endif // TATOMIC_H
