#pragma once

#include <cstdint>

template<std::size_t I, typename Arg, typename ...Args>
consteval auto args_helper()
{
    if constexpr (I == 0)
        return Arg{};
    else
        return args_helper<I-1, Args...>();
}

template<std::size_t N, typename ...Args>
using nth_argument = decltype(args_helper<N, Args...>());
