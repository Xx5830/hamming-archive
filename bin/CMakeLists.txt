add_executable(
    hamarc
    main.cpp
)

target_link_libraries(hamarc PRIVATE argparser)
target_link_libraries(hamarc PRIVATE manager_file)
target_compile_features(hamarc PRIVATE cxx_std_20)
