cmake_minimum_required(VERSION 3.20)

project(scriptr)

option(CPM_USE_LOCAL_PACKAGES ON)
include(cmake/CPM.cmake)

CPMAddPackage("gh:libsdl-org/SDL#release-2.30.11")

add_executable(Scriptr
    "source/imgui_demo.cpp"
    "source/imgui_draw.cpp"
    "source/imgui_impl_opengl3.cpp"
    "source/imgui_impl_sdl2.cpp"
    "source/imgui_tables.cpp"
    "source/imgui_widgets.cpp"
    "source/imgui.cpp"

    "source/imnodes.cpp"

    "source/glad/src/gl.c"

    "source/main.cpp")

target_link_libraries(Scriptr PUBLIC SDL2::SDL2)
target_include_directories(Scriptr PRIVATE "source/glad/include")
