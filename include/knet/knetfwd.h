#pragma once
#include <cstdint>
#include <string>
#include <memory>

#if defined(_WIN32)
#if defined(EXPORTING_KNET)
#define KNET_API __declspec(dllexport)
#else
#define KNET_API __declspec(dllimport)
#endif
#else // non windows
#define KNET_API
#endif

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
