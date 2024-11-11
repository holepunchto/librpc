cmake_minimum_required(VERSION 3.25)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(rpc C)

fetch_package("github:holepunchto/libcompact")

add_library(rpc OBJECT)

set_target_properties(
  rpc
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  rpc
  INTERFACE
    include/rpc.h
  PRIVATE
    src/rpc.c
)

target_include_directories(
  rpc
  PUBLIC
    include
)

target_link_libraries(
  rpc
  PUBLIC
    utf
    compact
)

add_library(rpc_shared SHARED)

set_target_properties(
  rpc_shared
  PROPERTIES
  OUTPUT_NAME rpc
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  rpc_shared
  PUBLIC
    rpc
    utf_shared
    compact_shared
)

add_library(rpc_static STATIC)

set_target_properties(
  rpc_static
  PROPERTIES
  OUTPUT_NAME rpc
  PREFIX lib
)

target_link_libraries(
  rpc_static
  PUBLIC
    rpc
    utf_static
    compact_static
)

install(TARGETS rpc_shared rpc_static)

install(FILES include/rpc.h DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
