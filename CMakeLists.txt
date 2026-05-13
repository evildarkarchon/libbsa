cmake_minimum_required(VERSION 4.0)

project(libbsa VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)

option(LIBBSA_BUILD_TESTS "Build libbsa tests" ${BUILD_TESTING})
option(LIBBSA_BUILD_BENCHMARKS "Build libbsa benchmark tools" ON)

if(TARGET LZ4::lz4_static)
  set(libbsa_lz4_target LZ4::lz4_static)
elseif(TARGET LZ4::lz4_shared)
  set(libbsa_lz4_target LZ4::lz4_shared)
else()
  set(libbsa_lz4_target lz4::lz4)
endif()

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY
  )

  add_custom_target(libbsa_docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating libbsa public API documentation"
    VERBATIM
  )
else()
  message(STATUS "Doxygen not found; libbsa_docs target not available")
endif()

add_library(libbsa)
add_library(libbsa::libbsa ALIAS libbsa)

target_compile_features(libbsa PUBLIC cxx_std_20)

get_target_property(libbsa_target_type libbsa TYPE)
if(libbsa_target_type STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(libbsa
    PRIVATE
      LIBBSA_BUILDING_LIBRARY
  )
elseif(libbsa_target_type STREQUAL "STATIC_LIBRARY")
  target_compile_definitions(libbsa
    PUBLIC
      LIBBSA_STATIC_DEFINE
  )
endif()

get_target_property(libbsa_windows_auto_exports libbsa WINDOWS_EXPORT_ALL_SYMBOLS)
if(libbsa_windows_auto_exports)
  message(FATAL_ERROR "libbsa uses explicit LIBBSA_API annotations; WINDOWS_EXPORT_ALL_SYMBOLS must remain disabled")
endif()

target_include_directories(libbsa
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(libbsa
  PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_link_libraries(libbsa
  PRIVATE
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,libdeflate::libdeflate_static,libdeflate::libdeflate_shared>
    # Use the concrete vcpkg target when available so installed shared exports retain the LZ4 runtime DLL.
    ${libbsa_lz4_target}
    Microsoft::DirectXTex
    bcrypt
)

set(libbsa_library_sources
  src/archive.cpp
  src/validation.cpp
  src/detail/archive_path.cpp
  src/detail/bethesda_hash.cpp
  src/detail/binary_io.cpp
  src/detail/compression_router.cpp
  src/detail/deflate_codec.cpp
  src/detail/lz4_block_codec.cpp
  src/detail/lz4_frame_codec.cpp
  src/detail/parallel_work.cpp
  src/detail/parser_primitives.cpp
  src/detail/payload_stream.cpp
  src/detail/writer_publish.cpp
  src/detail/host_file.cpp
  src/detail/host_file_path.cpp
  src/formats/ba2/ba2_format_detector.cpp
  src/formats/ba2/ba2_dx10_parser.cpp
  src/formats/ba2/ba2_dx10_reader.cpp
  src/formats/ba2/ba2_dx10_prepare.cpp
  src/formats/ba2/ba2_dx10_layout.cpp
  src/formats/ba2/ba2_dx10_serialize.cpp
  src/formats/ba2/ba2_dx10_writer.cpp
  src/formats/ba2/ba2_gnrl_parser.cpp
  src/formats/ba2/ba2_gnrl_reader.cpp
  src/formats/ba2/ba2_gnrl_prepare.cpp
  src/formats/ba2/ba2_gnrl_layout.cpp
  src/formats/ba2/ba2_gnrl_serialize.cpp
  src/formats/ba2/ba2_gnrl_writer.cpp
  src/formats/bsa/bsa_format_detector.cpp
  src/formats/bsa/tes3_bsa_parser.cpp
  src/formats/bsa/tes3_bsa_reader.cpp
  src/formats/bsa/tes3_bsa_prepare.cpp
  src/formats/bsa/tes3_bsa_layout.cpp
  src/formats/bsa/tes3_bsa_serialize.cpp
  src/formats/bsa/tes3_bsa_writer.cpp
  src/formats/bsa/tes4_bsa_parser.cpp
  src/formats/bsa/tes4_bsa_reader.cpp
  src/formats/bsa/tes4_bsa_prepare.cpp
  src/formats/bsa/tes4_bsa_layout.cpp
  src/formats/bsa/tes4_bsa_serialize.cpp
  src/formats/bsa/tes4_bsa_writer.cpp
  src/texture/dds_layout.cpp
  src/texture/directxtex_analyzer.cpp
  src/libbsa.cpp
)

target_sources(libbsa
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/libbsa/archive.hpp
        include/libbsa/export.hpp
        include/libbsa/libbsa.hpp
        include/libbsa/result.hpp
        include/libbsa/validation.hpp
        include/libbsa/version.hpp
        include/libbsa/writer.hpp
  PRIVATE
    ${libbsa_library_sources}
)

if(LIBBSA_BUILD_BENCHMARKS)
  add_executable(libbsa_benchmarks
    benchmarks/libbsa_benchmarks.cpp
  )

  target_link_libraries(libbsa_benchmarks
    PRIVATE
      libbsa::libbsa
  )

  target_compile_features(libbsa_benchmarks PRIVATE cxx_std_20)

  add_custom_target(libbsa_benchmark_report
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/benchmarks"
    COMMAND $<TARGET_FILE:libbsa_benchmarks>
      --output-json "${CMAKE_BINARY_DIR}/benchmarks/libbsa-benchmark.json"
      --output-markdown "${CMAKE_BINARY_DIR}/benchmarks/libbsa-benchmark.md"
    DEPENDS libbsa_benchmarks
    COMMENT "Generating libbsa synthetic benchmark report"
    VERBATIM
  )
endif()

install(TARGETS libbsa
  EXPORT libbsaTargets
  FILE_SET HEADERS
)

install(EXPORT libbsaTargets
  NAMESPACE libbsa::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libbsa
)

configure_package_config_file(
  cmake/libbsaConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/libbsaConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libbsa
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/libbsaConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libbsaConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/libbsaConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libbsa
)

if(BUILD_TESTING AND LIBBSA_BUILD_TESTS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()
