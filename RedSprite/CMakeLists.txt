cmake_minimum_required(VERSION 3.15)
project(RedSprite VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -ffast-math")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(MSVC)
    add_compile_options(/O2 /arch:AVX2)
endif()

# Source files
set(SOURCES
    src/core/bitboard.cpp
    src/core/position.cpp
    src/movegen/move.cpp
    src/eval/nnue.cpp
    src/ttable/tt.cpp
    src/search/search.cpp
    src/uci/uci.cpp
)

set(HEADERS
    src/core/types.h
    src/core/bitboard.h
    src/core/position.h
    src/movegen/move.h
    src/eval/nnue.h
    src/ttable/tt.h
    src/search/search.h
    src/uci/uci.h
)

# Main executable
add_executable(redsprite ${SOURCES} ${HEADERS})

target_include_directories(redsprite PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/movegen
    ${CMAKE_SOURCE_DIR}/src/eval
    ${CMAKE_SOURCE_DIR}/src/ttable
    ${CMAKE_SOURCE_DIR}/src/search
    ${CMAKE_SOURCE_DIR}/src/uci
)

# Threading support
find_package(Threads REQUIRED)
target_link_libraries(redsprite PRIVATE Threads::Threads)

# SIMD optimizations (optional)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(redsprite PRIVATE -mavx2 -mbmi2)
    elseif(MSVC)
        target_compile_options(redsprite PRIVATE /arch:AVX2)
    endif()
endif()

# Install target
install(TARGETS redsprite DESTINATION bin)

# Testing
enable_testing()

# Perft test
add_executable(perft_test tests/perft_test.cpp ${SOURCES})
target_include_directories(perft_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/movegen
    ${CMAKE_SOURCE_DIR}/src/eval
    ${CMAKE_SOURCE_DIR}/src/ttable
    ${CMAKE_SOURCE_DIR}/src/search
    ${CMAKE_SOURCE_DIR}/src/uci
)
target_link_libraries(perft_test PRIVATE Threads::Threads)
