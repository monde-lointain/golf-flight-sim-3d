cmake_minimum_required(VERSION 3.14)
project(GolfFlightSim3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if (${FORCE_COLORED_OUTPUT})
   if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      add_compile_options (-fdiagnostics-color=always)
   elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      add_compile_options (-fcolor-diagnostics)
   endif ()
endif ()

add_subdirectory(lib)
add_subdirectory(src/Framework)
add_subdirectory(src/GolfFlightSim3D)