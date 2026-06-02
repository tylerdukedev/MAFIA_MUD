#pragma once

#include <cstdint>

namespace Core {

enum class NationalityId : uint8_t {
    None = 0,
    American,
    Italian,
    Irish,
    Chinese,
    Mexican,
    Russian,
    Greek,
    Polish,
    Cuban,
    Canadian,
    British,
    French,
    COUNT
};

enum class HeritageId : uint8_t {
    None = 0,
    Italian,
    Sicilian,
    Irish,
    Jewish,
    Chinese,
    Polish,
    Russian,
    Greek,
    AfricanAmerican,
    Mexican,
    COUNT
};

enum class GenerationId : uint8_t {
    None = 0,
    Immigrant,
    FirstGeneration,
    SecondGeneration,
    ThirdGenerationPlus,
    COUNT
};

enum class BackgroundId : uint8_t {
    None = 0,
    StreetHustler,
    NeighborhoodOrganizer,
    Bookkeeper,
    COUNT
};

constexpr int32_t CHARACTER_CREATION_MIN_AGE = 16;
constexpr int32_t CHARACTER_CREATION_MAX_AGE = 25;

} // namespace Core
