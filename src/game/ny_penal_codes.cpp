#include "game/ny_penal_codes.h"

namespace Core {

namespace {

constexpr NyPenalCodeEntry NY_PENAL_CODE_TABLE[] = {
    {CrimeLegalTier::PettyStreet, "PL 240.20", "Disorderly Conduct", "Violation", 0, 0},
    {CrimeLegalTier::Street, "PL 155.25", "Petit Larceny", "Class A Misdemeanor", 0, 12},
    {CrimeLegalTier::Organization, "PL 155.30", "Grand Larceny 4th", "Class E Felony", 12, 48},
    {CrimeLegalTier::Financial, "PL 190.65", "Scheme to Defraud", "Class E Felony", 24, 84},
};

} // namespace

const NyPenalCodeEntry* getNyPenalCodeForTier(CrimeLegalTier legalTier) {
    for (const NyPenalCodeEntry& entry : NY_PENAL_CODE_TABLE) {
        if (entry.legalTier == legalTier) {
            return &entry;
        }
    }
    return &NY_PENAL_CODE_TABLE[1];
}

const char* getNyPenalStatuteCode(CrimeLegalTier legalTier) {
    const NyPenalCodeEntry* entry = getNyPenalCodeForTier(legalTier);
    return entry != nullptr ? entry->statuteCode : "PL 240.20";
}

const char* getNyPenalShortTitle(CrimeLegalTier legalTier) {
    const NyPenalCodeEntry* entry = getNyPenalCodeForTier(legalTier);
    return entry != nullptr ? entry->shortTitle : "Disorderly Conduct";
}

const char* getNyPenalClassification(CrimeLegalTier legalTier) {
    const NyPenalCodeEntry* entry = getNyPenalCodeForTier(legalTier);
    return entry != nullptr ? entry->classification : "Violation";
}

} // namespace Core
