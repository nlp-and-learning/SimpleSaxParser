cmake_minimum_required(VERSION 3.12)
project(parsertest)

set(CMAKE_CXX_STANDARD 14)

if(CMAKE_COMPILER_IS_GNUCXX)
    SET(GCC_COVERAGE_COMPILE_FLAGS "-Werror=return-type")
    add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
else()
    add_definitions(-DUSE64bit=true)
endif()

add_executable(parsertest SaxParserTest.cpp SaxFormatter.cpp SaxParser.cpp)