#include <iostream>
#include <functional>
#include "stack_based_function.hpp"

int foo(int) { return 0; }

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

    f = [](){};

    function f2{[](){}};

    f2();

    function f3{&foo};
    f3 = foo;

    auto a = f3(1);

    function f4{[i=0lu](int){return 0;}};
    f4 = &foo;

    function f5(&S::f);
    S s{};
    f5(s);

    const function f6(&S::f2);
    f6(s);
}
