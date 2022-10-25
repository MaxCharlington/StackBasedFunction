#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>

// std::function replacement to store lambda of any size on stack
template <size_t CtxSize, size_t Align, typename Ret, typename ...Args>
class function {
    alignas(Align) std::array<std::byte, CtxSize> ctx;  // lambda capture
    Ret(*func)(void*);

public:
    template <typename Lambda>
    function(Lambda&& lmb)
    {
        if constexpr (CtxSize == 0)
        {
            func = [ptr = static_cast<Ret(*)(Args...)>(lmb)]([[maybe_unused]] void*, Args ...args){ return ptr(args...); };
        }
        else 
        {
            func = [](void* ctx, Args... args) { return (*static_cast<Lambda*>(ctx))(args...); };
            new (ctx.data()) Lambda{std::forward<Lambda>(lmb)};
        }
    }
    
    template <typename Ret_, typename ...Args_>
    function(Ret_(*lmb)(Args_...))
    {
        func = [lmb]([[maybe_unused]] void*, Args_ ...args){ return lmb(args...); };
    }

    template <typename Lambda>
    function& operator=(Lambda&& lmb) {
        static_assert(sizeof(Lambda) == CtxSize);
        static_assert(alignof(Lambda) == Align);
        static_assert(std::is_same_v<decltype(lmb()), Ret>);

        if constexpr (CtxSize == 0)
        {
            func = [ptr = static_cast<Ret(*)(Args...)>(lmb)]([[maybe_unused]] void*, Args ...args){ return ptr(args...); };
        }
        else 
        {
            func = [](void* ctx, Args... args) { return (*static_cast<Lambda*>(ctx))(args...); };
            new (ctx.data()) Lambda{std::forward<Lambda>(lmb)};
        }
        return *this;
    }

    template <typename Ret_, typename ...Args_>
    function& operator=(Ret_(*lmb)(Args_...))
    {
        func = [lmb]([[maybe_unused]] void*, Args_ ...args){ return lmb(args...); };
    }

    // function() {
    //     func = nullptr;
    // }

    function(const function&) = default;
    function(function&&) noexcept = default;
    function& operator=(const function&) = default;
    function& operator=(function&&) = default;

    decltype(auto) operator()() {
        if constexpr (CtxSize == 0)
        {
            return func();
        }
        else
        {
            return func(ctx.data());
        }
    }

    operator bool() {
        return func != nullptr;
    }
};

template <typename Lambda>
function(Lambda&& lmb) ->
    function<sizeof(Lambda), alignof(Lambda), decltype(lmb())>;

template <typename Ret_, typename ...Args_>
function(Ret_(*lmb)(Args_...)) ->
    function<0, 0, Ret_, Args_...>;

int f(int) { return 0; }

int main()
{
    function f{[a=1, b=10](){}};
    f = [f=5.6f, g=1](){};

    function f2{[](){}};
    // function f3{&f};
}
