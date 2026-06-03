#pragma once

#include <cstdint>

namespace Core {

enum class GameDockZone : uint8_t {
    Center = 0,
    Left = 1,
    Right = 2,
    Bottom = 3,
};

struct GameDockPanelDef {
    const char* windowTitle;
    GameDockZone zone;
};

namespace GameDockPanel {
constexpr const char* Simulation = "Simulation";
constexpr const char* Character = "Character";
constexpr const char* Operations = "Operations";
constexpr const char* Boroughs = "Boroughs";
constexpr const char* Contacts = "Contacts";
constexpr const char* City = "City";
constexpr const char* Business = "Business";
constexpr const char* TileInspector = "Tile Inspector";
constexpr const char* MapViewport = "Map Viewport";
} // namespace GameDockPanel

constexpr GameDockPanelDef GAME_DOCK_PANEL_DEFINITIONS[] = {
    {GameDockPanel::TileInspector, GameDockZone::Left},
    {GameDockPanel::Simulation, GameDockZone::Left},
    {GameDockPanel::Character, GameDockZone::Left},
    {GameDockPanel::Operations, GameDockZone::Left},
    {GameDockPanel::Boroughs, GameDockZone::Right},
    {GameDockPanel::Contacts, GameDockZone::Right},
    {GameDockPanel::City, GameDockZone::Bottom},
    {GameDockPanel::Business, GameDockZone::Bottom},
    {GameDockPanel::MapViewport, GameDockZone::Center},
};

constexpr int32_t GAME_DOCK_PANEL_DEFINITION_COUNT =
    static_cast<int32_t>(sizeof(GAME_DOCK_PANEL_DEFINITIONS) / sizeof(GAME_DOCK_PANEL_DEFINITIONS[0]));

} // namespace Core
