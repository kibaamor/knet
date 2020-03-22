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

struct buffer {
    const void* data;
    size_t size;

    buffer(const void* d = nullptr, size_t s = 0)
        : data(d)
        , size(s)
    {
    }
};

} // namespace knet

#ifdef DEBUG
#define KNET_DEBUG
#endif