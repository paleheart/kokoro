# CMakeLists.txt - Kokoro compiler

cmake_minimum_required(VERSION 3.10)
project(kokoro LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add executable
add_executable(kokoro
    kokoro.c
)

# Add warning flags
# Add warning flags per platform
if (MSVC)
    target_compile_options(kokoro PRIVATE /W4)
else()
    target_compile_options(kokoro PRIVATE -Wall -Wextra)
endif()
