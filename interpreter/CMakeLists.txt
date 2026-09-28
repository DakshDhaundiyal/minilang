cmake_minimum_required(VERSION 3.14)
project(minilang VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Main interpreter library (shared between CLI and tests)
add_library(minilang_lib STATIC
    src/token.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/semantic.cpp
    src/interpreter.cpp
    src/json.cpp
)
target_include_directories(minilang_lib PUBLIC src)

# CLI executable
add_executable(minilang src/main.cpp)
target_link_libraries(minilang PRIVATE minilang_lib)

# Tests
enable_testing()
add_executable(minilang_tests tests/test_main.cpp)
target_link_libraries(minilang_tests PRIVATE minilang_lib)
add_test(NAME MiniLangTests COMMAND minilang_tests)
