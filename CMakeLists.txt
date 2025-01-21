cmake_minimum_required(VERSION 3.14)

project(
  kittyhook
  LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(UNICODE)
add_library(kittyhook SHARED
 src/dt.cpp
 src/mem.cpp
 src/netstuff.cpp
 src/plugin.cpp
 src/thirdparty/Zydis.c
 src/thirdparty/safetyhook.cpp
 src/thirdparty/scanner.cpp
)


include_directories(kittyhook include/ include/thirdparty/)

#target_link_libraries(fixes PUBLIC engine)