cmake_minimum_required(VERSION 3.15)
project(librecad)
include(settings.cmake)
set(CMAKE_CXX_STANDARD 17)      #for using std::filesystem::temp_directory_path

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(libraries)
add_subdirectory(librecad)
add_subdirectory(plugins)
add_subdirectory(tools)

