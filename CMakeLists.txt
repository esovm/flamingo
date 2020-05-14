cmake_minimum_required(VERSION 3.1)

if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(flamingo VERSION 0.6.0 LANGUAGES C)

## Standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

include_directories("include")

## Executable
add_executable(${PROJECT_NAME} ${SOURCES})
## Flags
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -flto)
## Source files
target_sources(${PROJECT_NAME} PRIVATE
    "src/flamingo.c" "src/type.c" "src/gc.c" "src/util.c" "src/main.c"
    "lib/libbase.c" "lib/lib.c" "lib/math/fl_math.c")
target_link_libraries(${PROJECT_NAME} m dl)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY lib/ DESTINATION $ENV{HOME}/.Flamingo/lib FILES_MATCHING PATTERN "*.fl")
