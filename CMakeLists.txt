cmake_minimum_required(VERSION 3.16)
project(FJP_SP)
SET(GCC_FPERMISSIVE_FLAGS "-fpermissive")
set(CMAKE_CXX_STANDARD 17)
find_package(FLEX)
find_package(BISON)

flex_target(flex token.l ${CMAKE_CURRENT_BINARY_DIR}/token.cpp)

bison_target(bison
        grammar.y ${CMAKE_BINARY_DIR}/grammar.y.cpp
        DEFINES_FILE ${CMAKE_BINARY_DIR}/grammar.tab.h)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_FPERMISSIVE_FLAGS}")
add_definitions(${GCC_FPERMISSIVE_FLAGS})
add_executable(FJP_SP main.cpp  ${FLEX_flex_OUTPUTS} ${BISON_bison_OUTPUTS} grammar.y token.l file.h synt_tree.cpp synt_tree.h method.h code_block.h value.cpp value.h method.cpp)