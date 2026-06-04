#include "game/police_contacts.h"
#include "utils/seed_hash.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace Core {

namespace {

constexpr const char* OFFICER_FIRST_NAMES[] = {
    "M.", "J.", "R.", "T.", "D.", "F.", "C.", "A.", "L.", "P."
};
constexpr int32_t OFFICER_FIRST_NAME_COUNT = 10;

constexpr const char* OFFICER_LAST_NAMES[] = {
    "Russo", "Malone", "Torres", "Burke", "Chen", "Williams",
    "Flanagan", "Ortiz", "DiMaggio", "Kowalski", "Washington",
    "Reyes", "Shea", "Greco", "Okafor"
};
constexpr int32_t OFFICER_LAST_NAME_COUNT = 15;

} // namespace

void resetPoliceContactStore(PoliceContactStore& store) {
    store = PoliceContactStore{};
}

int32_t generatePoliceOfficer(
    PoliceContactStore& store,
    const char* officerName,
    const char* precinct,
    PoliceRank rank,
    int32_t corruptibility,
    uint64_t tickCount) {
    if (store.activeCount >= MAX_POLICE_CONTACTS) {
        return -1;
    }
    const int32_t slot = store.activeCount++;
    PoliceContactState& contact = store.contacts[slot];
    contact.isActive = true;
    contact.rank = rank;
    contact.corruptibility = std::clamp(corruptibility, 0, 100);
    contact.opinionOfPlayer = -10;
    contact.encounterCount = 1;
    contact.lastEncounterTick = tickCount;
    contact.isOnPayroll = false;
    if (officerName != nullptr) {
        std::strncpy(contact.officerName, officerName, sizeof(contact.officerName) - 1);
    }
    if (precinct != nullptr) {
        std::strncpy(contact.precinct, precinct, sizeof(contact.precinct) - 1);
    }
    return slot;
}

int32_t findPoliceContactByName(const PoliceContactStore& store, const char* officerName) {
    if (officerName == nullptr) {
        return -1;
    }
    for (int32_t i = 0; i < store.activeCount; ++i) {
        if (store.contacts[i].isActive && std::strncmp(store.contacts[i].officerName, officerName, 32) == 0) {
            return i;
        }
    }
    return -1;
}

void recordPoliceEncounter(PoliceContactStore& store, int32_t contactIndex, uint64_t tickCount) {
    if (contactIndex < 0 || contactIndex >= MAX_POLICE_CONTACTS) {
        return;
    }
    PoliceContactState& contact = store.contacts[contactIndex];
    contact.encounterCount += 1;
    contact.lastEncounterTick = tickCount;
    contact.opinionOfPlayer = std::clamp(contact.opinionOfPlayer - 3, OFFICER_OPINION_MIN, OFFICER_OPINION_MAX);
}

void adjustOfficerOpinion(PoliceContactStore& store, int32_t contactIndex, int32_t delta) {
    if (contactIndex < 0 || contactIndex >= MAX_POLICE_CONTACTS) {
        return;
    }
    PoliceContactState& contact = store.contacts[contactIndex];
    contact.opinionOfPlayer = std::clamp(contact.opinionOfPlayer + delta, OFFICER_OPINION_MIN, OFFICER_OPINION_MAX);
}

bool tryBribeOfficer(PoliceContactStore& store, int32_t contactIndex, uint64_t worldSeed, uint64_t tickCount) {
    if (contactIndex < 0 || contactIndex >= MAX_POLICE_CONTACTS) {
        return false;
    }
    PoliceContactState& contact = store.contacts[contactIndex];
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), contactIndex) % 100U;
    const bool accepted = static_cast<int32_t>(roll) < contact.corruptibility;
    if (accepted) {
        contact.bribeAcceptedCount += 1;
        adjustOfficerOpinion(store, contactIndex, 15);
    } else {
        adjustOfficerOpinion(store, contactIndex, -5);
    }
    return accepted;
}

int64_t computeOfficerBribeCostCents(const PoliceContactState& officer) {
    const int64_t rankScale = static_cast<int64_t>(officer.rank) * OFFICER_BRIBE_RANK_SCALE_CENTS;
    return static_cast<int64_t>(OFFICER_BRIBE_BASE_COST_CENTS) + rankScale;
}

const PoliceContactState* getPoliceContact(const PoliceContactStore& store, int32_t contactIndex) {
    if (contactIndex < 0 || contactIndex >= MAX_POLICE_CONTACTS) {
        return nullptr;
    }
    return &store.contacts[contactIndex];
}

const char* policeRankToString(PoliceRank rank) {
    switch (rank) {
        case PoliceRank::Officer:    return "Officer";
        case PoliceRank::Detective:  return "Detective";
        case PoliceRank::Sergeant:   return "Sergeant";
        case PoliceRank::Lieutenant: return "Lieutenant";
    }
    return "Officer";
}

void generateOfficerName(char* outBuffer, int32_t bufferSize, uint64_t worldSeed, uint64_t tickCount, int32_t contactIndex) {
    const uint32_t firstHash = Utils::hashSeedMix(worldSeed, contactIndex, 0x464E414D) % static_cast<uint32_t>(OFFICER_FIRST_NAME_COUNT);
    const uint32_t lastHash = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), contactIndex + 0x4C4E414D) % static_cast<uint32_t>(OFFICER_LAST_NAME_COUNT);
    std::snprintf(outBuffer, bufferSize, "%s %s", OFFICER_FIRST_NAMES[firstHash], OFFICER_LAST_NAMES[lastHash]);
}

void generatePrecinctName(char* outBuffer, int32_t bufferSize, uint8_t regionId) {
    switch (regionId) {
        case 1: std::snprintf(outBuffer, bufferSize, "Midtown South Precinct"); return;
        case 2: std::snprintf(outBuffer, bufferSize, "Brooklyn North Precinct"); return;
        case 3: std::snprintf(outBuffer, bufferSize, "Queens South Precinct"); return;
        case 4: std::snprintf(outBuffer, bufferSize, "Bronx NYPD — 42nd Pct"); return;
        case 5: std::snprintf(outBuffer, bufferSize, "SI NYPD — 120th Pct"); return;
        case 6: std::snprintf(outBuffer, bufferSize, "NJSP — Hudson County"); return;
        default: std::snprintf(outBuffer, bufferSize, "NYPD Central Precinct"); return;
    }
}

void generateBadgeNumber(char* outBuffer, int32_t bufferSize, uint64_t worldSeed, int32_t contactIndex) {
    const uint32_t badge = 1000U + (Utils::hashSeedMix(worldSeed, contactIndex, 0x42444745) % 8999U);
    std::snprintf(outBuffer, bufferSize, "#%u", badge);
}

int32_t computeOfficerCorruptibility(PoliceRank rank, uint64_t worldSeed, int32_t contactIndex) {
    const uint32_t baseRoll = Utils::hashSeedMix(worldSeed, contactIndex, 0x43525054) % 70U;
    const int32_t rankPenalty = static_cast<int32_t>(rank) * 10;
    return static_cast<int32_t>(baseRoll + 15U) - rankPenalty;
}

} // namespace Core
