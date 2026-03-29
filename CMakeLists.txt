cmake_minimum_required(VERSION 3.16)
project(SimEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COVERAGE "Build with coverage instrumentation and GoogleTest/GoogleMock unit tests" OFF)

set(SIM_ENG_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Wunused-result
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
)

add_executable(sim_eng
    src/main.cpp
)

target_compile_options(sim_eng PRIVATE ${SIM_ENG_WARNING_FLAGS})

target_include_directories(sim_eng PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/event
)

if(COVERAGE)
    include(CTest)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_library(coverage_flags INTERFACE)
        target_compile_options(coverage_flags INTERFACE -O0 -g --coverage)
        target_link_options(coverage_flags INTERFACE --coverage)
    else()
        message(WARNING "COVERAGE is ON, but compiler '${CMAKE_CXX_COMPILER_ID}' may not support --coverage flags.")
    endif()

    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        find_package(GTest MODULE QUIET)
    endif()

    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(sim_eng_tests
        tests/src/event_bus_tests.cpp
        tests/src/movement_system_tests.cpp
        tests/src/logger_tests.cpp
    )

    target_include_directories(sim_eng_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/event
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/include
    )

    target_compile_options(sim_eng_tests PRIVATE ${SIM_ENG_WARNING_FLAGS})

    if(TARGET coverage_flags)
        target_link_libraries(sim_eng_tests PRIVATE coverage_flags)
    endif()

    target_link_libraries(sim_eng_tests PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(sim_eng_tests)

    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)
    find_program(GCOVR_EXECUTABLE gcovr)

    string(REGEX MATCH "^[0-9]+" CXX_COMPILER_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(GCOV_TOOL_EXECUTABLE
        NAMES
            gcov-${CMAKE_CXX_COMPILER_VERSION}
            gcov-${CXX_COMPILER_MAJOR}
            gcov
    )

    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        if(NOT GCOV_TOOL_EXECUTABLE)
            message(FATAL_ERROR "lcov/genhtml were found but no gcov executable was found. Install gcov matching your compiler.")
        endif()

        add_custom_target(coverage_html
            COMMAND ${CMAKE_COMMAND} -E rm -f coverage.base.info coverage.test.info coverage.all.info coverage.filtered.info
            COMMAND ${CMAKE_COMMAND} -E remove_directory coverage_html
            COMMAND ${LCOV_EXECUTABLE} --gcov-tool ${GCOV_TOOL_EXECUTABLE} --capture --initial --directory . --output-file coverage.base.info
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND $<TARGET_FILE:sim_eng_tests>
            COMMAND ${LCOV_EXECUTABLE} --gcov-tool ${GCOV_TOOL_EXECUTABLE} --capture --directory . --output-file coverage.test.info
            COMMAND ${LCOV_EXECUTABLE} --gcov-tool ${GCOV_TOOL_EXECUTABLE} --add-tracefile coverage.base.info --add-tracefile coverage.test.info --output-file coverage.all.info
            COMMAND ${LCOV_EXECUTABLE} --gcov-tool ${GCOV_TOOL_EXECUTABLE} --remove coverage.all.info '/usr/*' '${CMAKE_BINARY_DIR}/_deps/*' '${CMAKE_SOURCE_DIR}/tests/*' --output-file coverage.filtered.info
            COMMAND ${GENHTML_EXECUTABLE} coverage.filtered.info --output-directory coverage_html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS sim_eng_tests
            COMMENT "Running tests and generating HTML coverage report with lcov/genhtml"
        )
    elseif(GCOVR_EXECUTABLE)
        add_custom_target(coverage_html
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${GCOVR_EXECUTABLE} --root ${CMAKE_SOURCE_DIR} --html-details --output coverage_html/index.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS sim_eng_tests
            COMMENT "Running tests and generating HTML coverage report with gcovr"
        )
    else()
        message(WARNING "coverage_html target was not created because neither lcov/genhtml nor gcovr were found.")
    endif()
endif()
