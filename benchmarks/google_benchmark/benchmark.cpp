#include <benchmark/benchmark.h>

#include <functional>
#include <vector>
#include <stack_based_function.hpp>

// https://quick-bench.com/q/FMlFWvYfRnOg9zhiS7rqJHg4D_4

static void BM_FunctionCreation(benchmark::State& state) {
    for (auto _ : state)
        function f{[a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3](){}};
}
BENCHMARK(BM_FunctionCreation);

static void BM_StdFunctionCreation(benchmark::State& state) {
    for (auto _ : state)
        std::function f{[a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3](){}};
}
BENCHMARK(BM_StdFunctionCreation);

static void BM_FunctionInvoke(benchmark::State& state) {
    function f{[a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; }};
    for (auto _ : state)
        auto d = f();
}
BENCHMARK(BM_FunctionInvoke);

static void BM_StdFunctionInvoke(benchmark::State& state) {
    std::function f{[a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; }};
    for (auto _ : state)
        auto d = f();
}
BENCHMARK(BM_StdFunctionInvoke);

static void BM_FunctionInitVector(benchmark::State& state) {
    function f{[a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; }};
    std::vector<function<40, 8, Const, false, double()>> v;
    v.reserve(301*301);
    for (auto _ : state)
    {
        for (size_t i = 0; i < v.capacity(); i++)
            v.emplace_back([a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; });
        v.clear();
    }
}
BENCHMARK(BM_FunctionInitVector);

static void BM_StdFunctionInitVector(benchmark::State& state) {
    std::vector<std::function<double()>> v;
    v.reserve(301*301);
    for (auto _ : state)
    {
        for (size_t i = 0; i < v.capacity(); i++)
            v.emplace_back([a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; });
        v.clear();
    }
}
BENCHMARK(BM_StdFunctionInitVector);

struct F
{
    double a, b, c, d, e;
    double operator()() { return d; }
};

static void BM_FunctorInitVector(benchmark::State& state) {
    std::vector<F> v;
    v.reserve(301*301);
    for (auto _ : state)
    {
        for (size_t i = 0; i < v.capacity(); i++)
            v.emplace_back(F{.a = 0.5, .b = 1.5, .c = 7.5, .d = 5.6, .e = 3});
        v.clear();
    }
}
BENCHMARK(BM_FunctorInitVector);

static void BM_FunctionVectorRun(benchmark::State& state) {
    std::vector<function<40, 8, Const, false, double()>> v;
    v.reserve(301*301);
    for (size_t i = 0; i < v.capacity(); i++)
        v.emplace_back([a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; });
    for (auto _ : state)
    {
        for (auto& f : v)
            auto r = f();
    }
}
BENCHMARK(BM_FunctionVectorRun);

static void BM_StdFunctionVectorRun(benchmark::State& state) {
    std::vector<std::function<double()>> v;
    v.reserve(301*301);
    for (size_t i = 0; i < v.capacity(); i++)
        v.emplace_back([a = 0.5, b = 1.5, c = 7.5, d = 5.6, e = 3]{ return d; });
    for (auto _ : state)
    {
        for (auto& f : v)
            auto r = f();
    }
}
BENCHMARK(BM_StdFunctionVectorRun);

static void BM_FunctorVectorRun(benchmark::State& state) {
    std::vector<F> v;
    v.reserve(301*301);
    for (size_t i = 0; i < v.capacity(); i++)
        v.emplace_back(F{.a = 0.5, .b = 1.5, .c = 7.5, .d = 5.6, .e = 3});
    v.clear();
    for (auto _ : state)
    {
        for (auto& f : v)
            auto r = f();
    }
}
BENCHMARK(BM_FunctorVectorRun);

