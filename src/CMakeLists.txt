cmake_minimum_required(VERSION 3.20)
project(allocator LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(
  main.cpp
  arena.cpp
  extent_manager.cpp
  super_block.cpp
  lock_free_stack.cpp
  alloc_route.cpp
  alloc_init.cpp
  huge_allocation.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(allocator PRIVATE os_api_linux.cpp)
  target_link_libraries(allocator PRIVATE numa)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_sources(allocator PRIVATE os_api_windows.cpp)
endif()
