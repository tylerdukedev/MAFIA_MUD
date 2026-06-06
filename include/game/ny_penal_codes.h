#pragma once

#include "game/crime_legal_tier.h"
#include <cstdint>

namespace Core {

struct NyPenalCodeEntry {
    CrimeLegalTier legalTier = CrimeLegalTier::Street;
    const char* statuteCode;
    const char* shortTitle;
    const char* classification;
    int32_t minSentenceMonths;
    int32_t maxSentenceMonths;
};

const NyPenalCodeEntry* getNyPenalCodeForTier(CrimeLegalTier legalTier);
const char* getNyPenalStatuteCode(CrimeLegalTier legalTier);
const char* getNyPenalShortTitle(CrimeLegalTier legalTier);
const char* getNyPenalClassification(CrimeLegalTier legalTier);

} // namespace Core
