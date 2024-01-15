cmake_minimum_required(VERSION 2.8)
project(network_stack)

add_compile_options(-g -O2 -march=native -fomit-frame-pointer -std=c11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -Wformat=2
)

set(CMAKE_C_STANDARD 11)
set(NIC_LIB nic)
set(TARGET_APP pcap)
add_library(${NIC_LIB} STATIC src/driver/pci.c)

add_executable(${TARGET_APP} src/app/${TARGET_APP}.c)
target_link_libraries(${TARGET_APP} PUBLIC ${NIC_LIB})
target_include_directories(${TARGET_APP} PUBLIC "src/")
