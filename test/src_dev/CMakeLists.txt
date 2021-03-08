cmake_minimum_required(VERSION 2.6.0)

project(test C)

include_directories(../include)

add_library(task1 task1.c)
add_library(task2 task2.c)

add_executable(test main.c)
