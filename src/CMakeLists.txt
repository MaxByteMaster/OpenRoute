cmake_minimum_required(VERSION 3.5)

project(OpenRoute VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)

add_executable(OpenRoute
    main.cpp
    Location.h Location.cpp
    Projection.h Projection.cpp
    MapWidget.h MapWidget.cpp
    MapGraphicsView.h MapGraphicsView.cpp
    Map.h
    RendererProcessesManager.h RendererProcessesManager.cpp
    MapControlsWidget.h MapControlsWidget.cpp
    NavigationManager.h NavigationManager.cpp
)

target_link_libraries(OpenRoute PRIVATE Qt6::Widgets Qt6::Sql curl proj)

add_subdirectory(renderer)

set(ICON_DIR ${CMAKE_SOURCE_DIR}/../icon)
set(ICON_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/icon)

# Copy icon catalog into bin catalog
add_custom_command(TARGET OpenRoute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ICON_DIR}
    ${ICON_OUTPUT_DIR}
)
