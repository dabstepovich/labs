cmake_minimum_required(VERSION 3.16)
project(FuelPrices LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(FuelPrices
    main.cpp
    main_window.cpp
    main_window.h
    file_reader.cpp
    file_reader.h
    fuel_price.h
)

target_link_libraries(FuelPrices
    PRIVATE Qt6::Core Qt6::Widgets
)
