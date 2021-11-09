set(VERSION 1.0.1)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(../../librecad/src/plugins)

add_library(sameprop SHARED
        sameprop.cpp
)

target_link_libraries(sameprop
        PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )
