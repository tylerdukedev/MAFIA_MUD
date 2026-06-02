cmake_minimum_required(VERSION 3.20)
project(CapitalVice VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
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

# --- Main executable ---
set(CAPITAL_VICE_SOURCES
    src/main.cpp
    src/core/application.cpp
    src/core/sim_clock.cpp
    src/sim/system_registry.cpp
    src/procgen/world_generator.cpp
    src/utils/seed_hash.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
    src/ui/dock_layout.cpp
    src/ui/map_camera.cpp
    src/ui/map_renderer.cpp
    src/ui/game_ui.cpp
)

add_executable(capital_vice ${CAPITAL_VICE_SOURCES})
target_include_directories(capital_vice PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(capital_vice PRIVATE imgui_lib OpenGL::GL ${CMAKE_DL_LIBS})

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

add_executable(world_generator_test
    test/world_generator_test.cpp
    src/procgen/world_generator.cpp
    src/utils/seed_hash.cpp
    src/world/world_config.cpp
    src/world/chunk_store.cpp
    src/world/region_table.cpp
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
