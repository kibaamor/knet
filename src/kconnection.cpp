#include "kconnection.h"
#include "ksocket.h"

namespace knet {
bool connection::send_data(buffer* buf, size_t num)
{
    return nullptr != _socket && _socket->write(buf, num);
}

void connection::disconnect()
{
    if (!is_disconnecting())
        _socket->close();
}

bool connection::is_disconnecting() const
{
    return nullptr == _socket || _socket->is_closing();
}
} // namespace knet