#pragma once

#include <utility>

template<bool Const>
struct const_member_function
{
    static constexpr bool value = Const;
    template<typename T>
    const_member_function(T&&) {}
};

template<typename T, typename Ret, bool Nx, typename ...Args>
const_member_function(Ret(T::*)(Args...) const noexcept(Nx)) -> const_member_function<true>;

template<typename T>
const_member_function(T&&) -> const_member_function<false>;

template<typename T>
static constexpr bool is_const_member_function_v = decltype(const_member_function(std::declval<T>()))::value;
