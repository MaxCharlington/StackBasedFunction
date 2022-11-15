#pragma once

#include <concepts>

#include "signature_helper.hpp"

template<typename Lambda>
concept Capturing = not requires(Lambda l)
{
    { l } -> std::convertible_to<
        std::add_pointer_t<
            Signature<decltype(&Lambda::operator())>
        >
    >;
};
