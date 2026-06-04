#include "game/criminal_record.h"
#include "utils/seed_hash.h"
#include <cstring>
#include <algorithm>

namespace Core {

void resetCriminalRecordStore(CriminalRecordStore& store) {
    store = CriminalRecordStore{};
}

int32_t addCriminalCharge(
    CriminalRecordStore& store,
    ChargeType chargeType,
    CrimeLegalTier legalTier,
    uint64_t arrestTick,
    const char* officerName,
    const char* jurisdictionLabel) {
    if (store.chargeCount >= MAX_CRIMINAL_CHARGES) {
        return -1;
    }
    CriminalCharge& charge = store.charges[store.chargeCount];
    charge.chargeType = chargeType;
    charge.outcome = ChargeOutcome::Pending;
    charge.legalTier = legalTier;
    charge.arrestTick = arrestTick;
    charge.resolutionTick = 0;
    if (officerName != nullptr) {
        std::strncpy(charge.officerName, officerName, sizeof(charge.officerName) - 1);
    }
    if (jurisdictionLabel != nullptr) {
        std::strncpy(charge.jurisdictionLabel, jurisdictionLabel, sizeof(charge.jurisdictionLabel) - 1);
    }
    store.pendingChargeCount += 1;
    return store.chargeCount++;
}

void resolveLatestCharge(
    CriminalRecordStore& store,
    ChargeOutcome outcome,
    uint64_t resolutionTick) {
    for (int32_t i = store.chargeCount - 1; i >= 0; --i) {
        if (store.charges[i].outcome == ChargeOutcome::Pending) {
            store.charges[i].outcome = outcome;
            store.charges[i].resolutionTick = resolutionTick;
            store.pendingChargeCount = std::max(0, store.pendingChargeCount - 1);
            if (outcome == ChargeOutcome::GuiltyVerdict || outcome == ChargeOutcome::PleaBargain) {
                store.convictionCount += 1;
                if (isFelonyCharge(store.charges[i].chargeType)) {
                    store.priorFelonyCount += 1;
                }
            }
            return;
        }
    }
}

const CriminalCharge* getLatestPendingCharge(const CriminalRecordStore& store) {
    for (int32_t i = store.chargeCount - 1; i >= 0; --i) {
        if (store.charges[i].outcome == ChargeOutcome::Pending) {
            return &store.charges[i];
        }
    }
    return nullptr;
}

const CriminalCharge* getCriminalCharge(const CriminalRecordStore& store, int32_t chargeIndex) {
    if (chargeIndex < 0 || chargeIndex >= store.chargeCount) {
        return nullptr;
    }
    return &store.charges[chargeIndex];
}

ChargeType chargeTypeFromLegalTier(CrimeLegalTier tier, uint64_t worldSeed, uint64_t tickCount) {
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), 0x4348524745U) % 3U;
    switch (tier) {
        case CrimeLegalTier::PettyStreet:
            if (roll == 0) { return ChargeType::Disorderly; }
            if (roll == 1) { return ChargeType::PettyTheft; }
            return ChargeType::Vandalism;
        case CrimeLegalTier::Street:
            if (roll == 0) { return ChargeType::Assault; }
            if (roll == 1) { return ChargeType::Robbery; }
            return ChargeType::Trespassing;
        case CrimeLegalTier::Organization:
            if (roll == 0) { return ChargeType::Extortion; }
            if (roll == 1) { return ChargeType::Racketeering; }
            return ChargeType::AggravatedAssault;
        case CrimeLegalTier::Financial:
            if (roll == 0) { return ChargeType::Embezzlement; }
            if (roll == 1) { return ChargeType::MoneyLaundering; }
            return ChargeType::TaxEvasion;
    }
    return ChargeType::Disorderly;
}

const char* chargeTypeToString(ChargeType chargeType) {
    switch (chargeType) {
        case ChargeType::Disorderly:         return "Disorderly Conduct";
        case ChargeType::PettyTheft:         return "Petty Theft";
        case ChargeType::Trespassing:        return "Criminal Trespass";
        case ChargeType::Vandalism:          return "Criminal Mischief";
        case ChargeType::Assault:            return "Assault, 3rd Degree";
        case ChargeType::AggravatedAssault:  return "Assault, 2nd Degree";
        case ChargeType::Robbery:            return "Robbery, 2nd Degree";
        case ChargeType::Extortion:          return "Grand Larceny / Extortion";
        case ChargeType::Racketeering:       return "Enterprise Corruption";
        case ChargeType::MurderSecondDegree: return "Murder, 2nd Degree";
        case ChargeType::MurderFirstDegree:  return "Murder, 1st Degree";
        case ChargeType::Embezzlement:       return "Grand Larceny / Embezzlement";
        case ChargeType::MoneyLaundering:    return "Money Laundering";
        case ChargeType::TaxEvasion:         return "Criminal Tax Fraud";
    }
    return "Unknown Charge";
}

const char* chargeTypeToStatuteLabel(ChargeType chargeType) {
    switch (chargeType) {
        case ChargeType::Disorderly:         return "NYPL §240.20";
        case ChargeType::PettyTheft:         return "NYPL §155.25";
        case ChargeType::Trespassing:        return "NYPL §140.10";
        case ChargeType::Vandalism:          return "NYPL §145.00";
        case ChargeType::Assault:            return "NYPL §120.00";
        case ChargeType::AggravatedAssault:  return "NYPL §120.05";
        case ChargeType::Robbery:            return "NYPL §160.10";
        case ChargeType::Extortion:          return "NYPL §155.40";
        case ChargeType::Racketeering:       return "NYPL §460.20";
        case ChargeType::MurderSecondDegree: return "NYPL §125.25";
        case ChargeType::MurderFirstDegree:  return "NYPL §125.27";
        case ChargeType::Embezzlement:       return "NYPL §155.42";
        case ChargeType::MoneyLaundering:    return "NYPL §470.20";
        case ChargeType::TaxEvasion:         return "NY Tax Law §1801";
    }
    return "NYPL §000.00";
}

const char* chargeOutcomeToString(ChargeOutcome outcome) {
    switch (outcome) {
        case ChargeOutcome::Pending:       return "Pending";
        case ChargeOutcome::Dismissed:     return "Dismissed";
        case ChargeOutcome::PleaBargain:   return "Plea Bargain";
        case ChargeOutcome::GuiltyVerdict: return "Guilty";
        case ChargeOutcome::Acquitted:     return "Acquitted";
    }
    return "Unknown";
}

bool isFelonyCharge(ChargeType chargeType) {
    switch (chargeType) {
        case ChargeType::AggravatedAssault:
        case ChargeType::Robbery:
        case ChargeType::Extortion:
        case ChargeType::Racketeering:
        case ChargeType::MurderSecondDegree:
        case ChargeType::MurderFirstDegree:
        case ChargeType::Embezzlement:
        case ChargeType::MoneyLaundering:
        case ChargeType::TaxEvasion:
            return true;
        default:
            return false;
    }
}

const char* jurisdictionLabelFromRegionId(uint8_t regionId) {
    switch (regionId) {
        case 1: return "Criminal Court of the City of New York — Manhattan";
        case 2: return "Kings County Criminal Court — Brooklyn";
        case 3: return "Queens County Criminal Court";
        case 4: return "Bronx County Criminal Court";
        case 5: return "Richmond County Criminal Court — Staten Island";
        case 6: return "Superior Court of New Jersey";
        default: return "Criminal Court of the City of New York — Manhattan";
    }
}

} // namespace Core
