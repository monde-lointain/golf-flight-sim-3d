cmake_minimum_required(VERSION 3.14)
project(Framework)

add_subdirectory(lib)

set(sourceFiles
    Application.cpp
    MatrixStack.cpp
)

add_library(Framework ${sourceFiles})

find_package(OpenGL REQUIRED)

if("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
    target_compile_options(Framework PRIVATE /W4 /WX)
elseif("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "GNU")
    target_compile_options(Framework PRIVATE -Wall -Wextra -Werror)
endif()

if(WIN32 AND NOT MINGW)
    target_compile_definitions(Framework PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_include_directories(Framework PUBLIC include)

target_link_libraries(Framework PRIVATE glfw glad glm TracyClient spdlog imgui)
