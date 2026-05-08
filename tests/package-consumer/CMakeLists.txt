cmake_minimum_required(VERSION 3.24)

project(libbsa_package_consumer LANGUAGES CXX)

find_package(libbsa CONFIG REQUIRED)

add_executable(libbsa_package_consumer main.cpp)
target_compile_features(libbsa_package_consumer PRIVATE cxx_std_20)
target_link_libraries(libbsa_package_consumer PRIVATE libbsa::libbsa)

# Keep the consumer test runnable for shared installs without relying on source-tree paths.
add_custom_command(TARGET libbsa_package_consumer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:libbsa_package_consumer> $<TARGET_RUNTIME_DLLS:libbsa_package_consumer>
  COMMAND_EXPAND_LISTS
)

enable_testing()
add_test(NAME libbsa_package_consumer_run COMMAND libbsa_package_consumer)
