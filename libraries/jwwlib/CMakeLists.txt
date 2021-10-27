set(CMAKE_CXX_STANDARD 17)

set(DLL_NAME "jwwlib")

include_directories(src)

add_library(${DLL_NAME} STATIC
  src/dl_writer_ascii.cpp
    src/dl_jww.cpp
    src/jwwdoc.cpp
)
