# This file is licensed under the MIT License.
# Please see the LICENSE file in the root of the repository for the full license text.
# Copyright (c) 2024-2025 Konvt

cmake_minimum_required(VERSION 3.15.0)
project(tish LANGUAGES CXX)

add_executable(tish "")
set_target_properties(tish PROPERTIES OUTPUT_NAME "tish")
set_target_properties(tish PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.")

target_compile_options(tish PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-DNDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:-DNDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:-Wall>
  $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
  $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
  $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>)
set_target_properties(tish PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(tish PRIVATE cxx_std_20)

target_include_directories(tish PRIVATE "${CMAKE_SOURCE_DIR}/inc/")
file(GLOB_RECURSE TISH_SRC
  ${CMAKE_SOURCE_DIR}/src/*.cpp)
target_sources(tish PRIVATE ${TISH_SRC})

set(FORMAT_DIRS
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/inc")
  set(FORMAT_FILES "")
foreach(dir IN LISTS FORMAT_DIRS)
  file(GLOB_RECURSE TMP_FILES
    "${dir}/*.cpp"
    "${dir}/*.hpp"
    "${dir}/*.cxx"
    "${dir}/*.hxx"
    "${dir}/*.c"
    "${dir}/*.h")
  list(APPEND FORMAT_FILES ${TMP_FILES})
endforeach()

add_custom_target(format
  COMMAND ${CMAKE_COMMAND} -E echo "Formatting source files with clang-format..."
  COMMAND clang-format -i -style=file ${FORMAT_FILES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Running clang-format on all source files in src/ and inc/")
