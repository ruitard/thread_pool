cmake_minimum_required(VERSION 3.0.0)
project(thread_pool VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(CTest)
enable_testing()

find_package(Threads REQUIRED)

add_executable(demo main.cpp thread_pool.hpp)

target_compile_options(demo PRIVATE -Wall)
target_link_libraries(demo PRIVATE Threads::Threads)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
