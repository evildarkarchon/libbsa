cmake_minimum_required(VERSION 3.21)

project(libbsa VERSION 0.1.0 LANGUAGES CXX)

find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

add_library(libbsa INTERFACE)
add_library(libbsa::libbsa ALIAS libbsa)

target_compile_features(libbsa INTERFACE cxx_std_20)

target_link_libraries(libbsa
    INTERFACE
        $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
        lz4::lz4
)
