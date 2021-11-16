set(CMAKE_CXX_STANDARD 11)

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

set(DLL_NAME "dxfrw")


add_library(${DLL_NAME} STATIC
    src/libdxfrw.cpp 
    src/libdwgr.cpp
    src/drw_base.cpp
    src/drw_header.cpp 
    src/drw_classes.cpp 
    src/drw_entities.cpp 
    src/drw_objects.cpp 
    src/intern/drw_textcodec.cpp 
    src/intern/dxfreader.cpp 
    src/intern/dxfwriter.cpp 
    src/intern/dwgreader.cpp 
    src/intern/dwgbuffer.cpp 
    src/intern/drw_dbg.cpp 
    src/intern/dwgreader21.cpp 
    src/intern/dwgreader18.cpp 
    src/intern/dwgreader15.cpp 
    src/intern/dwgutil.cpp 
    src/intern/rscodec.cpp 
    src/intern/dwgreader27.cpp 
    src/intern/dwgreader24.cpp
)
target_include_directories(${DLL_NAME} PRIVATE src)
