#pragma once

#include <cstdint>

namespace Core {

enum class CrimeLegalTier : uint8_t {
    PettyStreet = 0,
    Street = 1,
    Organization = 2,
    Financial = 3,
};

constexpr int32_t CRIME_LEGAL_TIER_COUNT = 4;

constexpr int32_t LEGAL_TIER_GATE_PETTY = 0;
constexpr int32_t LEGAL_TIER_GATE_STREET = 1;
constexpr int32_t LEGAL_TIER_GATE_ORGANIZATION = 2;
constexpr int32_t LEGAL_TIER_GATE_FINANCIAL = 3;

const char* crimeLegalTierToString(CrimeLegalTier tier);
bool meetsCrimeLegalTierGate(CrimeLegalTier requiredTier, CrimeLegalTier playerMaxTier);

} // namespace Core
