#include <iostream>
#include <functional>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include <stack_based_function.hpp>

using namespace std::chrono;

int main()
{
    std::cout << "std::function " << sizeof(std::function<void()>) << "byte\nstack function " << sizeof(function<4, 8, Const, Trivial, void()>) << "byte\n\n";

    const size_t size = 5000000;
    std::vector<std::function<void()>> arr_function;
    std::vector<function<40, 8, Const, false, void()>> arr_stack_function;

    const auto l = [a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3](){};
    static_assert(sizeof(l) == 40);
    static_assert(alignof(l) == 8);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function.push_back(l);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "[INIT] std::function " << duration.count() << " microseconds\n";

    start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function.push_back(l);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "[INIT] stack based function " << duration.count() << " microseconds\n";

    start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function[i]();
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "[INVOKE] std::function " << duration.count() << " microseconds\n";

    start = high_resolution_clock::now();
    for (size_t i = 0; i < size; i++)
    {
        arr_function[i]();
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "[INVOKE] stack based function " << duration.count() << " microseconds\n";

    return 0;
}
