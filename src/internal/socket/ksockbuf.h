#pragma once
#include "../kplatform.h"
#include "../ksocket_utils.h"

namespace knet {

struct sockbuf {
#ifdef _WIN32
    WSAOVERLAPPED ol = {}; // must be the first member to use CONTAINING_RECORD
    bool cancel = false;
#endif // _WIN32
    size_t used_size = 0;
    char chunk[SOCKET_RWBUF_SIZE] = {};

    char* unused_ptr() { return chunk + used_size; }
    size_t unused_size() const { return sizeof(chunk) - used_size; }

    void discard_used(size_t num)
    {
        kassert(num > 0 && used_size >= num);
        used_size -= num;
        if (used_size > 0) {
            memmove(chunk, chunk + num, used_size);
        }
    }

    void save_data(void* data, size_t num)
    {
        memcpy(unused_ptr(), data, num);
        used_size += num;
    }

    void batch_save_data(const buffer* buf, size_t num)
    {
        for (size_t i = 0; i < num; ++i) {
            save_data(buf[i].data, buf[i].size);
        }
    }

#ifdef _WIN32
    bool post_write(rawsocket_t rs)
    {
        memset(&ol, 0, sizeof(ol));

        WSABUF buf;
        buf.buf = chunk;
        buf.len = static_cast<ULONG>(used_size);

        DWORD dw = 0;
        return SOCKET_ERROR != WSASend(rs, &buf, 1, &dw, 0, &ol, nullptr)
            || ERROR_IO_PENDING == WSAGetLastError();
    }

    bool post_read(rawsocket_t rs)
    {
        memset(&ol, 0, sizeof(ol));

        WSABUF buf;
        buf.buf = unused_ptr();
        buf.len = static_cast<ULONG>(unused_size());

        DWORD dw = 0;
        DWORD flag = 0;
        return SOCKET_ERROR != WSARecv(rs, &buf, 1, &dw, &flag, &ol, nullptr)
            || ERROR_IO_PENDING == WSAGetLastError();
    }

#else // !_WIN32

    bool try_write(rawsocket_t rs)
    {
        buffer buf(chunk, used_size);
        size_t used = 0;
        if (!rawsocket_sendv(rs, &buf, 1, used)) {
            return false;
        }
        if (used) {
            discard_used(used);
        }
        return true;
    }

    bool try_read(rawsocket_t rs)
    {
        size_t used = 0;
        if (!rawsocket_recv(rs, unused_ptr(), unused_size(), used)) {
            return false;
        }
        used_size += used;
        return true;
    }

#endif // _WIN32
};

} // namespace knet
