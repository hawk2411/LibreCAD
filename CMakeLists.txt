cmake_minimum_required(VERSION 3.15)
project(librecad)

set(CMAKE_CXX_STANDARD 17)      #for using std::filesystem::temp_directory_path

add_subdirectory(libraries)
add_subdirectory(librecad)
# add_subdirectory(plugins)
add_subdirectory(tools)

