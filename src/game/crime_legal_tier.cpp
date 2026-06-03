#include "game/crime_legal_tier.h"

namespace Core {

const char* crimeLegalTierToString(CrimeLegalTier tier) {
    switch (tier) {
    case CrimeLegalTier::PettyStreet:
        return "Petty street";
    case CrimeLegalTier::Street:
        return "Street";
    case CrimeLegalTier::Organization:
        return "Organization";
    case CrimeLegalTier::Financial:
        return "Financial";
    default:
        return "Unknown";
    }
}

bool meetsCrimeLegalTierGate(CrimeLegalTier requiredTier, CrimeLegalTier playerMaxTier) {
    return static_cast<int32_t>(playerMaxTier) >= static_cast<int32_t>(requiredTier);
}

} // namespace Core
