cmake_minimum_required(VERSION 3.16)

# Ensure the compiler has a writable temporary directory even inside
# sandboxed environments (e.g., WSL mounting Windows paths read-only).
set(_stack_array_tmp "${CMAKE_BINARY_DIR}/tmp")
file(MAKE_DIRECTORY "${_stack_array_tmp}")
set(ENV{TMPDIR} "${_stack_array_tmp}")
set(ENV{TMP} "${_stack_array_tmp}")
set(ENV{TEMP} "${_stack_array_tmp}")

project(stack_array
    VERSION 1.0.0
    DESCRIPTION "Fixed-capacity stack arrays and strings for C"
    LANGUAGES C
)

include(GNUInstallDirs)
include(CTest)

option(STACK_ARRAY_BUILD_TESTS "Build stack_array tests" ON)

add_library(stack_array STATIC
    stack_array.c
)

add_library(stack_array::stack_array ALIAS stack_array)

target_include_directories(stack_array
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(stack_array PUBLIC c_std_11)

if(MSVC)
    target_compile_options(stack_array PRIVATE /W4)
else()
    target_compile_options(stack_array PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING AND STACK_ARRAY_BUILD_TESTS)
    add_executable(stack_array_tests
        tests/test_stack_array.c
        tests/main.c
    )

    target_link_libraries(stack_array_tests PRIVATE stack_array)
    target_include_directories(stack_array_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_compile_features(stack_array_tests PRIVATE c_std_11)

    if(MSVC)
        target_compile_options(stack_array_tests PRIVATE /W4)
    else()
        target_compile_options(stack_array_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME stack_array_tests COMMAND stack_array_tests)
endif()

install(TARGETS stack_array
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    stack_array.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
