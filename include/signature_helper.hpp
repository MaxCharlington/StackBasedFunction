
#pragma once

#include <type_traits>

template <typename>
struct function_guide_helper {};

template <typename Ret_, typename Functor_, bool Nx_, typename... Args_>
struct function_guide_helper<Ret_(Functor_::*)(Args_...) noexcept(Nx_)> {
    using type = Ret_(Args_...);
};

template <typename Ret_, typename Functor_, bool Nx_, typename... Args_>
struct function_guide_helper<Ret_(Functor_::*)(Args_...) & noexcept(Nx_)> {
    using type = Ret_(Args_...);
};

template <typename Ret_, typename Functor_, bool Nx_, typename... Args_>
struct function_guide_helper<Ret_(Functor_::*)(Args_...) const noexcept(Nx_)> {
    using type = Ret_(Args_...);
};

template <typename Ret_, typename Functor_, bool Nx_, typename... Args_>
struct function_guide_helper<Ret_(Functor_::*)(Args_...) const & noexcept(Nx_)> {
    using type = Ret_(Args_...);
};

template<typename T> requires requires{std::is_member_function_pointer_v<T>;}
using Signature = typename function_guide_helper<T>::type;
