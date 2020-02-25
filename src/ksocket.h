#pragma once
#include "kconnection.h"


namespace knet
{
    class worker;
    class poller;
    struct sockbuf;

    class socket final : noncopyable
    {
    public:
        socket(worker* worker, rawsocket_t rawsock) noexcept;
        ~socket();

        bool attach_poller(poller& poller);
        void on_pollevent(const pollevent_t& pollevent) noexcept;

        void close() noexcept;

        bool write(buffer* buf, size_t num) noexcept;

        socketid_t get_socketid() const noexcept { return _socketid; }

    private:
        bool start() noexcept;

#ifdef KNET_USE_IOCP
        bool try_read() noexcept;
        void handle_write(size_t wrote) noexcept;
#endif
        bool handle_read() noexcept;
        bool try_write() noexcept;

    private:
        worker* const _worker = nullptr;
        rawsocket_t _rawsocket = INVALID_RAWSOCKET;

        socketid_t _socketid = INVALID_SOCKETID;
        connection* _conn = nullptr;

        uint8_t _flag = 0;
        sockbuf* _rbuf = nullptr;
        sockbuf* _wbuf = nullptr;
    };
}
