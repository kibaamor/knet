#pragma once
#include <cstdint>
#include <type_traits>


namespace knet
{
    struct userdata
    {
        enum
        {
            unknown,
            pointer,
            floatpoint,
            integral,
        } type = unknown;
        union
        {
            void* ptr;
            double f64;
            int64_t i64 = 0;
        } data;

        userdata() = default;
        userdata(void* v) : type(pointer)
        {
            data.ptr = v;
        }
        template <typename T, typename std::enable_if<
            std::is_floating_point<T>::value, int>::type = 0>
            userdata(T v) : type(floatpoint)
        {
            data.f64 = static_cast<double>(v);
        }
        template <typename T, typename std::enable_if<
            std::is_integral<T>::value, int>::type = 0>
            userdata(T v) : type(integral)
        {
            data.i64 = static_cast<int64_t>(v);
        }

        template <typename T>
        T* as_ptr() const
        {
            return pointer == type ? static_cast<T*>(data.ptr) : nullptr;
        }
    };
}
