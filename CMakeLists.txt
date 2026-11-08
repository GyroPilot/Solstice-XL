# Solstice for the classic Mac: build with Retro68 (68000 code, Plus and up, System 6 and 7).
cmake_minimum_required(VERSION 3.10)
project(Solstice C)
add_application(Solstice
    src/main.c
    src/astro.c
    src/worldmap.c
    src/splash.c
    src/facts.c
    src/pictures.c
    src/solstice.r
    TYPE "APPL"
    CREATOR "SlSt"
    )
target_include_directories(Solstice PRIVATE src)
target_link_libraries(Solstice m)
set_target_properties(Solstice PROPERTIES COMPILE_OPTIONS "-ffunction-sections;-Os;-Wall;-Wno-multichar;-Wno-pointer-sign;-Wno-array-bounds")
set_target_properties(Solstice PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
