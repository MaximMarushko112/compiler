cmake_minimum_required(VERSION 3.15)
project(C-lite-compiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/Tokenization/Tokenizer.cpp
    src/Parsing/Parser.cpp
    src/Parsing/PrintAST.cpp
    src/Visitors/Interpreter.cpp
    src/Visitors/ScopeVisitor.cpp
    src/Visitors/PrintVisitor.cpp
    src/main.cpp
)

add_executable(c_lite_test ${SOURCES})