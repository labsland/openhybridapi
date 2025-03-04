cmake_minimum_required(VERSION 3.17)
project(hybridapi)

set(CMAKE_CXX_STANDARD 14)

add_compile_options( -I../src )

add_executable(hybridapi 
    src-stdcpp/main.cpp 
    src-stdcpp/labsland/utils/timemanagerstd.cpp
    src-stdcpp/labsland/simulations/targetdevicefiles.cpp
    src-stdcpp/labsland/protocols/i2ciowrapperfiles.cpp

    src/labsland/simulations/targetdevice.cpp
    src/labsland/simulations/watertanksimulation.cpp
    src/deusto/door.cpp
    src/deusto/watertankDeusto.cpp
    src/rhlab/butterfly.cpp

)
