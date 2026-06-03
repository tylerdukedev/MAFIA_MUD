cmake_minimum_required(VERSION 3.20)
project(CapitalVice VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
endif()
if(MSVC)
    add_compile_options(/utf-8)
endif()

# --- Dependencies ---
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(catch2)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui_lib STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw)

find_package(OpenGL REQUIRED)

option(CAPITALVICE_DEV_CONSOLE "Enable dev-only console with ImGui viewports" ON)

set(CHARACTER_SOURCES
    src/character/character_tables.cpp
    src/character/profile_builder.cpp
)

# --- Main executable ---
set(CAPITAL_VICE_SOURCES
    src/main.cpp
    src/core/application.cpp
    src/core/sim_clock.cpp
    src/sim/system_registry.cpp
    src/sim/borough_vitality_system.cpp
    src/procgen/world_generator.cpp
    src/utils/seed_hash.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
    src/world/landmark_table.cpp
    src/world/tile_vitality.cpp
    src/persistence/save_game.cpp
    src/ui/dock_layout.cpp
    src/ui/map_camera.cpp
    src/ui/map_renderer.cpp
    src/ui/landmark_renderer.cpp
    src/ui/help_manual.cpp
    src/ui/context_help.cpp
    src/ui/game_ui.cpp
    ${CHARACTER_SOURCES}
)

if(CAPITALVICE_DEV_CONSOLE)
    list(APPEND CAPITAL_VICE_SOURCES src/dev/dev_console.cpp)
endif()

add_executable(capital_vice ${CAPITAL_VICE_SOURCES})
target_include_directories(capital_vice PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(capital_vice PRIVATE imgui_lib OpenGL::GL ${CMAKE_DL_LIBS})
if(CAPITALVICE_DEV_CONSOLE)
    target_compile_definitions(capital_vice PRIVATE CAPITALVICE_DEV_CONSOLE=1)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(capital_vice PRIVATE m)
endif()

# --- Tests ---
enable_testing()
add_executable(chunk_store_test
    test/chunk_store_test.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
)
target_include_directories(chunk_store_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(chunk_store_test PRIVATE Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(chunk_store_test)

add_executable(landmark_table_test
    test/landmark_table_test.cpp
    src/world/landmark_table.cpp
)
target_include_directories(landmark_table_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(landmark_table_test PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(landmark_table_test)

add_executable(world_generator_test
    test/world_generator_test.cpp
    src/procgen/world_generator.cpp
    src/utils/seed_hash.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
    src/world/tile_vitality.cpp
    src/world/landmark_table.cpp
)
target_include_directories(world_generator_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(world_generator_test PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(world_generator_test)

add_executable(map_camera_test
    test/map_camera_test.cpp
    src/ui/map_camera.cpp
)
target_include_directories(map_camera_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(map_camera_test PRIVATE Catch2::Catch2WithMain imgui_lib)
catch_discover_tests(map_camera_test)

add_executable(save_game_test
    test/save_game_test.cpp
    src/persistence/save_game.cpp
    src/procgen/world_generator.cpp
    src/utils/seed_hash.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
    src/world/tile_vitality.cpp
    src/world/landmark_table.cpp
    src/core/sim_clock.cpp
)
target_include_directories(save_game_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(save_game_test PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(save_game_test)

add_executable(profile_builder_test
    test/profile_builder_test.cpp
    ${CHARACTER_SOURCES}
)
target_include_directories(profile_builder_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(profile_builder_test PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(profile_builder_test)

add_executable(dev_console_test
    test/dev_console_test.cpp
    src/dev/dev_console.cpp
    ${CHARACTER_SOURCES}
)
target_include_directories(dev_console_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dev_console_test PRIVATE Catch2::Catch2WithMain imgui_lib)
catch_discover_tests(dev_console_test)

add_executable(tile_vitality_test
    test/tile_vitality_test.cpp
    src/world/tile_vitality.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
    src/world/landmark_table.cpp
    src/utils/seed_hash.cpp
)
target_include_directories(tile_vitality_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tile_vitality_test PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(tile_vitality_test)
