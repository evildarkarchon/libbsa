cmake_minimum_required(VERSION 3.24)

project(libbsa_package_consumer LANGUAGES CXX)

find_package(libbsa CONFIG REQUIRED)

add_executable(libbsa_package_consumer main.cpp)
target_compile_features(libbsa_package_consumer PRIVATE cxx_std_20)
target_link_libraries(libbsa_package_consumer PRIVATE libbsa::libbsa)

enable_testing()
add_test(NAME libbsa_package_consumer_run COMMAND libbsa_package_consumer)
