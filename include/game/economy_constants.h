#pragma once

#include <cstdint>

namespace Core {

constexpr int64_t CENTS_PER_DOLLAR = 100;
constexpr int64_t STARTING_CASH_MIN_DOLLARS = 0;
constexpr int64_t STARTING_CASH_MAX_DOLLARS = 25;
constexpr int64_t DEFAULT_CLAIM_CITY_COST_CENTS = 250000;
constexpr int64_t STREET_HUSTLER_CLAIM_COST_CENTS = 225000;
constexpr int64_t BROKE_CASH_THRESHOLD_CENTS = 200;
constexpr int32_t ECONOMY_INCOME_APPLY_INTERVAL_TICKS = 20;
constexpr float LEGIT_INCOME_BASE_CENTS_PER_TICK = 0.12f;
constexpr float CRIME_INCOME_BASE_CENTS_PER_TICK = 0.85f;
constexpr float IMMIGRANT_CRIME_INCOME_SCALE = 1.25f;
constexpr float BOOKKEEPER_LEGIT_INCOME_SCALE = 1.35f;
constexpr float STREET_HUSTLER_CRIME_INCOME_SCALE = 1.20f;
constexpr uint8_t PLAYER_OWNER_ID = 1;
constexpr uint8_t RIVAL_OWNER_ID = 2;
constexpr uint8_t CITY_OWNER_NONE = 0;

enum class WalletDeltaKind : uint8_t {
    None = 0,
    Loss = 1,
    GainLegit = 2,
    GainCrime = 3,
};

} // namespace Core
