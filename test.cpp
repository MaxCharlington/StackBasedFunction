#include "stack_based_function.hpp"

int foo(int) { return 0; }

int main()
{
    function f{[a=1, b=10](){}};
    f = [f=5.6f, g=1](){};

    f();

    function f2{[](){}};

    f2();

    function f3{&foo};

    auto a = f3(1);
}
