#pragma once
#include <atomic>

namespace knet {

template <typename T, unsigned int N>
class spsc_queue {
public:
    spsc_queue() = default;
    spsc_queue(const spsc_queue&) = delete;
    spsc_queue& operator=(const spsc_queue&) = delete;
    ~spsc_queue() = default;

    bool is_lock_free() const
    {
        return _rpos.is_lock_free() && _wpos.is_lock_free();
    }

    bool is_empty() const
    {
        return _rpos.load(std::memory_order_acquire)
            == _wpos.load(std::memory_order_acquire);
    }

    bool is_full() const
    {
        return _rpos.load(std::memory_order_acquire)
            == (_wpos.load(std::memory_order_acquire) + 1) % N;
    }

    bool push(const T& val)
    {
        const auto cur_wpos = _wpos.load(std::memory_order_acquire);
        const auto next_wpos = (cur_wpos + 1) % N;
        if (next_wpos == _rpos.load(std::memory_order_acquire)) {
            return false;
        }
        _ring[cur_wpos] = val;
        _wpos.store(next_wpos, std::memory_order_release);
        return true;
    }

    bool pop(T& val)
    {
        const auto rpos = _rpos.load(std::memory_order_acquire);
        if (rpos == _wpos.load(std::memory_order_acquire)) {
            return false;
        }
        val = _ring[rpos];
        _rpos.store((rpos + 1) % N, std::memory_order_release);
        return true;
    }

private:
    using pos_t = std::atomic<unsigned int>;
    pos_t _rpos = { 0 };
    pos_t _wpos = { 0 };
    T _ring[N] = {};
};

} // namespace knet
