cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0074 NEW)

project(cpp_pyquboc)

include(external/cimod.cmake)
include(external/eigen.cmake)
include(external/pybind11.cmake)
include(external/robin_hood.cmake)

find_package(Boost REQUIRED)

pybind11_add_module(cpp_pyquboc src/main.cpp)

target_compile_definitions(cpp_pyquboc PRIVATE VERSION_INFO=${PYQUBOC_VERSION_INFO})
target_compile_features(cpp_pyquboc PRIVATE cxx_std_17)
target_compile_options(cpp_pyquboc PRIVATE
    $<$<CXX_COMPILER_ID:GNU>: -Ofast -Wall -Wno-terminate>
    $<$<CXX_COMPILER_ID:AppleClang>: -Ofast -Wno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>: /Ot /wd4297>
)
target_include_directories(cpp_pyquboc PRIVATE ${Boost_INCLUDE_DIRS})
