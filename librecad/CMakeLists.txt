add_subdirectory(src)

configure_file(support/librecad.appdata.xml.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/appdata/librecad.appdata.xml ESCAPE_QUOTES)
