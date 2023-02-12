set(CMAKE_CXX_STANDARD 17)

set(DLL_NAME "muparser")
include_directories(include)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(${DLL_NAME}
  src/muParserTokenReader.cpp
    src/muParserError.cpp 
    src/muParserCallback.cpp 
    src/muParserBytecode.cpp 
    src/muParserBase.cpp 
    src/muParser.cpp)

