cmake_minimum_required(VERSION 3.20)
project(dnarng)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# START: CONFIG MBEDTLS ##########################################

option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(ENABLE_TESTING "Build mbed TLS tests." OFF)
option(GEN_FILES "Generate the auto-generated files as needed" ON)
add_subdirectory(3rd-party/mbedtls-3.2.1)

# END: CONFIG MBEDTLS ############################################

include_directories(3rd-party/TestU01/include)
link_directories(3rd-party/TestU01/lib)

add_executable(rbg src/main.cxx)
target_compile_options(rbg PRIVATE -Og -g)
target_link_libraries(rbg mbedcrypto)

add_executable(rbg_test src/test_main.cxx)
target_compile_options(rbg_test PRIVATE -Og -g)
target_link_libraries(rbg_test mbedcrypto testu01 probdist mylib)

add_custom_target(
  run
  $<TARGET_FILE:rbg>
  DEPENDS rbg)
