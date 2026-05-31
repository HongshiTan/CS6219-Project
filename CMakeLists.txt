cmake_minimum_required(VERSION 3.20)
project(dnarng)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# START: CONFIG MBEDTLS ##########################################

set(ENABLE_PROGRAMS OFF)
set(ENABLE_TESTING OFF)
add_subdirectory(3rd-party/mbedtls-3.2.1)

# END: CONFIG MBEDTLS ############################################


include_directories(3rd-party/TestU01/include)
link_directories(3rd-party/TestU01/lib)

find_package(OpenMP REQUIRED)

add_executable(rbg src/main.cxx)
target_compile_options(rbg PRIVATE -O3 -g)
target_link_libraries(rbg mbedcrypto)


add_executable(rbg_test src/test_main.cxx)
target_compile_options(rbg_test PRIVATE -O3 -g)
target_link_libraries(rbg_test mbedcrypto testu01 probdist mylib OpenMP::OpenMP_CXX)

add_executable(rbg_test_nosha src/test_main_nosha.cxx)
target_compile_options(rbg_test_nosha PRIVATE -O3 -g)
target_link_libraries(rbg_test_nosha mbedcrypto testu01 probdist mylib OpenMP::OpenMP_CXX)

add_executable(rbg_test_toeplitz src/test_main_toeplitz.cxx)
target_compile_options(rbg_test_toeplitz PRIVATE -O3 -g)
target_link_libraries(rbg_test_toeplitz mbedcrypto testu01 probdist mylib OpenMP::OpenMP_CXX)

add_custom_target(
  run
  $<TARGET_FILE:rbg>
  DEPENDS rbg)
