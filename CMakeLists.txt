cmake_minimum_required(VERSION 3.2)

project(bbmx LANGUAGES C)

add_subdirectory(argparse)
add_subdirectory(lua)
add_subdirectory(json-c)
add_subdirectory(openal-soft)

add_executable(bbmx "src/bbmx.c" "src/main.c" "src/utils.c" "src/bbmx_lapi.c" "src/globals.c" "src/bbmxs/bbmxs.c" "src/bbmxs/serial.c" "stb/stb_vorbis.c")

target_include_directories(bbmx PUBLIC "include/" "/" "json-c/" "/openal-soft/include/")
target_link_libraries(bbmx argparse_static)
target_link_libraries(bbmx lua)
target_link_libraries(bbmx json-c)
target_link_libraries(bbmx OpenAL)