#pragma once
#include "kaddress.h"
#include "kuserdata.h"


namespace knet
{
    class worker;
    struct sockbuf;

    class socket final : noncopyable
    {
    public:
        struct writebuf
        {
            const void* data;
            size_t size;

            writebuf() noexcept : data(nullptr), size(0) {}
            writebuf(const void* data_, size_t size_) noexcept
                : data(data_), size(size_)
            {}
        };

        class listener
        {
        public:
            virtual ~listener() = default;

            virtual void on_conn(socket& sock) = 0;
            virtual void on_close(socket& sock) = 0;
            virtual size_t on_data(socket& sock, char* data, size_t size) = 0;
            virtual void on_timer(socket& sock, const userdata& ud, int64_t absms) {}
        };

    public:
        socket(worker* wkr, rawsocket_t rawsock, socketid_t socketid, 
            listener* listener) noexcept;
        ~socket();

        int64_t set_abs_timer(int64_t absms, const userdata& ud);
        int64_t set_rel_timer(int64_t relms, const userdata& ud)
        { return set_abs_timer(now_ms() + relms, ud); }
        void del_timer(int64_t absms);

        void close() noexcept;

        bool write(const writebuf* buf, size_t num) noexcept;
        void on_pollevent(const pollevent_t& pollevent) noexcept;

        bool get_sockaddr(address& addr) const noexcept;
        bool get_peeraddr(address& addr) const noexcept;

        socketid_t get_socketid() const noexcept { return _socketid; }
        listener* get_listener() const noexcept { return _listener; }
        const userdata& get_userdata() const noexcept { return _userdata; }
        void set_userdata(const userdata& ud) noexcept { _userdata = ud; }

    private:
        friend class worker;
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
        listener* const _listener = nullptr;

        userdata _userdata;
        uint8_t _flag = 0;
        sockbuf* _rbuf = nullptr;
        sockbuf* _wbuf = nullptr;
    };
}
