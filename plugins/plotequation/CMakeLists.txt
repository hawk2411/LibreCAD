set(VERSION 1.0.1)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(plotequation SHARED plot.cpp plotdialog.cpp)

target_include_directories(plotequation PRIVATE
        ../../librecad/src/plugins
        ../../libraries/muparser/include)

target_link_libraries(plotequation
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        muparser
        )

set_target_properties(
        plotequation
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${output_directory}
        RUNTIME_OUTPUT_DIRECTORY ${output_directory}
)
