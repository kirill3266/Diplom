cmake_minimum_required(VERSION 3.27)
project(Diplom)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")

find_library(fftw fftw3 REQUIRED)

find_library(hackrf hackrf REQUIRED)

add_executable(Diplom main.cpp)

target_link_libraries(Diplom ${fftw} ${hackrf})