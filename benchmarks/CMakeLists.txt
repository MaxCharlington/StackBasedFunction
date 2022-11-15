project(benchmark CXX)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE stack_based_function)

target_compile_options(${PROJECT_NAME} PRIVATE -O3)
