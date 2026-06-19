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

# Don't build FTXUI's own examples/tests
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui)

# ── Executable ───────────────────────────────────────────────────
add_executable(tamagotchi tamagotchi-cplt-2.cpp)

target_link_libraries(tamagotchi
    PRIVATE
        ftxui::component   # ScreenInteractive, CatchEvent, …
        ftxui::dom         # Elements, Nodes
        ftxui::screen      # Color, Screen
)