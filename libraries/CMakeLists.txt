add_subdirectory(libdxfrw)
add_subdirectory(jwwlib)

find_package(muparser QUIET)
option(build_muparser "Boolean flag to build muparser." NO)

if(WIN32 OR APPLE OR ${build_muparser} OR (NOT ${muparser_FOUND}))
  message(STATUS "Using bundled muparser lib")
  add_subdirectory(muparser)
else()
  message(STATUS "Using external muparser lib")
endif()
