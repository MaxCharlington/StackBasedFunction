
#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <functional>

#include "args_helper.hpp"
#include "const_member_function_helper.hpp"
#include "nonconst_executor.hpp"
#include "signature_helper.hpp"

template<size_t CtxSize, size_t Align, bool Const, typename Signature_>
class function;

/* std::function replacement with stack storage */
template <size_t CtxSize, size_t Align, bool Const, typename Ret, typename ...Args>
class function<CtxSize, Align, Const, Ret(Args...)> : private std::conditional_t<(not Const), nonconst_executor_base<Ret, Args...>, dummy_executor_base>{
    using Func = Ret(Args...);

public:
    using result_type = Ret;
    template<std::size_t N>
    using nth_argument = nth_argument<N, Args...>;

    template <typename Functor> requires (not std::is_member_function_pointer_v<Functor>)
    function(Functor&& fctr) :
        const_executor{
            []{
                if constexpr (Const)
                {
                    return [](const std::byte* ctx, Args... args) {
                        const Functor* l = reinterpret_cast<const Functor*>(ctx);
                        return (*l)(args...);
                    };
                }
                else
                {
                    return [](const std::byte*, Args...) { throw std::bad_function_call(); };
                }
            }()
        },
        deleter{[](const std::byte* ctx) {
            const Functor* l = reinterpret_cast<const Functor*>(ctx);
            l->~Functor();
        }}
#ifdef std_function_compat
        , func_type_info(typeid(Functor))
#endif
    {
        if constexpr (not Const)
        {
            // The only condition where underlying context may be modified
            this->executor = [](std::byte* ctx, Args... args) {
                Functor* l = reinterpret_cast<Functor*>(ctx);
                return (*l)(args...);
            };
        }
        new (ctx_storage.data()) Functor{std::forward<Functor>(fctr)};
    }

    function(Ret(*ptr)(Args...)) :
        const_executor{[](const std::byte* ctx, Args ...args){
            Func* f = *reinterpret_cast<Func**>(const_cast<std::byte*>(ctx));
            return f(args...);
        }},
        deleter{trivial_deleter}
#ifdef std_function_compat
        , func_type_info{typeid(decltype(ptr))}
#endif
    {
        new (ctx_storage.data()) Func*{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function(Ret_(T::*ptr)(Args_...)) :
        const_executor{[](const std::byte* ctx, T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) = *reinterpret_cast<Ret_(T::**)(Args_...)>(const_cast<std::byte*>(ctx));
            return (obj.*f)(args...);
        }},
        deleter{trivial_deleter}
#ifdef std_function_compat
        , func_type_info{typeid(decltype(ptr))}
#endif
    {
        using MemberPtr = Ret_(T::*)(Args_...);
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function(Ret_(T::*ptr)(Args_...) const) :
        const_executor{[](const std::byte* ctx, const T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) const = *reinterpret_cast<Ret_(T::**)(Args_...) const>(const_cast<std::byte*>(ctx));
            return (obj.*f)(args...);
        }},
        deleter{trivial_deleter}
#ifdef std_function_compat
        , func_type_info{typeid(decltype(ptr))}
#endif
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

        if constexpr (is_const_member_function_v<decltype(&Functor::operator())> and Const)
        {
            const_executor = [](const std::byte* ctx, Args... args) {
                const Functor* l = reinterpret_cast<const Functor*>(ctx);
                return (*l)(args...);
            };
        }
        else if constexpr (is_const_member_function_v<decltype(&Functor::operator())> and not Const)
        {
            this->executor = [](std::byte* ctx, Args... args) {
                Functor* l = reinterpret_cast<Functor*>(ctx);
                return (*l)(args...);
            };
            const_executor = [](const std::byte* ctx, Args... args) {
                const Functor* l = reinterpret_cast<const Functor*>(ctx);
                return (*l)(args...);
            };
        }
        else if constexpr (not is_const_member_function_v<decltype(&Functor::operator())> and not Const)
        {
            this->executor = [](std::byte* ctx, Args... args) {
                Functor* l = reinterpret_cast<Functor*>(ctx);
                return (*l)(args...);
            };
            const_executor = [](const std::byte* ctx, Args...){ throw std::bad_function_call{}; };
        }
        else
        {
            throw std::invalid_argument{"cannot assign functor with non const call operator to const qualified stack_based_function"};
        }

        deleter = [](const std::byte* ctx) {
            const Functor* l = reinterpret_cast<const Functor*>(ctx);
            l->~Functor();
        };

#ifdef std_function_compat
        func_type_info = typeid(Functor);
#endif

        new (ctx_storage.data()) Functor{std::forward<Functor>(fctr)};
        return *this;
    }

    function& operator=(Ret(*ptr)(Args...))
    {
        static_assert(sizeof(Ret(*)(Args...)) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Ret(*)(Args...)) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");

        deleter(ctx_storage.data());

        this->executor = nullptr;
        const_executor = [](const std::byte* ctx, Args ...args){
            Func* f = *reinterpret_cast<Func**>(const_cast<std::byte*>(ctx));
            return f(args...);
        };
        deleter = trivial_deleter;

#ifdef std_function_compat
        func_type_info = typeid(decltype(ptr));
#endif

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

        this->executor = nullptr;
        const_executor = [](const std::byte*, T&, Args_...) { throw std::bad_function_call(); };
        deleter = trivial_deleter;

#ifdef std_function_compat
        func_type_info = typeid(decltype(ptr));
#endif

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

        this->executor = nullptr;
        const_executor = [](const std::byte* ctx, const T& obj, Args_ ...args){
            Ret_(T::*f)(Args_...) const = *reinterpret_cast<Ret_(T::**)(Args_...) const>(const_cast<std::byte*>(ctx));
            return (obj.*f)(args...);
        };
        deleter = trivial_deleter;

#ifdef std_function_compat
        func_type_info = typeid(decltype(ptr));
#endif

        using MemberPtr = Ret_(T::*)(Args_...) const;
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    function() :
        const_executor{[](const std::byte*){ throw std::bad_function_call{}; }},
        deleter{trivial_deleter}
#ifdef std_function_compat
        , func_type_info{typeid(void)}
#endif
    {}

    function(std::nullptr_t) :
        const_executor{[](const std::byte*){ throw std::bad_function_call{}; }},
        deleter{trivial_deleter}
#ifdef std_function_compat
        , func_type_info{typeid(void)}
#endif
    {}

    function(const function&) = delete;
    function(function&&) noexcept = delete;
    function& operator=(const function&) = delete;
    function& operator=(function&&) = delete;

    template <bool C = Const>
    typename std::enable_if_t<(not C), Ret>
    operator()(Args... args)
    {
        return this->executor(ctx_storage.data(), args...);
    }

    Ret operator()(Args... args) const
    {
        return const_executor(ctx_storage.data(), args...);
    }

    operator bool() const noexcept
    {
        return static_cast<bool>(this->executer) && static_cast<bool>(const_executor);
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return *this;
    }

#ifdef std_function_compat
    const std::type_info& target_type() const noexcept
    {
        return func_type_info;
    }
#endif

private:
    constexpr static inline std::size_t context_size = CtxSize;
    constexpr static inline std::size_t context_alignment = Align;

    static void trivial_deleter(const std::byte*) {}

    alignas(Align) std::array<std::byte, CtxSize> ctx_storage;  // lambda capture
    Ret(*const_executor)(const std::byte*, Args...);
    void(*deleter)(const std::byte*);
#ifdef std_function_compat
    std::reference_wrapper<const std::type_info> func_type_info;
#endif
};

struct S_;

static constexpr std::size_t min_size_ = std::max(sizeof(void (S_::*)()), sizeof(void*));
static constexpr std::size_t min_align_ = alignof(void (S_::*)());
static constexpr bool IsConst = true;

template <typename Functor>
function(Functor&&) ->
    function<std::max(sizeof(Functor), min_size_), std::max(alignof(Functor), min_align_), is_const_member_function_v<decltype(&Functor::operator())>, Signature<decltype(&Functor::operator())>>;

template <typename Ret, typename ...Args>
function(Ret(*)(Args...)) ->
    function<std::max(sizeof(Ret(*)(Args...)), min_size_) , std::max(alignof(Ret(*)(Args...)), min_align_), IsConst, Ret(Args...)>;

template <typename T, typename Ret_, typename ...Args_>
function(Ret_(T::*)(Args_...)) ->
    function<std::max(sizeof(Ret_(T::*)(Args_...)), min_size_), std::max(alignof(Ret_(T::*)(Args_...)), min_align_), IsConst, Ret_(T&, Args_...)>;

template <typename T, typename Ret_, typename ...Args_>
function(Ret_(T::*)(Args_...) const) ->
    function<std::max(sizeof(Ret_(T::*)(Args_...) const), min_size_), std::max(alignof(Ret_(T::*)(Args_...) const), min_align_), IsConst, Ret_(const T&, Args_...)>;
