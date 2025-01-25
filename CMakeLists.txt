cmake_minimum_required(VERSION 3.14)
project(jazz)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/d122c0d435a6d305cdd50526127c84a98b77d87b.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_compile_options(-Wall -Wextra -Wpedantic -Wuninitialized -Wno-gnu-zero-variadic-macro-arguments)

set(SOURCE_FILES
  ./src/core.hpp
  ./src/tokenizer.hpp
  ./src/tokenizer.cpp
  ./src/ast.hpp
  ./src/ast.cpp
  ./src/parser.hpp
  ./src/parser.cpp
  ./src/token_pos.hpp
  ./src/token_pos.cpp
  ./src/sema.hpp
  ./src/sema.cpp
  ./src/bytecode.hpp
  ./src/compiler.hpp
  ./src/compiler.cpp
  ./src/vm.hpp
  ./src/vm.cpp
)

add_executable(
  jazz
  ${SOURCE_FILES}
  ./src/main.cpp
)
target_compile_options(jazz PRIVATE)

add_executable(
  jazz_test
  ${SOURCE_FILES}
  ./tests/common.hpp
  ./tests/core_test.cpp
  ./tests/tokenizer_test.cpp
  ./tests/parser_test.cpp
  ./tests/token_pos_test.cpp
  ./tests/sema_test.cpp
  ./tests/vm_test.cpp
  ./tests/e2e.cpp
)
target_compile_options(jazz_test PRIVATE)

target_include_directories(
  jazz_test PRIVATE src
)

target_link_libraries(
  jazz_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(jazz_test)

# Set the compilers to Homebrew-installed Clang
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")

# Add LLVM include and library paths
include_directories("/opt/homebrew/opt/llvm/include")
link_directories("/opt/homebrew/opt/llvm/lib")

add_executable(
  fuzz_tokenizer
  ${SOURCE_FILES}
  ./fuzztests/tokenizer_fuzz_test.cpp
)
target_compile_definitions(fuzz_tokenizer PRIVATE LIBFUZZER)
target_compile_options(fuzz_tokenizer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
target_include_directories(fuzz_tokenizer PRIVATE src)
target_link_libraries(fuzz_tokenizer PRIVATE -fsanitize=fuzzer,address)

add_executable(
  fuzz_parser
  ${SOURCE_FILES}
  ./fuzztests/parser_fuzz_test.cpp
)
target_compile_definitions(fuzz_parser PRIVATE LIBFUZZER)
target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
target_include_directories(fuzz_parser PRIVATE src)
target_link_libraries(fuzz_parser PRIVATE -fsanitize=fuzzer,address)

