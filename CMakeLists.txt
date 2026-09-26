cmake_minimum_required(VERSION 3.16)
project(vaccination_pthreads VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_THREAD_SANITIZER "Enable ThreadSanitizer" OFF)

if(ENABLE_SANITIZERS AND ENABLE_THREAD_SANITIZER)
    message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be enabled together")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    elseif(ENABLE_THREAD_SANITIZER)
        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread)
    endif()
endif()

find_package(Threads REQUIRED)

add_executable(practica2 practica2.c)
target_link_libraries(practica2 PRIVATE Threads::Threads)

add_executable(benchmark benchmark.c)
target_compile_definitions(benchmark PRIVATE _POSIX_C_SOURCE=200809L)
target_link_libraries(benchmark PRIVATE Threads::Threads)

include(CTest)
if(BUILD_TESTING)
    foreach(thread_count 1 2 4 8)
        set(target practica2_threads_${thread_count})
        add_executable(${target} practica2.c)
        target_compile_definitions(${target} PRIVATE FABRICAS=${thread_count})
        target_link_libraries(${target} PRIVATE Threads::Threads)
        add_test(
            NAME simulation_${thread_count}_factory_threads
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_simulation.sh
                $<TARGET_FILE:${target}>
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/valid-17.txt
                ${CMAKE_CURRENT_BINARY_DIR}/result-${thread_count}.txt
                17
        )
    endforeach()
    add_test(
        NAME simulation_edge_one_resident
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_simulation.sh
            $<TARGET_FILE:practica2>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/edge-one.txt
            ${CMAKE_CURRENT_BINARY_DIR}/result-edge.txt
            1
    )
    add_test(
        NAME invalid_extra_input
        COMMAND practica2
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/invalid-extra-field.txt
            ${CMAKE_CURRENT_BINARY_DIR}/invalid-output.txt
    )
    set_tests_properties(invalid_extra_input PROPERTIES WILL_FAIL TRUE)
    add_test(NAME benchmark_smoke COMMAND benchmark --threads 2 --items 10000 --rounds 2)
endif()
