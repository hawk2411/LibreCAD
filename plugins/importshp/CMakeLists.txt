set(VERSION 1.0.0)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(../../librecad/src/plugins shapelib)

add_library(importshp SHARED
        importshp.cpp 
        shapelib/shpopen.c
        shapelib/safileio.c
        shapelib/dbfopen.c
        )

target_link_libraries(importshp
        PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )

set_target_properties(
        importshp
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${output_directory}
        RUNTIME_OUTPUT_DIRECTORY ${output_directory}
)