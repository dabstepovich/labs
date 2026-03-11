cmake_minimum_required(VERSION 3.16)
project(FuelPrices LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

option(ENABLE_COVERAGE "Enable test coverage (GCC/Clang)" OFF)

add_library(fuel_logic_lib STATIC
    fuel_model.cpp
    fuel_model.h
    command_processor.cpp
    command_processor.h
    fuel_price.h
    parse_exception.h
)
target_link_libraries(fuel_logic_lib PUBLIC Qt6::Core)

if(ENABLE_COVERAGE)
    target_compile_options(fuel_logic_lib PRIVATE --coverage -O0 -g)
    target_link_options(fuel_logic_lib  PRIVATE --coverage)
endif()

add_executable(FuelPrices
    main.cpp
    main_window.cpp
    main_window.h
)
target_link_libraries(FuelPrices PRIVATE fuel_logic_lib Qt6::Widgets)

enable_testing()

add_executable(TestAll
    test_all.cpp
)
target_link_libraries(TestAll PRIVATE fuel_logic_lib Qt6::Test)

if(ENABLE_COVERAGE)
    target_compile_options(TestAll PRIVATE --coverage -O0 -g)
    target_link_options(TestAll    PRIVATE --coverage)
endif()

add_test(NAME TestAll COMMAND TestAll)
