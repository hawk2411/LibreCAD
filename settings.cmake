find_program(TRANSLATOR NAMES lrelease)
if(NOT TRANSLATOR)
    message(FATAL_ERROR "Qt translator 'lrelease' not found")
endif()

# TODO is APPLE really among to UNIX
if(UNIX)
    if(APPLE)
        message("This ins apple")
#        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/apple/lib)
#        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/apple/lib)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/apple/bin)
    else(APPLE)
        message("This is unix")
#        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/unix/lib)
#        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/unix/lib)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/unix/bin)
    endif(APPLE)
#    add_compile_options(-Wall -Wextra -pedantic -Werror)
    add_compile_options(-Wall -Wextra -pedantic)
else(UNIX)
    message("This is windows")
#    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/windows/lib)
#    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/windows/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/windows/bin)
#    add_compile_options(/W4 /WX)
    add_compile_options(/W4)
    add_compile_definitions(_USE_MATH_DEFINES)
endif(UNIX)

# Windows compiler settings
#win32 {
#QMAKE_CXXFLAGS += -U__STRICT_ANSI__
#QMAKE_CFLAGS_THREAD -= -mthreads
#QMAKE_CXXFLAGS_THREAD -= -mthreads
#QMAKE_LFLAGS_THREAD -= -mthreads
##qt version check for mingw
#win32-g++ {
#contains(QT_VERSION, ^4\\.8\\.[0-4]) {
#DEFINES += QT_NO_CONCURRENT=0
#}
## Silence warning: typedef '...' locally defined but not used [-Wunused-local-typedefs]
## this was caused by boost headers and g++ 4.8.0 (Qt 5.1 / MinGW 4.8)
#greaterThan( QT_MAJOR_VERSION, 4 ) {
#QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
#}
#}else{
#!build_pass:verbose:message(Setting up support for MSVC.)
## define the M_PI etc macros for MSVC compilers.


#
## The .NET 2003 compiler (at least) is touchy about its own headers ...
#win32-msvc2003 {
## Silence "unused formal parameter" warnings about unused `_Iosbase`
## in the header file `xloctime` (a Vc7 header after all!).
#QMAKE_CXXFLAGS += /wd4100
#}
#}
#
#unix|macx|win32-g++ {
## no such option for MSVC
#QMAKE_CXXFLAGS_DEBUG += -g
#QMAKE_CXXFLAGS += -g
#}
#
## fix for GitHub Issue #880
## prevent QMake from using -isystem flag for system include path
## this breaks gcc 6 builds because of its #include_next feature
#QMAKE_CFLAGS_ISYSTEM = ""
#
## svg support
#QT += svg
#
#greaterThan( QT_MAJOR_VERSION, 4) {
#CONFIG += c++11
#}else{
#unix|macx|win32-g++ {
## no such option for MSVC
#QMAKE_CXXFLAGS += -std=c++11
#QMAKE_CXXFLAGS_DEBUG += -std=c++11
#}
#}
