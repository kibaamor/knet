#include "kconn.h"
#include "ksocket.h"

namespace knet {
bool conn::send_data(buffer* buf, size_t num)
{
    return nullptr != _socket && _socket->write(buf, num);
}

void conn::disconnect()
{
    if (!is_disconnecting())
        _socket->close();
}

bool conn::is_disconnecting() const
{
    return nullptr == _socket || _socket->is_closing();
}
} // namespace knet