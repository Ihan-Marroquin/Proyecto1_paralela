cmake_minimum_required(VERSION 3.18)

project(AuroraParalela VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(aurora_saver
    src/main.cpp
    src/AppConfig.cpp
    src/AuroraSimulation.cpp
    src/AuroraRenderer.cpp
)

target_include_directories(aurora_saver PRIVATE include)
target_link_libraries(aurora_saver PRIVATE OpenMP::OpenMP_CXX)

if(WIN32)
    target_compile_definitions(aurora_saver PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(aurora_saver PRIVATE gdi32 user32)
endif()

if(MSVC)
    target_compile_options(aurora_saver PRIVATE /W4 /permissive-)
else()
    target_compile_options(aurora_saver PRIVATE -Wall -Wextra -Wpedantic)
endif()
