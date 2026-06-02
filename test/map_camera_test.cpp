#include "ui/map_camera.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace Core;

TEST_CASE("MapCamera screen/world conversion round-trips", "[map][camera]") {
    MapCamera camera;
    camera.centerWorldX = 256.0f;
    camera.centerWorldY = 256.0f;
    camera.pixelsPerTile = 2.0f;
    constexpr float canvasOriginX = 100.0f;
    constexpr float canvasOriginY = 50.0f;
    constexpr float canvasWidth = 800.0f;
    constexpr float canvasHeight = 600.0f;
    float inputWorldX = 300.0f;
    float inputWorldY = 280.0f;
    float screenX = 0.0f;
    float screenY = 0.0f;
    camera.tileToScreen(inputWorldX, inputWorldY, canvasOriginX, canvasOriginY, canvasWidth, canvasHeight, screenX, screenY);
    float actualWorldX = 0.0f;
    float actualWorldY = 0.0f;
    camera.screenToWorld(screenX, screenY, canvasOriginX, canvasOriginY, canvasWidth, canvasHeight, actualWorldX, actualWorldY);
    REQUIRE(std::abs(actualWorldX - inputWorldX) < 0.001f);
    REQUIRE(std::abs(actualWorldY - inputWorldY) < 0.001f);
}

TEST_CASE("MapCamera fitToWorld sets reasonable zoom", "[map][camera]") {
    MapCamera camera;
    WorldConfig config;
    camera.fitToWorld(config.WORLD_WIDTH_TILES, config.WORLD_HEIGHT_TILES, 1024.0f, 768.0f);
    REQUIRE(camera.pixelsPerTile > 0.0f);
    REQUIRE(camera.centerWorldX > 0.0f);
    REQUIRE(camera.centerWorldY > 0.0f);
}

TEST_CASE("MapCamera zoomAt respects limits", "[map][camera]") {
    MapCamera camera;
    camera.pixelsPerTile = 1.0f;
    camera.zoomAt(1000.0f, 10.0f, 10.0f);
    REQUIRE(camera.pixelsPerTile <= 48.0f);
    camera.zoomAt(0.001f, 10.0f, 10.0f);
    REQUIRE(camera.pixelsPerTile >= 0.25f);
}
