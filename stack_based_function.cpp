#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>

/* std::function replacement to store lambda of any size on stack*/
template <size_t CtxSize, size_t Align, typename Ret, typename ...Args>
class function {
    alignas(Align) std::array<std::byte, CtxSize> ctx;  // lambda capture
    Ret(*func)(void*, Args...);

public:
    template <typename Lambda>
    function(Lambda&& lmb)
    {
        func = [](void* ctx, Args... args) { return (*static_cast<Lambda*>(ctx))(args...); };
        new (ctx.data()) Lambda{std::forward<Lambda>(lmb)};
    }
    
    template <typename Ret_, typename ...Args_>
    function(Ret_(*ptr)(Args_...))
    {
        func = [](void* ctx, Args ...args){ return (reinterpret_cast<Ret(*)(Args...)>(ctx))(args...); };
        new (ctx.data()) decltype(ptr){ptr};
    }

    template <typename Lambda>
    function& operator=(Lambda&& lmb) {
        static_assert(sizeof(Lambda) == CtxSize);
        static_assert(alignof(Lambda) == Align);
        static_assert(std::is_same_v<decltype(lmb()), Ret>);

        func = [](void* ctx, Args... args) { return (*static_cast<Lambda*>(ctx))(args...); };
        new (ctx.data()) Lambda{std::forward<Lambda>(lmb)};
        return *this;
    }

    template <typename Ret_, typename ...Args_>
    function& operator=(Ret_(*ptr)(Args_...))
    {
        static_assert(std::is_same_v<Ret_, Ret>);
        static_assert(std::is_same_v<std::tuple<Args_...>, std::tuple<Args...>>);

        func = [](void* ctx, Args ...args){ return (reinterpret_cast<Ret(*)(Args...)>(ctx))(args...); };
        new (ctx.data()) decltype(ptr){ptr};
    }

    function() {
        func = nullptr;
    }

    function(const function&) = default;
    function(function&&) noexcept = default;
    function& operator=(const function&) = default;
    function& operator=(function&&) = default;

    template<typename ...Args_>
    Ret operator()(Args_... args) {
        static_assert(std::is_same_v<std::tuple<Args_...>, std::tuple<Args...>>);
    
        return func(ctx.data(), args...);
    }

    operator bool() {
        return func != nullptr;
    }
};

template <typename Lambda>
function(Lambda&& lmb) ->
    function<sizeof(Lambda), alignof(Lambda), decltype(lmb())>;

template <typename Ret_, typename ...Args_>
function(Ret_(*ptr)(Args_...)) ->
    function<sizeof(ptr), alignof(Ret_(*)(Args_...)), Ret_, Args_...>;


int foo(int) { return 0; }

int main()
{
    function f{[a=1, b=10](){}};
    f = [f=5.6f, g=1](){};

    f();

    function f2{[](){}};

    f2();

    function f3{&foo};

    // auto a = f3(1);
}
