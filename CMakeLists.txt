cmake_minimum_required(VERSION 3.20)
project(librecad)
include(settings.cmake)
set(CMAKE_CXX_STANDARD 17)      #for using std::filesystem::temp_directory_path

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS ON)

add_subdirectory(libraries)
add_subdirectory(librecad)
add_subdirectory(plugins)
add_subdirectory(tools)

