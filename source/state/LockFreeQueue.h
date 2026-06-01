#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace arch
{
/**
    Single-producer / single-consumer bounded lock-free queue.

    Used for one-directional, real-time-safe message passing:
      - UI  -> audio          (transport commands)
      - audio -> background    (analysis work hints)
      - background -> audio    (voice spawn commands)

    Capacity must be a power of two. T must be trivially copyable; we never
    construct/destruct elements on push/pop, so no allocation ever occurs.
*/
template <typename T, std::size_t Capacity>
class LockFreeQueue
{
public:
    static_assert ((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert (std::is_trivially_copyable_v<T>, "T must be trivially copyable");

    bool push (const T& item) noexcept
    {
        const auto head = head_.load (std::memory_order_relaxed);
        const auto next = (head + 1) & mask;
        if (next == tail_.load (std::memory_order_acquire))
            return false;                       // full — drop, never block
        buffer_[head] = item;
        head_.store (next, std::memory_order_release);
        return true;
    }

    bool pop (T& out) noexcept
    {
        const auto tail = tail_.load (std::memory_order_relaxed);
        if (tail == head_.load (std::memory_order_acquire))
            return false;                       // empty
        out = buffer_[tail];
        tail_.store ((tail + 1) & mask, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return head_.load (std::memory_order_acquire) == tail_.load (std::memory_order_acquire);
    }

private:
    static constexpr std::size_t mask = Capacity - 1;
    std::array<T, Capacity> buffer_ {};
    std::atomic<std::size_t> head_ { 0 };       // producer writes
    std::atomic<std::size_t> tail_ { 0 };       // consumer writes
};
} // namespace arch
