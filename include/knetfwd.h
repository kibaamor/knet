#pragma once
#include <cstdint>
#include <string>
#include <memory>

namespace knet {

#ifdef _WIN32
using rawsocket_t = uintptr_t;
#else
using rawsocket_t = int;
#endif

} // namespace knet
