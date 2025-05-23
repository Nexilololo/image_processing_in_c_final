cmake_minimum_required(VERSION 3.30)
project(image_processing_in_c_final C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(image_processing_in_c_final main.c
        bmp24.h
        bmp8.h
        bmp8.c
        bmp24.c)
