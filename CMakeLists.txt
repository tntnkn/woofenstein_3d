cmake_minimum_required(VERSION 3.19)

if(MSVC)
else()
endif()

add_compile_definitions("$<$<CONFIG:DEBUG>:DEBUG;>")

project(
    Woofenstein_3d
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets")

add_subdirectory(src)

add_executable(main)

target_link_libraries(main 
  PRIVATE
    WOOF::GAME
)
