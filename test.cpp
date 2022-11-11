#include <iostream>
#include <functional>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "stack_based_function.hpp"

using namespace std::chrono;

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

    f = [](){};

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

    // Benchmark

    std::cout << "std::function " << sizeof(std::function<void()>) << "byte\nstack function " << sizeof(function<4, 8, Const, Trivial, void()>) << "byte\n\n";

    const size_t size = 5000000;
    std::vector<std::function<void()>> arr_function;
    std::vector<function<4, 4, Const, Trivial, void()>> arr_stack_function;

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function.push_back(foo2);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "std::function " << duration.count() << " microseconds\n";

    start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function.push_back(foo2);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "stack function " << duration.count() << " microseconds\n";

    start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function[i]();
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "std::function call " << duration.count() << " microseconds\n";

    start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function[i]();
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "stack function call " << duration.count() << " microseconds\n";

    return 0;
}
