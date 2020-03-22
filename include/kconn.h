#pragma once
#include "knetfwd.h"
#include "kuserdata.h"

namespace knet {

using connid_t = uint32_t;
using timerid_t = int64_t;
constexpr timerid_t INVALID_TIMERID = 0;

class conn_factory;
class socket;
class conn {
public:
    explicit conn(conn_factory& cf);
    virtual ~conn();

    virtual void on_connected(socket* s);
    virtual void on_disconnect();
    virtual size_t on_recv_data(char* data, size_t size) = 0;

    virtual void on_timer(int64_t absms, const userdata& ud) = 0;
    timerid_t add_timer(int64_t absms, const userdata& ud);
    void del_timer(timerid_t tid);

    bool send_data(buffer* buf, size_t num);
    void disconnect();
    bool is_disconnecting() const;

    connid_t get_connid() const { return _id; }

protected:
    bool set_sockbuf_size(size_t size);

private:
    conn_factory& _cf;
    const connid_t _id;
    socket* _s = nullptr;
};

} // namespace knet
