#pragma once
#include "kinternal.h"

namespace knet {

class flag {
public:
    void mark_close()
    {
        kassert(!is_close());
        _f |= kClose;
    }
    void mark_call()
    {
        kassert(!is_call());
        _f |= kCall;
    }
    void mark_read()
    {
        kassert(!is_read());
        _f |= kRead;
    }
    void mark_write()
    {
        kassert(!is_write());
        _f |= kWrite;
    }

    void unmark_close()
    {
        kassert(is_close());
        _f &= ~kClose;
    }
    void unmark_call()
    {
        kassert(is_call());
        _f &= ~kCall;
    }
    void unmark_read()
    {
        kassert(is_read());
        _f &= ~kRead;
    }
    void unmark_write()
    {
        kassert(is_write());
        _f &= ~kWrite;
    }

    bool is_close() const
    {
        return 0 != (_f & kClose);
    }
    bool is_call() const
    {
        return 0 != (_f & kCall);
    }
    bool is_read() const
    {
        return 0 != (_f & kRead);
    }
    bool is_write() const
    {
        return 0 != (_f & kWrite);
    }

private:
    static constexpr uint8_t kClose = 1u << 0u;
    static constexpr uint8_t kCall = 1u << 1u;
    static constexpr uint8_t kRead = 1u << 2u;
    static constexpr uint8_t kWrite = 1u << 3u;

    uint8_t _f = 0;
};

class scoped_call_flag {
public:
    scoped_call_flag(flag& f)
        : _f(f)
    {
        _f.mark_call();
    }

    ~scoped_call_flag()
    {
        _f.unmark_call();
    }

private:
    flag& _f;
};

} // namespace knet