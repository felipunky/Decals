cmake_minimum_required(VERSION 3.0...3.25)

project(
    DECALS 
    VERSION 0.0.1 
    DESCRIPTION "Different decal rendering techniques"
)

# add_definitions(-std=c++17)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_PATH "${PROJECT_SOURCE_DIR}/src")
set(INCLUDE_PATH "${PROJECT_SOURCE_DIR}/include")

# file(GLOB_RECURSE HS "${INCLUDE_PATH}/*.h")
# file(GLOB_RECURSE CPPS "${SOURCE_PATH}/*.cpp")

if(DEFINED EMSCRIPTEN)
    message(STATUS "This is an EMSCRIPTEN build!")

    add_executable(calc 
    "${SOURCE_PATH}/calc.cpp"
    "${INCLUDE_PATH}/calc.h"
    )

    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")

    set_target_properties(calc PROPERTIES COMPILE_FLAGS "-Os -s SIDE_MODULE=1 ")
	set_target_properties(calc PROPERTIES LINK_FLAGS    "-Os -s WASM=1 -s SIDE_MODULE=1 -s STANDALONE_WASM --no-entry")

else()
    message(STATUS "This is not an EMSCRIPTEN build!")

    if(WIN32)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
        set(BUILD_SHARED_LIBS TRUE)
    endif()

    add_library(calc SHARED
    "${SOURCE_PATH}/calc.cpp"
    "${INCLUDE_PATH}/calc.h"
    )

    add_executable(App 
        "${INCLUDE_PATH}/calc.h"
        "${SOURCE_PATH}/main.cpp"
    )

    target_include_directories(App PUBLIC
        ${INCLUDE_PATH}
    )

    target_link_libraries(App PUBLIC calc)
endif()

target_include_directories(calc PUBLIC 
    ${INCLUDE_PATH}
)