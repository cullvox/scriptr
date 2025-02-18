cmake_minimum_required(VERSION 3.20)

project(scriptr)

option(CPM_USE_LOCAL_PACKAGES ON)
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.4
    VERSION 3.2.4
    OPTIONS "SDL_TEST OFF" "SDL_SHARED OFF" "SDL_STATIC ON" "SDL2_DISABLE_UNINSTALL ON"
)

CPMAddPackage(
    NAME POCO
    GIT_REPOSITORY https://github.com/pocoproject/poco.git
    GIT_TAG poco-1.14.0-release
    VERSION 1.14.0
    OPTIONS "ENABLE_PDF ON"
)

CPMAddPackage(
  NAME freetype
  GIT_REPOSITORY https://github.com/freetype/freetype
  GIT_TAG VER-2-13-3
  VERSION 2.13.3
)

CPMAddPackage(
  NAME plutosvg
  GIT_REPOSITORY https://github.com/sammycage/plutosvg
  GIT_TAG v0.0.4
  VERSION 0.0.4
)

CPMAddPackage("gh:nlohmann/json@3.11.3")

if (freetype_ADDED)
  add_library(Freetype::Freetype ALIAS freetype)
endif()

if(POCO_ADDED) 
  add_library(POCO INTERFACE IMPORTED)
  target_include_directories(POCO INTERFACE "${POCO_SOURCE_DIR}/include")
endif()

add_executable(Scriptr
    "source/imgui/imgui_demo.cpp"
    "source/imgui/imgui_draw.cpp"
    "source/imgui/imgui_impl_opengl3.cpp"
    "source/imgui/imgui_impl_sdl3.cpp"
    "source/imgui/imgui_tables.cpp"
    "source/imgui/imgui_widgets.cpp"
    "source/imgui/imgui.cpp"
    "source/imgui/imgui_stdlib.cpp"
    "source/imgui/misc/freetype/imgui_freetype.cpp"
    "source/imgui/imnodes.cpp"

    "source/glad/src/gl.c"

    "source/RichTextDocument.cpp"
    "source/RichTextEditor.cpp"
    "source/node.cpp"
    "source/main.cpp")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

target_compile_features(Scriptr PUBLIC cxx_std_17)
target_link_libraries(Scriptr PUBLIC SDL3::SDL3 freetype Poco::Foundation nlohmann_json plutosvg)
target_include_directories(Scriptr PRIVATE "source/glad/include" "source/imgui/")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)