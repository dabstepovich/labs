cmake_minimum_required(VERSION 3.16)
project(FuelPrices LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

option(ENABLE_COVERAGE "Enable test coverage" OFF)

add_library(fuel_model_lib STATIC
    fuel_model.cpp
    fuel_model.h
    fuel_price.h
    parse_exception.h
)
target_link_libraries(fuel_model_lib PUBLIC Qt6::Core)

if(ENABLE_COVERAGE)
    target_compile_options(fuel_model_lib PRIVATE --coverage -O0 -g)
    target_link_options(fuel_model_lib PRIVATE --coverage)

    target_compile_options(TestFuelModel PRIVATE --coverage -O0 -g)
    target_link_options(TestFuelModel PRIVATE --coverage)

    target_link_options(FuelPrices PRIVATE --coverage)
endif()

add_executable(FuelPrices
    main.cpp
    main_window.cpp
    main_window.h
)
target_link_libraries(FuelPrices PRIVATE fuel_model_lib Qt6::Widgets)

enable_testing()

add_executable(TestFuelModel
    test_fuel_model.cpp
)
target_link_libraries(TestFuelModel PRIVATE fuel_model_lib Qt6::Test)

add_test(NAME TestFuelModel COMMAND TestFuelModel)
