#pragma once

#include <bit>

struct nontrivial_deleter_base
{
    void(*deleter)(const std::byte*);
};

struct dummy_deleter
{
    dummy_deleter(auto...) {}
    dummy_deleter& operator=(auto&&) noexcept { return *this; }
    void operator()(auto...) const {}
};

struct dummy_deleter_base
{
    [[no_unique_address]] dummy_deleter deleter;
};

inline void trivial_deleter(const std::byte*) {}
