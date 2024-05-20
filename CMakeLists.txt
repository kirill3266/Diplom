cmake_minimum_required(VERSION 3.27)
project(Diplom)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -v")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

find_library(fftw fftw3 REQUIRED)

find_library(hackrf hackrf REQUIRED)

find_package(Poco REQUIRED Net NetSSL Foundation Util Data)

include_directories(include src res)

add_executable(Diplom main.cpp
        src/RBUData.cpp
        include/RBUData.h
        include/ConcurrentQueue.h)

target_link_libraries(Diplom PRIVATE ${fftw} ${hackrf} ${Poco_LIBRARIES})