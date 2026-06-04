#pragma once

#include "game/crime_legal_tier.h"
#include <cstdint>

namespace Core {

enum class ChargeType : uint8_t {
    // Misdemeanors
    Disorderly = 0,
    PettyTheft = 1,
    Trespassing = 2,
    Vandalism = 3,
    // Felonies
    Assault = 4,
    AggravatedAssault = 5,
    Robbery = 6,
    Extortion = 7,
    Racketeering = 8,
    MurderSecondDegree = 9,
    MurderFirstDegree = 10,
    // Financial
    Embezzlement = 11,
    MoneyLaundering = 12,
    TaxEvasion = 13,
};

constexpr int32_t CHARGE_TYPE_COUNT = 14;

enum class ChargeOutcome : uint8_t {
    Pending = 0,
    Dismissed = 1,
    PleaBargain = 2,
    GuiltyVerdict = 3,
    Acquitted = 4,
};

constexpr int32_t MAX_CRIMINAL_CHARGES = 32;

struct CriminalCharge {
    ChargeType chargeType = ChargeType::Disorderly;
    ChargeOutcome outcome = ChargeOutcome::Pending;
    CrimeLegalTier legalTier = CrimeLegalTier::PettyStreet;
    uint64_t arrestTick = 0;
    uint64_t resolutionTick = 0;
    char officerName[32]{};
    char jurisdictionLabel[40]{};
};

struct CriminalRecordStore {
    CriminalCharge charges[MAX_CRIMINAL_CHARGES]{};
    int32_t chargeCount = 0;
    int32_t convictionCount = 0;
    int32_t priorFelonyCount = 0;
    int32_t pendingChargeCount = 0;
};

void resetCriminalRecordStore(CriminalRecordStore& store);
int32_t addCriminalCharge(
    CriminalRecordStore& store,
    ChargeType chargeType,
    CrimeLegalTier legalTier,
    uint64_t arrestTick,
    const char* officerName,
    const char* jurisdictionLabel);
void resolveLatestCharge(
    CriminalRecordStore& store,
    ChargeOutcome outcome,
    uint64_t resolutionTick);
const CriminalCharge* getLatestPendingCharge(const CriminalRecordStore& store);
const CriminalCharge* getCriminalCharge(const CriminalRecordStore& store, int32_t chargeIndex);
ChargeType chargeTypeFromLegalTier(CrimeLegalTier tier, uint64_t worldSeed, uint64_t tickCount);
const char* chargeTypeToString(ChargeType chargeType);
const char* chargeOutcomeToString(ChargeOutcome outcome);
const char* chargeTypeToStatuteLabel(ChargeType chargeType);
bool isFelonyCharge(ChargeType chargeType);
const char* jurisdictionLabelFromRegionId(uint8_t regionId);

} // namespace Core
