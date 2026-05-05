cmake_minimum_required(VERSION 3.24)

option(BUILD_SHARED_LIBS "Build libbsa as a shared library" OFF)

project(libbsa VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(DirectXTex CONFIG REQUIRED)

include(CTest)

function(libbsa_require_first_target output_variable)
  foreach(candidate IN LISTS ARGN)
    if(TARGET "${candidate}")
      set(${output_variable} "${candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  message(FATAL_ERROR "None of the expected imported targets exists: ${ARGN}")
endfunction()

libbsa_require_first_target(LIBBSA_LIBDEFLATE_TARGET
  libdeflate::libdeflate
  libdeflate::libdeflate_static
  libdeflate::libdeflate_shared
  unofficial::libdeflate::libdeflate)
libbsa_require_first_target(LIBBSA_LZ4_TARGET
  lz4::lz4
  lz4::lz4_static
  lz4::lz4_shared)
libbsa_require_first_target(LIBBSA_DIRECTXTEX_TARGET
  Microsoft::DirectXTex
  DirectXTex::DirectXTex
  DirectXTex)

add_library(libbsa)
add_library(libbsa::libbsa ALIAS libbsa)

target_compile_features(libbsa PUBLIC cxx_std_20)

# TES5Edit/ is read-only reference material; do not compile, link, vendor,
# or add it to libbsa source lists.
target_sources(libbsa
  PRIVATE
    src/libbsa.cpp
    src/detect.cpp
    src/io.cpp
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES
      include/libbsa/result.hpp
      include/libbsa/archive.hpp
      include/libbsa/detect.hpp
      include/libbsa/io.hpp)

target_include_directories(libbsa
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(libbsa
  PRIVATE
    ${LIBBSA_LIBDEFLATE_TARGET}
    ${LIBBSA_LZ4_TARGET}
    ${LIBBSA_DIRECTXTEX_TARGET})

install(TARGETS libbsa
  EXPORT libbsaTargets
  FILE_SET public_headers)

install(EXPORT libbsaTargets
  NAMESPACE libbsa::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libbsa)

if(BUILD_TESTING)
  find_package(Catch2 CONFIG REQUIRED)

  if(Catch2_DIR)
    list(APPEND CMAKE_MODULE_PATH "${Catch2_DIR}/../..")
  endif()

  include(Catch)

  add_executable(libbsa_foundation_tests tests/foundation_tests.cpp)
  target_link_libraries(libbsa_foundation_tests
    PRIVATE
      libbsa::libbsa
      Catch2::Catch2WithMain)

  catch_discover_tests(libbsa_foundation_tests PROPERTIES LABELS "unit")

  add_executable(libbsa_io_tests tests/io_tests.cpp)
  target_link_libraries(libbsa_io_tests
    PRIVATE
      libbsa::libbsa
      Catch2::Catch2WithMain)

  catch_discover_tests(libbsa_io_tests TEST_PREFIX "libbsa_io_tests." PROPERTIES LABELS "unit")

  add_executable(libbsa_detection_tests tests/detection_tests.cpp)
  target_link_libraries(libbsa_detection_tests
    PRIVATE
      libbsa::libbsa
      Catch2::Catch2WithMain)

  catch_discover_tests(libbsa_detection_tests TEST_PREFIX "libbsa_detection_tests." PROPERTIES LABELS "unit")

  add_executable(libbsa_public_header_smoke tests/public_header_smoke.cpp)
  target_link_libraries(libbsa_public_header_smoke
    PRIVATE
      libbsa::libbsa)

  add_test(NAME libbsa.public_header_smoke COMMAND libbsa_public_header_smoke)
  set_tests_properties(libbsa.public_header_smoke PROPERTIES LABELS smoke)
endif()
