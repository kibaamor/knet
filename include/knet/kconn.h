#pragma once
#include "knetfwd.h"
#include "kuserdata.h"
#include "kaddress.h"

namespace knet {

using connid_t = uint32_t;
using timerid_t = int64_t;
constexpr timerid_t INVALID_TIMERID = 0;

class conn_factory;
class socket;

class conn {
public:
    // successful call statistics
    struct stat {
        uint32_t send_count = 0; // send_data
        uint32_t write_count = 0; // writev
        uint32_t copy_count = 0; // memcpy for send
        uint32_t move_count = 0; // memmove for send

        uint32_t recv_count = 0; // on_recv_data
        uint32_t read_count = 0; // read
    };

    explicit conn(conn_factory& cf);
    virtual ~conn();

    bool send_data(const buffer* buf, size_t num);

    void on_connected(socket* s);
    void on_disconnect();
    size_t on_recv_data(char* data, size_t size);

    void disconnect();
    bool is_disconnecting() const;

    timerid_t add_timer(int64_t ms, const userdata& ud);
    void del_timer(timerid_t tid);
    void on_timer(int64_t ms, const userdata& ud);

    bool get_stat(stat& s) const;
    bool get_sockaddr(address& addr) const;
    bool get_peeraddr(address& addr) const;

    connid_t get_connid() const { return _id; }

protected:
    bool set_sockbuf_size(size_t size);

    virtual void do_on_connected() { }
    virtual void do_on_disconnect() { }
    virtual void do_on_timer(int64_t ms, const userdata& ud) { }
    virtual size_t do_on_recv_data(char* data, size_t size) = 0;

private:
    conn_factory& _cf;
    const connid_t _id;
    socket* _s = nullptr;
};

} // namespace knet
