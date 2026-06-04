#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t MAX_POLICE_CONTACTS = 8;
constexpr int32_t OFFICER_OPINION_MIN = -100;
constexpr int32_t OFFICER_OPINION_MAX = 100;
constexpr int32_t OFFICER_BRIBE_BASE_COST_CENTS = 3000;
constexpr int32_t OFFICER_BRIBE_RANK_SCALE_CENTS = 2000;

enum class PoliceRank : uint8_t {
    Officer = 0,
    Detective = 1,
    Sergeant = 2,
    Lieutenant = 3,
};

struct PoliceContactState {
    bool isActive = false;
    char officerName[32]{};
    char badgeNumber[8]{};
    char precinct[36]{};
    PoliceRank rank = PoliceRank::Officer;
    int32_t opinionOfPlayer = 0;
    int32_t corruptibility = 50;
    int32_t encounterCount = 0;
    int32_t bribeAcceptedCount = 0;
    uint64_t lastEncounterTick = 0;
    bool isOnPayroll = false;
};

struct PoliceContactStore {
    PoliceContactState contacts[MAX_POLICE_CONTACTS]{};
    int32_t activeCount = 0;
};

void resetPoliceContactStore(PoliceContactStore& store);
int32_t generatePoliceOfficer(
    PoliceContactStore& store,
    const char* officerName,
    const char* precinct,
    PoliceRank rank,
    int32_t corruptibility,
    uint64_t tickCount);
int32_t findPoliceContactByName(const PoliceContactStore& store, const char* officerName);
void recordPoliceEncounter(PoliceContactStore& store, int32_t contactIndex, uint64_t tickCount);
void adjustOfficerOpinion(PoliceContactStore& store, int32_t contactIndex, int32_t delta);
bool tryBribeOfficer(PoliceContactStore& store, int32_t contactIndex, uint64_t worldSeed, uint64_t tickCount);
int64_t computeOfficerBribeCostCents(const PoliceContactState& officer);
const PoliceContactState* getPoliceContact(const PoliceContactStore& store, int32_t contactIndex);
const char* policeRankToString(PoliceRank rank);
void generateOfficerName(char* outBuffer, int32_t bufferSize, uint64_t worldSeed, uint64_t tickCount, int32_t contactIndex);
void generatePrecinctName(char* outBuffer, int32_t bufferSize, uint8_t regionId);
void generateBadgeNumber(char* outBuffer, int32_t bufferSize, uint64_t worldSeed, int32_t contactIndex);
int32_t computeOfficerCorruptibility(PoliceRank rank, uint64_t worldSeed, int32_t contactIndex);

} // namespace Core
