# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026 - Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

find_package(GTest CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

set(PLANG_TEST_SOURCES LexerTest.cpp)

add_executable(PLang-Tests ${PLANG_TEST_SOURCES})

target_link_libraries(PLang-Tests PRIVATE PLang-Core)
target_link_libraries(PLang-Tests PRIVATE GTest::gtest_main)
target_link_libraries(PLang-Tests PRIVATE magic_enum::magic_enum)

target_include_directories(PLang-Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(PLang-Tests DISCOVERY_MODE PRE_TEST)
