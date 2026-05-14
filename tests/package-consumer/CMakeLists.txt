cmake_minimum_required(VERSION 4.0)

project(libbsa_package_consumer LANGUAGES CXX)

find_package(libbsa CONFIG REQUIRED)

add_executable(libbsa_package_consumer main.cpp)
target_compile_features(libbsa_package_consumer PRIVATE cxx_std_20)
target_link_libraries(libbsa_package_consumer PRIVATE libbsa::libbsa)

# Keep the consumer test runnable for shared installs without relying on source-tree paths.
cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH libbsa_package_consumer_compiler_dir)

add_custom_command(TARGET libbsa_package_consumer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    "-DRUNTIME_DLLS=$<TARGET_RUNTIME_DLLS:libbsa_package_consumer>"
    "-DEXTRA_RUNTIME_DLLS=${libbsa_package_consumer_compiler_dir}/clang_rt.asan_dynamic-x86_64.dll;${libbsa_package_consumer_compiler_dir}/clang_rt.asan_dbg_dynamic-x86_64.dll"
    "-DTARGET_DIR=$<TARGET_FILE_DIR:libbsa_package_consumer>"
    -P "${CMAKE_CURRENT_LIST_DIR}/copy-runtime-dlls.cmake"
  VERBATIM
)

enable_testing()
add_test(NAME libbsa_package_consumer_run COMMAND libbsa_package_consumer)
