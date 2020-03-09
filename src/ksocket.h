#pragma once
#include "kconnection.h"

namespace knet {
class connection_factory;
class poller;
struct sockbuf;

class socket final : noncopyable {
public:
    socket(connection_factory* cf, rawsocket_t rs);
    ~socket();

    bool attach_poller(poller* poller);

    bool write(buffer* buf, size_t num);

    void close();
    bool is_closing() const;

    bool on_rawpollevent(const rawpollevent_t& evt);

private:
    bool start();

#ifdef KNET_USE_IOCP
    bool try_read();
    void handle_write(size_t wrote);
#else
    bool handle_can_read();
    bool handle_can_write();
#endif
    bool handle_read();
    bool try_write();

private:
    connection_factory* const _cf;
    rawsocket_t _rs;

    connection* _conn = nullptr;

    uint8_t _flag = 0;
    sockbuf* _rbuf = nullptr;
    sockbuf* _wbuf = nullptr;
};
} // namespace knet
