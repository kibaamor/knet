#pragma once
#include "../kplatform.h"

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

    bool can_save_data(const buffer* buf, size_t num) const
    {
        size_t total_size = 0;
        for (size_t i = 0; i < num; ++i) {
            auto b = buf + i;
            kassert(b->size && b->data);
            total_size += b->size;
        }
        return total_size < unused_size();
    }

    void save_data(const buffer* buf, size_t num)
    {
        for (size_t i = 0; i < num; ++i) {
            auto b = buf + i;
            memcpy(unused_ptr(), b->data, b->size);
            used_size += b->size;
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
        size_t size = 0;
        int ret = 0;
        while (size < used_size) {
#ifdef SO_NOSIGPIPE
            ret = TEMP_FAILURE_RETRY(write(rs, chunk + size, used_size - size));
#else // !SO_NOSIGPIPE
            ret = TEMP_FAILURE_RETRY(send(rs, chunk + size, used_size - size, MSG_NOSIGNAL));
#endif // SO_NOSIGPIPE
            if (RAWSOCKET_ERROR == ret) {
                if (EAGAIN == errno || EWOULDBLOCK == errno) {
                    break;
                }
                return false;
            }
            if (0 == ret) {
                return false;
            }
            size += ret;
        }

        if (size > 0) {
            discard_used(size);
        }

        return true;
    }

    bool try_read(rawsocket_t rs)
    {
        int ret = 0;
        while (unused_size() > 0) {
            ret = TEMP_FAILURE_RETRY(read(rs, unused_ptr(), unused_size()));
            if (RAWSOCKET_ERROR == ret) {
                if (EAGAIN == errno || EWOULDBLOCK == errno) {
                    break;
                }
                return false;
            }
            if (0 == ret) {
                return false;
            }
            used_size += ret;
        }
        return true;
    }

#endif // _WIN32
};

} // namespace knet
