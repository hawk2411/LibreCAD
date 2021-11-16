find_package(freetype REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(ttf2lff main.cpp)
target_link_libraries(ttf2lff freetype)