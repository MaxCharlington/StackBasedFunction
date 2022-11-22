cmake_minimum_required(VERSION 3.16)

project(stack_based_function
        LANGUAGES CXX
        DESCRIPTION "A header-only library that implements std::function interface without dynamic allocations"
        HOMEPAGE_URL "https://github.com/MaxCharlington/StackBasedFunction")

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# set(CMAKE_CXX_CLANG_TIDY clang-tidy;-checks=*;-header-filter='${CMAKE_CURRENT_SOURCE_DIR}/include/*';-extra-arg=-Wno-unknown-warning-option)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (BUILD_TESTS EQUAL 1)
add_subdirectory(tests)
endif()

if (BUILD_BENCHMARKS EQUAL 1)
add_subdirectory(benchmarks)
endif()
