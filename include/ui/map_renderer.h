#pragma once

#include "core/types.h"
#include "ui/map_camera.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include "imgui.h"

namespace Core {

void renderMapTiles(
    ImDrawList* drawList,
    const MapCamera& camera,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const WorldCoord* hoveredTileCoord = nullptr);

ImU32 getTileColor(RegionId regionId, TerrainId terrainId, int16_t elevation);

} // namespace Core
