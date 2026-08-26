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
    src/deusto/watertankDeustoNoTemperature.cpp
    src/deusto/wiperDeusto.cpp
    src/deusto/wiper2BitDeusto.cpp
    src/rhlab/butterfly.cpp
    src/rhlab/airportWindStation.cpp
    src/rhlab/matrix.cpp
    src/rhlab/morse.cpp
)

include(CTest)
if(BUILD_TESTING)
    add_executable(airport_wind_station_tests
        tests/airport_wind_station_tests.cpp
        src/labsland/simulations/targetdevice.cpp
        src/rhlab/airportWindStation.cpp
    )
    target_include_directories(airport_wind_station_tests PRIVATE src)
    add_test(NAME airport_wind_station_tests COMMAND airport_wind_station_tests)

    add_test(NAME embedded_manifest_airport_wind_station
        COMMAND ${CMAKE_COMMAND}
            -DMANIFEST=${CMAKE_CURRENT_SOURCE_DIR}/simulations.cmake
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/embedded_manifest_test.cmake
    )

    add_test(NAME public_simulation_catalog
        COMMAND ${CMAKE_COMMAND}
            -DMANIFEST=${CMAKE_CURRENT_SOURCE_DIR}/simulations.cmake
            -DREADME=${CMAKE_CURRENT_SOURCE_DIR}/README.md
            -DRUNNER=${CMAKE_CURRENT_SOURCE_DIR}/src-stdcpp/main.cpp
            -DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/public_simulation_catalog_test.cmake
    )
endif()
