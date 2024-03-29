#pragma once
#include "../echo_conn.h"
#include <knet/kconn_factory.h>
#include <knet/kutils.h>
#include <cassert>
#include <algorithm>

class cecho_conn : public echo_conn {
public:
    using echo_conn::echo_conn;

protected:
    void do_on_connected() override
    {
        echo_conn::do_on_connected();

        _bufs[0].data = reinterpret_cast<char*>(&_hdr);
        _bufs[0].size = sizeof(msg_hdr);

        send_msg(0);
    }

    size_t do_on_recv_data(char* data, size_t size) override
    {
        echo_conn::do_on_recv_data(data, size);

        const auto hdr = reinterpret_cast<msg_hdr*>(data);
        if (size < sizeof(msg_hdr) || size < sizeof(msg_hdr) + hdr->size) {
            return 0;
        }

        const uint32_t token = *reinterpret_cast<uint32_t*>(data + sizeof(msg_hdr) + hdr->size - sizeof(uint32_t));
        if (token != _token) {
            std::cerr << "invalid msg token:" << token << ", expected: " << _token << "\n";
            disconnect();
            return 0;
        }
        ++mgr.total_recv;

        const size_t new_size = hdr->size > sizeof(uint32_t) ? hdr->size - sizeof(uint32_t) : 0;
        if (!send_msg(new_size)) {
            std::cerr << "send msg failed!\n";
            disconnect();
            return 0;
        }

        return static_cast<size_t>(sizeof(msg_hdr) + hdr->size);
    }

    bool send_msg(size_t size)
    {
        static thread_local char buf[SOCKET_RWBUF_SIZE - sizeof(msg_hdr)];

        if (size < sizeof(uint32_t) || size > sizeof(buf)) {
            size = u32rand_between(sizeof(uint32_t), sizeof(buf));
        }

        _hdr.size = static_cast<uint32_t>(size);
        *reinterpret_cast<uint32_t*>(buf + size - sizeof(uint32_t)) = _token = u32rand();

        _bufs[1].data = buf;
        _bufs[1].size = size;

        return send_data(_bufs, 2);
    }

private:
    uint32_t _token;
    msg_hdr _hdr;
    buffer _bufs[2];
};
