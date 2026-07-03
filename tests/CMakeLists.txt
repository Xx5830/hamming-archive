include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    FIND_PACKAGE_ARGS NAMES GTest
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Создать директорию ресурсов при сборке
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/resources)

add_executable(hamarc_tests
    test_archiver.cpp
)

target_link_libraries(hamarc_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(hamarc_tests PRIVATE cxx_std_23)

target_compile_definitions(hamarc_tests
    PRIVATE
        RESOURCES_DIR="${CMAKE_SOURCE_DIR}/tests/resources"
        HAMARC_EXE_PATH="$<TARGET_FILE:hamarc>"
)

include(GoogleTest)
gtest_discover_tests(hamarc_tests)
