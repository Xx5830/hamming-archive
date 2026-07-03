cmake_minimum_required(VERSION 3.20)

project(HamArc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

#add_subdirectory(src)
#add_subdirectory(bin)
add_executable(
    hamarc
    bin/main.cpp
    src/manage_haf.cpp
    src/manage_file.cpp
    src/argparser.cpp
    src/headers/argparser.hpp
    src/headers/manager_files.hpp
)

enable_testing()
add_subdirectory(tests)
