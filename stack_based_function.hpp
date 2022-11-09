
#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <functional>

#include "args_helper.hpp"
#include "signature_helper.hpp"

template<size_t CtxSize, size_t Align, typename Signature_>
class function;

/* std::function replacement with stack storage */
template <size_t CtxSize, size_t Align, typename Ret, typename ...Args>
class function<CtxSize, Align, Ret(Args...)> {
    using Func = Ret(Args...);

public:
    using result_type = Ret;
    template<std::size_t N>
    using nth_argument = nth_argument<N, Args...>;

    template <typename Lambda>
    function(Lambda&& lmb) :
        executer{[](void* ctx, Args... args) {
            Lambda* l = static_cast<Lambda*>(ctx);
            return (*l)(args...);
        }},
        deleter{[](void* ctx) {
            Lambda* l = static_cast<Lambda*>(ctx);
            l->~Lambda();
        }},
        func_type_info(typeid(Lambda))
    {
        new (ctx_storage.data()) Lambda{std::forward<Lambda>(lmb)};
    }

    function(Ret(*ptr)(Args...)) :
        executer{[](void* ctx, Args ...args){
            Func* f = *static_cast<Func**>(ctx);
            return f(args...);
        }},
        deleter{[](void*) {}},
        func_type_info{typeid(decltype(ptr))}
    {
        new (ctx_storage.data()) Func*{ptr};
    }

    template <typename Lambda>
    function& operator=(Lambda&& lmb) {
        static_assert(sizeof(Lambda) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Lambda) <= Align, "reassigned callable alignment should be less or equal than initial");
        static_assert(std::is_same_v<Ret(Args...), Signature<decltype(&Lambda::operator())>>, "reassigned callable signature should be the same as initial");

        deleter(ctx_storage.data());

        executer = [](void* ctx, Args... args) {
            Lambda* l = static_cast<Lambda*>(ctx);
            return (*l)(args...);
        };
        deleter = [](void* ctx) {
            Lambda* l = static_cast<Lambda*>(ctx);
            l->~Lambda();
        };
        func_type_info = typeid(Lambda);

        new (ctx_storage.data()) Lambda{std::forward<Lambda>(lmb)};
        return *this;
    }

    function& operator=(Ret(*ptr)(Args...))
    {
        static_assert(sizeof(void*) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(Align == alignof(void*), "initial callable alingment should be alignment of a pointer to store this callable");

        deleter(ctx_storage.data());

        executer = [](void* ctx, Args ...args){
            Func* f = *static_cast<Func**>(ctx);
            return f(args...);
        };
        deleter = [](void*) {};
        func_type_info = typeid(decltype(ptr));

        new (ctx_storage.data()) Func*{ptr};
        return *this;
    }

    function() {
        executer = [](void*){ throw std::bad_function_call{}; };
        deleter = [](void*){};
    }

    function(std::nullptr_t) {
        executer = [](void*){ throw std::bad_function_call{}; };
        deleter = [](void*){};
    }

    function(const function&) = default;
    function(function&&) noexcept = default;
    function& operator=(const function&) = delete;
    function& operator=(function&&) = delete;

    Ret operator()(Args... args)
    {
        return executer(ctx_storage.data(), args...);
    }

    operator bool() const noexcept
    {
        return static_cast<bool>(executer);
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return static_cast<bool>(executer);
    }

    const std::type_info& target_type() const noexcept
    {
        return func_type_info;
    }

private:
    constexpr static inline std::size_t context_size = CtxSize;
    constexpr static inline std::size_t context_alignment = Align;

    alignas(Align) std::array<std::byte, CtxSize> ctx_storage;  // lambda capture
    Ret(*executer)(void*, Args...);
    void(*deleter)(void*);
    std::reference_wrapper<const std::type_info> func_type_info;
};

template <typename Lambda>
function(Lambda&& lmb) ->
    function<sizeof(Lambda), std::max(alignof(Lambda), alignof(void*)), Signature<decltype(&Lambda::operator())>>;

template <typename Ret, typename ...Args>
function(Ret(*ptr)(Args...)) ->
    function<sizeof(void*), alignof(void*), Ret(Args...)>;
