cmake_minimum_required(VERSION 3.1)

if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(flamingo VERSION 0.6.0 LANGUAGES C)

## Standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories("include")

## Source files
file(GLOB SOURCES "src/*.c" "lib/fl_aux.c")

## Executable
add_executable(${PROJECT_NAME} ${SOURCES})
## Flags
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -pedantic)
target_link_libraries(${PROJECT_NAME} PRIVATE m)
