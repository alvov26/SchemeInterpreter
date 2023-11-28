cmake_minimum_required(VERSION 3.20)

project(SchemeInterpreter)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

add_subdirectory(3rd_party)
add_subdirectory(tests)
add_subdirectory(scheme)
add_subdirectory(repl)