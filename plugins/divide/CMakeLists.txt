set(VERSION 0.0.1)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(../../librecad/src/plugins)

add_library(divide SHARED
        divide.cpp
        dividedlg.cpp
)

target_link_libraries(divide
        PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )
set_target_properties(
        divide
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${output_directory}
        RUNTIME_OUTPUT_DIRECTORY ${output_directory}
)