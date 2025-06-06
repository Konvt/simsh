# This file is licensed under the MIT License.
# Please see the LICENSE file in the root of the repository for the full license text.
# Copyright (c) 2024-2025 Konvt

cmake_minimum_required(VERSION 3.15.0)
project(format LANGUAGES CXX)

add_custom_target(format)

add_executable(simsh "")
set_target_properties(simsh PROPERTIES OUTPUT_NAME "simsh")
set_target_properties(simsh PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.")

target_compile_options(simsh PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-DNDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:-DNDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:-Wall>
  $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
  $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
  $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>)
set_target_properties(simsh PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(simsh PRIVATE cxx_std_20)

target_include_directories(simsh PRIVATE "${CMAKE_SOURCE_DIR}/inc/")
file(GLOB_RECURSE SIMSH_SRC
  ${CMAKE_SOURCE_DIR}/src/*.cpp)
target_sources(simsh PRIVATE ${SIMSH_SRC})
