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

add_executable(fwd app/fwd.c)