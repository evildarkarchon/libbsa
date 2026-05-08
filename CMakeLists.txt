cmake_minimum_required(VERSION 3.24)

project(libbsa VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(LIBBSA_BUILD_TESTS "Build libbsa tests" ON)

add_library(libbsa)
add_library(libbsa::libbsa ALIAS libbsa)

set_target_properties(libbsa PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_compile_features(libbsa PUBLIC cxx_std_20)

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

target_sources(libbsa
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/libbsa/archive.hpp
        include/libbsa/libbsa.hpp
        include/libbsa/result.hpp
        include/libbsa/version.hpp
  PRIVATE
    src/archive.cpp
    src/detail/archive_path.cpp
    src/detail/binary_io.cpp
    src/detail/payload_stream.cpp
    src/libbsa.cpp
)

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

if(LIBBSA_BUILD_TESTS)
  include(CTest)
  enable_testing()
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()
