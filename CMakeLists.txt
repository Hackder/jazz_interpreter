cmake_minimum_required(VERSION 3.14)
project(jazz)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/d122c0d435a6d305cdd50526127c84a98b77d87b.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  jazz
  ./src/core.hpp
  ./src/tokenizer.hpp
  ./src/tokenizer.cpp
  ./src/main.cpp
)

add_executable(
  jazz_test
  ./src/core.hpp
  ./src/tokenizer.hpp
  ./src/tokenizer.cpp
  ./tests/core_test.cpp
  ./tests/tokenizer_test.cpp
)

target_include_directories(
  jazz_test PRIVATE src
)

target_link_libraries(
  jazz_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(jazz_test)

