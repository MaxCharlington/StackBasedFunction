#pragma once

template<typename Ret, typename ...Args>
struct nonconst_executor_base
{
    Ret(*executor)(std::byte*, Args...) = nullptr;
};

struct dummy_executor
{
    dummy_executor(auto...) {}
    dummy_executor& operator=(auto&&) { return *this; }
};

struct dummy_executor_base
{
    [[no_unique_address]] dummy_executor executor;
};

