cmake_minimum_required(VERSION 3.6)
project(tinyos_5)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
    bios.c
    bios.h
    bios_example1.c
    bios_example2.c
    bios_example3.c
    bios_example4.c
    bios_example5.c
    console.c
    kernel_cc.c
    kernel_cc.h
    kernel_dev.c
    kernel_dev.h
    kernel_init.c
    kernel_pipe.c
    kernel_pipe.h
    kernel_proc.c
    kernel_proc.h
    kernel_sched.c
    kernel_sched.h
    kernel_socket.c
    kernel_socket.h
    kernel_streams.c
    kernel_streams.h
    kernel_threads.c
    mtask.c
    symposium.c
    symposium.h
    terminal.c
    test_example.c
    test_util.c
    tinyos.h
    tinyos_shell.c
    tinyoslib.c
    tinyoslib.h
    unit_testing.c
    unit_testing.h
    util.c
    util.h
    validate_api.c)

add_executable(tinyos_5 ${SOURCE_FILES})