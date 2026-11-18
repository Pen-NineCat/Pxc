# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

# Build Diagnostic error code table
# find_package() resolves the interpreter on every platform: python.exe on Windows, python3 on Linux.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
execute_process(
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Diagnostic/GenerateDiagnosticTable.py
	RESULT_VARIABLE generateResult
)
if(NOT generateResult EQUAL 0)
	# A stale DiagnosticCodeTable.hpp still compiles and silently misses the new diagnostic codes.
	message(FATAL_ERROR "GenerateDiagnosticTable.py failed with exit code ${generateResult}, check that PyYAML is installed for ${Python3_EXECUTABLE}.")
endif()

set(PLANG_CORE_SOURCES
	"Diagnostic/Diagnostic.hpp"
	"Diagnostic/DiagnosticCodeTable.hpp"
	"Common/Environment.hpp"
	"Common/SourceView.hpp"
	"Lex/Keywords.hpp"
	"Lex/Lexer.h"
	"Lex/Lexer.cpp"
	"Lex/Token.hpp"
)

set(PLANG_CLI_SOURCES
	"CLI/PLang.cpp"
)

find_package(LLVM CONFIG REQUIRED)

add_library(PLang-Core ${PLANG_CORE_SOURCES})
target_link_libraries(PLang-Core PRIVATE Pxc-Common)

add_executable(PLang-CLI ${PLANG_CLI_SOURCES})
target_link_libraries(PLang-CLI PRIVATE PLang-Core)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(PLang-Core PRIVATE /permissive-)
endif()

# llvm dependency workaround
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})
target_include_directories(PLang-Core PRIVATE ${LLVM_INCLUDE_DIRS})

target_include_directories(PLang-Core PUBLIC ${CMAKE_SOURCE_DIR}/Code)
