# This file is licensed under the MIT License.
# Please see the LICENSE file in the root of the repository for the full license text.
# Copyright (c) 2024-2025 Konvt

cmake_minimum_required(VERSION 3.8.0)
project(tish LANGUAGES CXX)

add_executable(tish "")
set_target_properties(tish PROPERTIES OUTPUT_NAME "tish")
set_target_properties(tish PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(tish PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(tish PRIVATE -fsanitize=address)
    target_link_options(tish PRIVATE -fsanitize=address)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(tish PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/W4>
    $<$<COMPILE_LANGUAGE:CXX>:/permissive->)
else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}. No warning flags set.")
endif()

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
    "${dir}/*.hxx")
  list(APPEND FORMAT_FILES ${TMP_FILES})
endforeach()

add_custom_target(format
  COMMAND ${CMAKE_COMMAND} -E echo "Formatting source files with clang-format..."
  COMMAND clang-format -i -style=file ${FORMAT_FILES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Running clang-format on all source files in src/ and inc/")
