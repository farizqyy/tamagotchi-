cmake_minimum_required(VERSION 3.14)
project(tamagotchi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Fetch FTXUI ──────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v5.0.0
)

set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui)

# ── Executable ───────────────────────────────────────────────────
add_executable(tamagotchi tamagotchi-cplt-2.cpp)

target_link_libraries(tamagotchi
    PRIVATE
        ftxui::component
        ftxui::dom
        ftxui::screen
)

# ── Platform-specific settings ───────────────────────────────────
if(WIN32)
    # Force UTF-8 source and execution charset on MSVC
    if(MSVC)
        target_compile_options(tamagotchi PRIVATE /utf-8 /W3)
    endif()

    # Keep it a console app (not a GUI window) on Windows
    set_target_properties(tamagotchi PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    # Link against Windows console API libs (FTXUI needs these)
    target_link_libraries(tamagotchi PRIVATE kernel32 user32)

else()
    # GCC / Clang on Linux & macOS
    target_compile_options(tamagotchi PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()