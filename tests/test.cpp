#include <stack_based_function.hpp>

int foo(int) { return 0; }
void foo2() {}

struct S
{
    void f() {}
    void f2() const noexcept {}
};

int main()
{
    function f{[a=1, b=10](){}};
    f = [f=5.6f, g=1](){};

    f();

    function f2{[](){}};

    f2();

    function f3{&foo};
    f3(1);
    f3 = foo;
    f3(2);

    auto a = f3(1);

    function f4{[i=0lu](int){return 0;}};
    f4(1);
    f4 = &foo;
    f4(2);

    function f5(&S::f);
    S s{};
    f5(s);

    const function f6(&S::f2);
    f6(s);

    // function<void(int)> f7{};

    function f7{[a = 10](int b)mutable{ a = b; }};
    f7(4);
}
