project(test CXX)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE stack_based_function)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wconversion -Werror -Wno-unused-variable -O0 -fanalyzer -Wno-analyzer-null-argument -fsanitize=address  -fsanitize=leak -g)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address  -fsanitize=leak)
