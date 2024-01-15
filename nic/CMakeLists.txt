cmake_minimum_required(VERSION 2.8)
project(network_stack
        LANGUAGES C CXX)

add_compile_options(-g -O2 -march=native -fomit-frame-pointer -std=c++17
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -Wformat=2
)

set(CMAKE_CXX_STANDARD                  17)
set(CMAKE_CXX_STANDARD_REQUIRED         ON)
set(CMAKE_C_STANDARD 11)
set(NIC_LIB nic)
set(TARGET_APP pcap)
add_library(${NIC_LIB} STATIC src/pci.cc src/driver/dev.cc)
target_include_directories(${NIC_LIB} PUBLIC "src/")

add_executable(${TARGET_APP} src/app/${TARGET_APP}.cc)
target_link_libraries(${TARGET_APP} PUBLIC ${NIC_LIB})
target_include_directories(${TARGET_APP} PUBLIC "src/")
