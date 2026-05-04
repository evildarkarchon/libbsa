cmake_minimum_required(VERSION 3.21)

project(libbsa VERSION 0.1.0 LANGUAGES CXX)

find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

option(BUILD_SHARED_LIBS "Build libbsa as a shared library." OFF)

add_library(libbsa
    src/archive_reader.cpp
    src/ba2_gnrl_archive.cpp
    src/ba2_hash.cpp
    src/tes3_archive.cpp
    src/tes3_hash.cpp
    src/tes4_archive.cpp
    src/tes4_hash.cpp
)
add_library(libbsa::libbsa ALIAS libbsa)

target_compile_features(libbsa PUBLIC cxx_std_20)

get_target_property(LIBBSA_LIBRARY_TYPE libbsa TYPE)
if(LIBBSA_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    target_compile_definitions(libbsa
        PUBLIC
            LIBBSA_SHARED
        PRIVATE
            LIBBSA_BUILDING_LIBRARY
    )
endif()

target_include_directories(libbsa
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(libbsa
    PRIVATE
        $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
        lz4::lz4
)

include(CTest)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(libbsa_tests
        tests/archive_reader_tests.cpp
        tests/fixture_builder.cpp
        src/ba2_hash.cpp
        src/tes3_hash.cpp
    )
    target_compile_features(libbsa_tests PRIVATE cxx_std_20)
    target_include_directories(libbsa_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(libbsa_tests
        PRIVATE
            libbsa::libbsa
            Catch2::Catch2WithMain
            $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
            lz4::lz4
    )

    include(Catch)
    catch_discover_tests(libbsa_tests)

    add_executable(libbsa_hash_tests
        tests/hash_tests.cpp
        src/ba2_hash.cpp
        src/tes3_hash.cpp
        src/tes4_hash.cpp
    )
    target_compile_features(libbsa_hash_tests PRIVATE cxx_std_20)
    target_include_directories(libbsa_hash_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(libbsa_hash_tests PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(libbsa_hash_tests)

    add_executable(public_header_smoke tests/public_header_smoke.cpp)
    target_compile_features(public_header_smoke PRIVATE cxx_std_20)
    target_link_libraries(public_header_smoke PRIVATE libbsa::libbsa)
    add_test(NAME public_header_smoke COMMAND public_header_smoke)
endif()
