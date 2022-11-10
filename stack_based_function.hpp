
#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <functional>

#include "args_helper.hpp"
#include "const_member_function_helper.hpp"
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

    template <typename Functor> requires (not std::is_member_function_pointer_v<Functor>)
    function(Functor&& fctr) :
        executer{
            [](std::byte* ctx, Args... args) {
                Functor* l = reinterpret_cast<Functor*>(ctx);
                return (*l)(args...);
            }
        },
        const_executer{
            []{
                if constexpr (is_const_member_function_v<decltype(&Functor::operator())>)
                {
                    return [](const std::byte* ctx, Args... args) {
                        const Functor* l = reinterpret_cast<const Functor*>(ctx);
                        return (*l)(args...);
                    };
                }
                else
                {
                    return [](const std::byte*) { throw std::bad_function_call(); };
                }
            }()
        },
        deleter{[](const std::byte* ctx) {
            const Functor* l = reinterpret_cast<const Functor*>(ctx);
            l->~Functor();
        }},
        func_type_info(typeid(Functor))
    {
        new (ctx_storage.data()) Functor{std::forward<Functor>(fctr)};
    }

    function(Ret(*ptr)(Args...)) :
        executer{[](std::byte* ctx, Args ...args){
            Func* f = *reinterpret_cast<Func**>(ctx);
            return f(args...);
        }},
        const_executer{[](const std::byte* ctx, Args ...args){
            Func* f = *reinterpret_cast<Func**>(const_cast<std::byte*>(ctx));
            return f(args...);
        }},
        deleter{[](const std::byte*) {}},
        func_type_info{typeid(decltype(ptr))}
    {
        new (ctx_storage.data()) Func*{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function(Ret_(T::*ptr)(Args_...)) :
        executer{[](std::byte* ctx, T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) = *reinterpret_cast<Ret_(T::**)(Args_...)>(ctx);
            return (obj.*f)(args...);
        }},
        const_executer{[](const std::byte*, T&) { throw std::bad_function_call(); }},
        deleter{[](const std::byte*) {}},
        func_type_info{typeid(decltype(ptr))}
    {
        using MemberPtr = Ret_(T::*)(Args_...);
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function(Ret_(T::*ptr)(Args_...) const) :
        executer{[](std::byte* ctx, const T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) const = *reinterpret_cast<Ret_(T::**)(Args_...) const>(ctx);
            return (obj.*f)(args...);
        }},
        const_executer{[](const std::byte* ctx, const T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) const = *reinterpret_cast<Ret_(T::**)(Args_...) const>(const_cast<std::byte*>(ctx));
            return (obj.*f)(args...);
        }},
        deleter{[](const std::byte*) {}},
        func_type_info{typeid(decltype(ptr))}
    {
        using MemberPtr = Ret_(T::*)(Args_...) const;
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    template <typename Functor>
    function& operator=(Functor&& fctr) {
        static_assert(sizeof(Functor) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Functor) <= Align, "reassigned callable alignment should be less or equal than initial");
        static_assert(std::is_same_v<Ret(Args...), Signature<decltype(&Functor::operator())>>, "reassigned callable signature should be the same as initial");

        deleter(ctx_storage.data());

        executer = [](std::byte* ctx, Args... args) {
            Functor* l = reinterpret_cast<Functor*>(ctx);
            return (*l)(args...);
        };

        if constexpr (is_const_member_function_v<decltype(&Functor::operator())>)
        {
            const_executer = [](const std::byte* ctx, Args... args) {
                const Functor* l = reinterpret_cast<const Functor*>(ctx);
                return (*l)(args...);
            };
        }
        else
        {
            const_executer = [](const std::byte*) { throw std::bad_function_call(); };
        }

        deleter = [](const std::byte* ctx) {
            const Functor* l = reinterpret_cast<const Functor*>(ctx);
            l->~Functor();
        };
        func_type_info = typeid(Functor);

        new (ctx_storage.data()) Functor{std::forward<Functor>(fctr)};
        return *this;
    }

    function& operator=(Ret(*ptr)(Args...))
    {
        static_assert(sizeof(Ret(*)(Args...)) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Ret(*)(Args...)) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");

        deleter(ctx_storage.data());

        executer = [](std::byte* ctx, Args ...args){
            Func* f = *reinterpret_cast<Func**>(ctx);
            return f(args...);
        };
        const_executer = [](const std::byte* ctx, Args ...args){
            Func* f = *reinterpret_cast<Func**>(const_cast<std::byte*>(ctx));
            return f(args...);
        };
        deleter = [](const std::byte*) {};
        func_type_info = typeid(decltype(ptr));

        new (ctx_storage.data()) Func*{ptr};
        return *this;
    }

    template<typename T, typename Ret_, typename ...Args_>
    function& operator=(Ret_(T::*ptr)(Args_...))
    {
        static_assert(sizeof(Ret_(T::*)(Args_...)) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Ret_(T::*)(Args_...)) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");
        static_assert(std::is_same_v<Ret(Args...), Ret_(T&, Args_...)>, "reassigned callable signature should be the same as initial");

        deleter(ctx_storage.data());

        executer = [](std::byte* ctx, T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) = *reinterpret_cast<Ret_(T::**)(Args_...)>(ctx);
            return obj.*f(args...);
        };
        const_executer = [](const std::byte*, T&) { throw std::bad_function_call(); };
        deleter = [](const std::byte*) {};
        func_type_info = typeid(decltype(ptr));

        using MemberPtr = Ret_(T::*)(Args...);
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function& operator=(Ret_(T::*ptr)(Args_...) const)
    {
        static_assert(sizeof(Ret_(T::*)(Args_...) const) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Ret_(T::*)(Args_...) const) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");
        static_assert(std::is_same_v<Ret(Args...), Ret_(const T&, Args_...)>, "reassigned callable signature should be the same as initial");

        deleter(ctx_storage.data());

        executer = [](std::byte* ctx, const T& obj, Args_ ...args){
            Func* f = *reinterpret_cast<Ret_(T::**)(Args_...) const>(ctx);
            return obj.*f(args...);
        };
        const_executer = [](const std::byte* ctx, const T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) const = *reinterpret_cast<Ret_(T::**)(Args_...) const>(const_cast<std::byte*>(ctx));
            return (obj.*f)(args...);
        };
        deleter = [](const std::byte*) {};
        func_type_info = typeid(decltype(ptr));

        using MemberPtr = Ret_(T::*)(Args_...) const;
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    function() {
        executer = [](std::byte*){ throw std::bad_function_call{}; };
        executer = [](const std::byte*){ throw std::bad_function_call{}; };
        deleter = [](const std::byte*){};
    }

    function(std::nullptr_t) {
        executer = [](std::byte*){ throw std::bad_function_call{}; };
        executer = [](const std::byte*){ throw std::bad_function_call{}; };
        deleter = [](const std::byte*){};
    }

    function(const function&) = default;
    function(function&&) noexcept = default;
    function& operator=(const function&) = delete;
    function& operator=(function&&) = delete;

    Ret operator()(Args... args)
    {
        return executer(ctx_storage.data(), args...);
    }

    Ret operator()(Args... args) const
    {
        return const_executer(ctx_storage.data(), args...);
    }

    operator bool() const noexcept
    {
        return static_cast<bool>(executer) && static_cast<bool>(const_executer);
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return *this;
    }

    const std::type_info& target_type() const noexcept
    {
        return func_type_info;
    }

private:
    constexpr static inline std::size_t context_size = CtxSize;
    constexpr static inline std::size_t context_alignment = Align;

    alignas(Align) std::array<std::byte, CtxSize> ctx_storage;  // lambda capture
    Ret(*executer)(std::byte*, Args...);
    Ret(*const_executer)(const std::byte*, Args...);
    void(*deleter)(const std::byte*);
    std::reference_wrapper<const std::type_info> func_type_info;
};

struct S_;

static constexpr std::size_t min_size_ = std::max(sizeof(void (S_::*)()), sizeof(void*));
static constexpr std::size_t min_align_ = alignof(void (S_::*)());

template <typename Functor>
function(Functor&&) ->
    function<std::max(sizeof(Functor), min_size_), std::max(alignof(Functor), min_align_), Signature<decltype(&Functor::operator())>>;

template <typename Ret, typename ...Args>
function(Ret(*)(Args...)) ->
    function<std::max(sizeof(Ret(*)(Args...)), min_size_) , std::max(alignof(Ret(*)(Args...)), min_align_), Ret(Args...)>;

template <typename T, typename Ret_, typename ...Args_>
function(Ret_(T::*)(Args_...)) ->
    function<std::max(sizeof(Ret_(T::*)(Args_...)), min_size_), std::max(alignof(Ret_(T::*)(Args_...)), min_align_), Ret_(T&, Args_...)>;

template <typename T, typename Ret_, typename ...Args_>
function(Ret_(T::*)(Args_...) const) ->
    function<std::max(sizeof(Ret_(T::*)(Args_...) const), min_size_), std::max(alignof(Ret_(T::*)(Args_...) const), min_align_), Ret_(const T&, Args_...)>;
