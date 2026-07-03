cmake_minimum_required(VERSION 3.20)

project(HamArc
    VERSION 1.0.0
    DESCRIPTION "Помехоустойчивый архиватор на основе кодов Хэмминга"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()


add_executable(hamarc
    bin/main.cpp
    src/manage_haf.cpp
    src/manage_file.cpp
    src/argparser.cpp
)

target_include_directories(hamarc PRIVATE src)
target_compile_features(hamarc PRIVATE cxx_std_23)


if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
