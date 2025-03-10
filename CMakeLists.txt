cmake_minimum_required( VERSION 3.5 )

project( http-server )

set(CMAKE_CXX_COMPILER "/usr/bin/gcc")
add_compile_options(-Wall -Wextra)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable( server src/main.c src/str.c src/http/parser_helpers.c src/http/headers.c src/http/status.c src/fs.c src/http/content-type.c src/http_thread.c )
