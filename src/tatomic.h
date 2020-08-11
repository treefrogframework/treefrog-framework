#pragma once
#include <TGlobal>
#include <atomic>


template <typename T>
class TAtomic : public std::atomic<T> {
public:
    TAtomic() = default;
    ~TAtomic() = default;
    TAtomic(T item) :
        std::atomic<T>(item) { }

    operator T() const { return load(); }
    T operator=(T item)
    {
        store(item);
        return item;
    }
    T load() const { return std::atomic<T>::load(std::memory_order_acquire); }
    void store(T item) { std::atomic<T>::store(item, std::memory_order_release); }
    T exchange(T item) { return std::atomic<T>::exchange(item, std::memory_order_acq_rel); }

    T fetchAdd(T item) { return std::atomic<T>::fetch_add(item, std::memory_order_acq_rel); }
    T fetchSub(T item) { return std::atomic<T>::fetch_sub(item, std::memory_order_acq_rel); }
    T operator++() { return fetchAdd(1) + 1; }  // Prefix increment operator.
    T operator++(int) { return fetchAdd(1); }  // Postfix increment operator.
    T operator--() { return fetchSub(1) - 1; }  // Prefix decrement operator.
    T operator--(int) { return fetchSub(1); }  // Postfix decrement operator.

    bool compareExchange(T &expected, T newValue)
    {
        return std::atomic<T>::compare_exchange_weak(expected, newValue, std::memory_order_acq_rel);
    }

    bool compareExchangeStrong(T &expected, T newValue)
    {
        return std::atomic<T>::compare_exchange_strong(expected, newValue, std::memory_order_acq_rel);
    }

    T_DISABLE_COPY(TAtomic)
    T_DISABLE_MOVE(TAtomic)
};

