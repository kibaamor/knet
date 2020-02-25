#pragma once
#include "kconnection.h"


namespace knet
{
    class connection_factory;
    class poller;
    struct sockbuf;

    class socket final : noncopyable
    {
    public:
        socket(connection_factory* cf, rawsocket_t rs) noexcept;
        ~socket();

        bool attach_poller(poller& poller);

        void close() noexcept;

        bool write(buffer* buf, size_t num) noexcept;

        void on_rawpollevent(const rawpollevent_t& evt) noexcept;

    private:
        bool start() noexcept;

#ifdef KNET_USE_IOCP
        bool try_read() noexcept;
        void handle_write(size_t wrote) noexcept;
#endif
        bool handle_read() noexcept;
        bool try_write() noexcept;

    private:
        connection_factory* const _cf;
        rawsocket_t _rs;

        connection* _conn = nullptr;

        uint8_t _flag = 0;
        sockbuf* _rbuf = nullptr;
        sockbuf* _wbuf = nullptr;
    };
}
