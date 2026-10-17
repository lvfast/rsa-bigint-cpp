cmake_minimum_required(VERSION 3.16)
project(rsa_cpp_lab LANGUAGES CXX)

# Build the original lab programs as three independent executables.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(primality_check src/primality/main.cpp)
add_executable(rsa_private_exponent src/private_exponent/main.cpp)
add_executable(modular_exponentiation src/modular_exponentiation/main.cpp)
