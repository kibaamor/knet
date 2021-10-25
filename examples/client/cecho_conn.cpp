#include "cecho_conn.h"
#include <knet/kutils.h>
#include <cassert>
#include <algorithm>

void cecho_conn::do_on_connected()
{
    echo_conn::do_on_connected();

    _bufs[0].data = reinterpret_cast<char*>(&_hdr);
    _bufs[0].size = sizeof(msg_hdr);

    send_msg(0);
}

void cecho_conn::do_on_timer(int64_t ms, const userdata& ud)
{
    echo_conn::do_on_timer(ms, ud);
}

size_t cecho_conn::do_on_recv_data(char* data, size_t size)
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

    const size_t new_size = hdr->size > sizeof(uint32_t) ? hdr->size - sizeof(uint32_t) : 0;
    if (!send_msg(new_size)) {
        std::cerr << "send msg failed!\n";
        disconnect();
        return 0;
    }

    return static_cast<size_t>(sizeof(msg_hdr) + hdr->size);
}

bool cecho_conn::send_msg(size_t size)
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
