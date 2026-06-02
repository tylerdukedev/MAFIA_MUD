#pragma once

#include "core/types.h"
#include "ui/map_camera.h"
#include "world/chunk_store.h"
#include "world/district_store.h"
#include "world/world_config.h"
#include "imgui.h"

namespace Core {

constexpr float DISTRICT_HEATMAP_MAX_PIXELS_PER_TILE = 8.0f;

void renderMapTiles(
    ImDrawList* drawList,
    const MapCamera& camera,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize);

void renderDistrictHeatmap(
    ImDrawList* drawList,
    const MapCamera& camera,
    const WorldConfig& worldConfig,
    const DistrictStore& districtStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize);

ImU32 getTileColor(RegionId regionId, TerrainId terrainId, int16_t elevation);

} // namespace Core
