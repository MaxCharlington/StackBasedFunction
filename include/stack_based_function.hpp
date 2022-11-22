#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>
#include <bit>

#include "args_helper.hpp"
#include "const_member_function_helper.hpp"
#include "lambda_capture_helper.hpp"
#include "nonconst_executor.hpp"
#include "nontrivial_deleter.hpp"
#include "signature_helper.hpp"

template<size_t CtxSize, size_t Align, bool Const, bool Trivial, typename Signature>
class function;

/* std::function replacement with stack storage */
template <size_t CtxSize, size_t Align, bool Const, bool Trivial, typename Ret, typename ...Args>
class function<CtxSize, Align, Const, Trivial, Ret(Args...)> :
    private std::conditional_t<Const, dummy_executor_base, nonconst_executor_base<Ret, Args...>>,
    private std::conditional_t<Trivial, dummy_deleter_base, nontrivial_deleter_base>
{
public:
    using signature = Ret(Args...);
    using result_type = Ret;
    template<std::size_t N>
    using nth_argument = nth_argument<N, Args...>;
    static constexpr std::size_t n_arguments = sizeof...(Args);

    template <typename Functor>
        requires Capturing<Functor> and (not std::is_member_function_pointer_v<Functor>)
    function(Functor&& fctr) :
        const_executor{
            []{
                if constexpr (Const)
                {
                    return [](const std::byte* ctx, Args... args) {
                        const Functor& f = *std::bit_cast<const Functor*>(ctx);
                        return f(args...);
                    };
                }
                else
                {
                    return [](const std::byte*, Args...) { throw std::bad_function_call(); };
                }
            }()
        }
#ifdef std_function_compat
        , func_type_info{typeid(Functor)}
#endif
    {
        if constexpr (not Const)
        {
            // The only condition where underlying context may be modified
            this->executor = [](std::byte* ctx, Args... args) {
                Functor& f = *std::bit_cast<Functor*>(ctx);
                return f(args...);
            };
        }
        if constexpr (not Trivial)
        {
            this->deleter = [](const std::byte* ctx) {
                const Functor& f = *std::bit_cast<const Functor*>(ctx);
                f.~Functor();
            };
        }
        new (ctx_storage.data()) Functor{std::forward<Functor>(fctr)};
    }

    function(Ret(*ptr)(Args...)) :
        const_executor{[](const std::byte* ctx, Args ...args) {
            using FuncPtr = Ret(*)(Args...);
            FuncPtr f = *std::bit_cast<FuncPtr*>(ctx);
            return f(args...);
        }}
#ifdef std_function_compat
        , func_type_info{typeid(decltype(ptr))}
#endif
    {
        using FuncPtr = Ret(*)(Args...);
        new (ctx_storage.data()) FuncPtr{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function(Ret_(T::*ptr)(Args_...)) :
        const_executor{[](const std::byte* ctx, T& obj, Args_ ...args){
            using MemberPtr = Ret_(T::*)(Args_...);
            MemberPtr f = *std::bit_cast<MemberPtr*>(ctx);
            return (obj.*f)(args...);
        }}
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
            using MemberPtr = Ret_(T::*)(Args_...) const;
            MemberPtr f = *std::bit_cast<MemberPtr*>(ctx);
            return (obj.*f)(args...);
        }}
#ifdef std_function_compat
        , func_type_info{typeid(decltype(ptr))}
#endif
    {
        using MemberPtr = Ret_(T::*)(Args_...) const;
        new (ctx_storage.data()) MemberPtr{ptr};
    }

    template <typename Functor>
        requires Capturing<Functor> and (not std::is_member_function_pointer_v<Functor>)
    function& operator=(Functor&& fctr) {
        static_assert(sizeof(Functor) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(Functor) <= Align, "reassigned callable alignment should be less or equal than initial");
        static_assert(std::is_same_v<Ret(Args...), ::Signature<decltype(&Functor::operator())>>, "reassigned callable signature should be the same as initial");
        static_assert(not(Trivial and not std::is_trivially_destructible_v<Functor>), "assigning to trivial stack_based_functon of not trivially destructible functor");
        static_assert(not(Const && not is_const_member_function_v<decltype(&Functor::operator())>), "assigning of functor with non const call operator to const qualified stack_based_function");

        if constexpr (not Trivial) {
            this->deleter(ctx_storage.data());
            this->deleter = [](const std::byte* ctx) {
                const Functor& f = *std::bit_cast<const Functor*>(ctx);
                f.~Functor();
            };
        }

        if constexpr (is_const_member_function_v<decltype(&Functor::operator())> and Const)
        {
            const_executor = [](const std::byte* ctx, Args... args) {
                const Functor& f = *std::bit_cast<const Functor*>(ctx);
                return f(args...);
            };
        }
        else if constexpr (is_const_member_function_v<decltype(&Functor::operator())> and not Const)
        {
            this->executor = [](std::byte* ctx, Args... args) {
                Functor& f = *std::bit_cast<Functor*>(ctx);
                return f(args...);
            };
            const_executor = [](const std::byte* ctx, Args... args) {
                const Functor& f = *std::bit_cast<const Functor*>(ctx);
                return f(args...);
            };
        }
        else
        {
            this->executor = [](std::byte* ctx, Args... args) {
                Functor& f = *std::bit_cast<Functor*>(ctx);
                return f(args...);
            };
            const_executor = [](const std::byte*, Args...){ throw std::bad_function_call{}; };
        }

#ifdef std_function_compat
        func_type_info = typeid(Functor);
#endif

        new (ctx_storage.data()) Functor{std::forward<Functor>(fctr)};
        return *this;
    }

    function& operator=(Ret(*ptr)(Args...))
    {
        using FuncPtr = Ret(*)(Args...);
        static_assert(sizeof(FuncPtr) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(FuncPtr) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");

        if constexpr (not Trivial) {
            this->deleter(ctx_storage.data());
            this->deleter = trivial_deleter;
        }

        this->executor = nullptr;
        const_executor = [](const std::byte* ctx, Args ...args){
            FuncPtr f = *std::bit_cast<FuncPtr*>(ctx);
            return f(args...);
        };

#ifdef std_function_compat
        func_type_info = typeid(decltype(ptr));
#endif

        new (ctx_storage.data()) FuncPtr{ptr};
        return *this;
    }

    template<typename T, typename Ret_, typename ...Args_>
    function& operator=(Ret_(T::*ptr)(Args_...))
    {
        using MemberPtr = Ret_(T::*)(Args...);
        static_assert(sizeof(MemberPtr) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(MemberPtr) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");
        static_assert(std::is_same_v<Ret(Args...), Ret_(T&, Args_...)>, "reassigned callable signature should be the same as initial");

        if constexpr (not Trivial) {
            this->deleter(ctx_storage.data());
            this->deleter = trivial_deleter;
        }

        if constexpr (not Const)
        {
            this->executor = [](std::byte*, Args...){ throw std::bad_function_call{}; };
        }
        const_executor = [](const std::byte* ctx, T& obj, Args_ ...args){
            MemberPtr f = *std::bit_cast<MemberPtr*>(ctx);
            return (obj.*f)(args...);
        };

#ifdef std_function_compat
        func_type_info = typeid(decltype(ptr));
#endif

        new (ctx_storage.data()) MemberPtr{ptr};
    }

    template<typename T, typename Ret_, typename ...Args_>
    function& operator=(Ret_(T::*ptr)(Args_...) const)
    {
        using MemberPtr = Ret_(T::*)(Args_...) const;
        static_assert(sizeof(MemberPtr const) <= CtxSize, "reassigned callable size should be less or equal than initial");
        static_assert(alignof(MemberPtr const) <= Align, "initial callable alingment should be alignment of a pointer to store this callable");
        static_assert(std::is_same_v<Ret(Args...), Ret_(const T&, Args_...)>, "reassigned callable signature should be the same as initial");

        if constexpr (not Trivial) {
            this->deleter(ctx_storage.data());
            this->deleter = trivial_deleter;
        }

        if constexpr (not Const)
        {
            this->executor = [](std::byte*, Args...){ throw std::bad_function_call{}; };
        }
        const_executor = [](const std::byte* ctx, const T& obj, Args_ ...args){
            MemberPtr f = *std::bit_cast<MemberPtr*>(ctx);
            return (obj.*f)(args...);
        };

#ifdef std_function_compat
        func_type_info = typeid(decltype(ptr));
#endif

        new (ctx_storage.data()) MemberPtr{ptr};
    }

    function() :
        const_executor{[](const std::byte*){ throw std::bad_function_call{}; }}
#ifdef std_function_compat
        , func_type_info{typeid(void)}
#endif
    {}

#ifdef std_function_compat
    function(std::nullptr_t) :
        const_executor{[](const std::byte*){ throw std::bad_function_call{}; }}
        , func_type_info{typeid(void)}
    {}
#endif

    function(const function&) = default;
    function(function&&) noexcept = default;
    function& operator=(const function&) = default;
    function& operator=(function&&) = default;

    ~function()
    {
        if constexpr (not Trivial) {
            this->deleter(ctx_storage.data());
        }
    }

    template <typename ...Args_, bool C = Const>
    typename std::enable_if_t<(not C), Ret>
    operator()(Args_&&... args)
    {
        return this->executor(ctx_storage.data(), std::forward<Args_>(args)...);
    }

    template <typename ...Args_>
    Ret operator()(Args_&&... args) const
    {
        return const_executor(ctx_storage.data(), std::forward<Args_>(args)...);
    }

#ifdef std_function_compat
    operator bool() const noexcept
    {
        return func_type_info != typeid(void);
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return *this;
    }
#endif

#ifdef std_function_compat
    const std::type_info& target_type() const noexcept
    {
        return func_type_info;
    }
#endif

private:
    constexpr static inline std::size_t context_size = CtxSize;
    constexpr static inline std::size_t context_alignment = Align;

    alignas(Align) std::array<std::byte, CtxSize> ctx_storage;  // lambda capture
    Ret(*const_executor)(const std::byte*, Args...);
#ifdef std_function_compat
    std::reference_wrapper<const std::type_info> func_type_info;
#endif
};


// DEDUCTION GUIDES
struct S_;

static constexpr bool Const = true;
static constexpr bool Trivial = true;

template <typename Functor>
function(Functor&&) ->
    function<
        sizeof(Functor),
        alignof(Functor),
        is_const_member_function_v<decltype(&Functor::operator())>,
        std::is_trivially_destructible_v<Functor>,
        Signature<decltype(&Functor::operator())>
    >;

template <typename Ret, typename ...Args>
function(Ret(*)(Args...)) ->
    function<
        sizeof(Ret(*)(Args...)),
        alignof(Ret(*)(Args...)),
        Const,
        Trivial,
        Ret(Args...)
    >;

template <typename T, typename Ret_, typename ...Args_>
function(Ret_(T::*)(Args_...)) ->
    function<
        sizeof(Ret_(T::*)(Args_...)),
        alignof(Ret_(T::*)(Args_...)),
        Const,
        Trivial,
        Ret_(T&, Args_...)
    >;

template <typename T, typename Ret_, typename ...Args_>
function(Ret_(T::*)(Args_...) const) ->
    function<
        sizeof(Ret_(T::*)(Args_...) const),
        alignof(Ret_(T::*)(Args_...) const),
        Const,
        Trivial,
        Ret_(const T&, Args_...)
    >;
