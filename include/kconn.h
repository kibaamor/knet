#pragma once
#include "knetfwd.h"

namespace knet {

struct buffer {
    const void* data;
    size_t size;

    buffer(const void* d = nullptr, size_t s = 0)
        : data(d)
        , size(s)
    {
    }
};

class socket;
class conn {
public:
    virtual ~conn() = default;

    virtual void on_connected() { }
    virtual size_t on_recv_data(char* data, size_t size) { return size; }
    virtual void on_disconnect() { }

    bool send_data(buffer* buf, size_t num);

    // connection may immediately deleted after disconnect() called
    void disconnect();
    bool is_disconnecting() const;

protected:
    virtual void on_attach_socket(rawsocket_t rs) { }

private:
    friend class socket;
    socket* _socket = nullptr;
};

class conn_factory {
public:
    virtual ~conn_factory() = default;

    virtual conn* create_conn() { return new conn(); }
    virtual void destroy_conn(conn* conn) { delete conn; }
};

class conn_factory_builder {
public:
    virtual ~conn_factory_builder() = default;

    virtual conn_factory* build_factory()
    {
        return new conn_factory();
    }
};

} // namespace knet
