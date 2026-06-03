#include "character/character_name_pools.h"
#include "utils/seed_hash.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr const char* GIVEN_ITALIAN[] = {"Vito", "Rosa", "Antonio", "Giulia", "Salvatore", "Nina"};
constexpr const char* GIVEN_IRISH[] = {"Patrick", "Maeve", "Sean", "Bridget", "Tommy", "Kathleen"};
constexpr const char* GIVEN_JEWISH[] = {"Morris", "Ruth", "Hyman", "Sadie", "Louis", "Esther"};
constexpr const char* GIVEN_CHINESE[] = {"Wing", "Mei", "Lok", "Sui", "Chen", "Lily"};
constexpr const char* GIVEN_POLISH[] = {"Stanislaw", "Helena", "Jozef", "Wanda", "Kazimierz", "Anya"};
constexpr const char* GIVEN_GENERIC[] = {"James", "Mary", "John", "Anna", "Michael", "Sarah", "Frank", "Lena"};

constexpr const char* SURNAME_ITALIAN[] = {"Marino", "Lombardi", "Ricci", "Esposito", "Moretti"};
constexpr const char* SURNAME_IRISH[] = {"Sullivan", "Murphy", "Kelly", "Doyle", "Flynn"};
constexpr const char* SURNAME_JEWISH[] = {"Katz", "Rosen", "Levy", "Goldman", "Weiss"};
constexpr const char* SURNAME_CHINESE[] = {"Wong", "Lee", "Chan", "Lum", "Fong"};
constexpr const char* SURNAME_POLISH[] = {"Kowalski", "Nowak", "Wisniewski", "Zielinski"};
constexpr const char* SURNAME_GENERIC[] = {"Johnson", "Williams", "Brown", "Davis", "Miller", "Wilson"};

constexpr int32_t AMERICAN_GENERIC_SURNAME_CHANCE_PERCENT = 28;

} // namespace

void pickCharacterNameFromPool(
    char* outBuffer,
    size_t bufferSize,
    const char* const* pool,
    int32_t poolCount,
    uint64_t seed,
    int32_t salt) {
    if (poolCount <= 0 || outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    const uint32_t index = Utils::hashSeedMix(seed, salt, 0x4E414D45) % static_cast<uint32_t>(poolCount);
    std::strncpy(outBuffer, pool[index], bufferSize - 1);
    outBuffer[bufferSize - 1] = '\0';
}

CharacterNamePools pickCharacterNamePools(HeritageId heritageId, NationalityId nationalityId) {
    CharacterNamePools pools{};
    if (heritageId == HeritageId::Italian || heritageId == HeritageId::Sicilian) {
        pools.givenNames = GIVEN_ITALIAN;
        pools.givenCount = static_cast<int32_t>(sizeof(GIVEN_ITALIAN) / sizeof(GIVEN_ITALIAN[0]));
        pools.surnames = SURNAME_ITALIAN;
        pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_ITALIAN) / sizeof(SURNAME_ITALIAN[0]));
    } else if (heritageId == HeritageId::Irish) {
        pools.givenNames = GIVEN_IRISH;
        pools.givenCount = static_cast<int32_t>(sizeof(GIVEN_IRISH) / sizeof(GIVEN_IRISH[0]));
        pools.surnames = SURNAME_IRISH;
        pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_IRISH) / sizeof(SURNAME_IRISH[0]));
    } else if (heritageId == HeritageId::Jewish) {
        pools.givenNames = GIVEN_JEWISH;
        pools.givenCount = static_cast<int32_t>(sizeof(GIVEN_JEWISH) / sizeof(GIVEN_JEWISH[0]));
        pools.surnames = SURNAME_JEWISH;
        pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_JEWISH) / sizeof(SURNAME_JEWISH[0]));
    } else if (heritageId == HeritageId::Chinese) {
        pools.givenNames = GIVEN_CHINESE;
        pools.givenCount = static_cast<int32_t>(sizeof(GIVEN_CHINESE) / sizeof(GIVEN_CHINESE[0]));
        pools.surnames = SURNAME_CHINESE;
        pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_CHINESE) / sizeof(SURNAME_CHINESE[0]));
    } else if (heritageId == HeritageId::Polish || heritageId == HeritageId::Russian || heritageId == HeritageId::Greek) {
        pools.givenNames = GIVEN_POLISH;
        pools.givenCount = static_cast<int32_t>(sizeof(GIVEN_POLISH) / sizeof(GIVEN_POLISH[0]));
        pools.surnames = SURNAME_POLISH;
        pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_POLISH) / sizeof(SURNAME_POLISH[0]));
    } else {
        pools.givenNames = GIVEN_GENERIC;
        pools.givenCount = static_cast<int32_t>(sizeof(GIVEN_GENERIC) / sizeof(GIVEN_GENERIC[0]));
        pools.surnames = SURNAME_GENERIC;
        pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_GENERIC) / sizeof(SURNAME_GENERIC[0]));
    }
    if (nationalityId == NationalityId::American) {
        const uint32_t roll = Utils::hashSeedMix(static_cast<uint64_t>(heritageId), static_cast<int32_t>(nationalityId), 0x414D4552) % 100U;
        if (roll < static_cast<uint32_t>(AMERICAN_GENERIC_SURNAME_CHANCE_PERCENT)) {
            pools.surnames = SURNAME_GENERIC;
            pools.surnameCount = static_cast<int32_t>(sizeof(SURNAME_GENERIC) / sizeof(SURNAME_GENERIC[0]));
        }
    }
    return pools;
}

void buildRandomCharacterName(
    char* outBuffer,
    size_t bufferSize,
    HeritageId heritageId,
    NationalityId nationalityId,
    uint64_t seed,
    int32_t salt) {
    const CharacterNamePools pools = pickCharacterNamePools(heritageId, nationalityId);
    char given[24]{};
    char surname[24]{};
    pickCharacterNameFromPool(given, sizeof(given), pools.givenNames, pools.givenCount, seed, salt);
    pickCharacterNameFromPool(surname, sizeof(surname), pools.surnames, pools.surnameCount, seed, salt + 17);
    std::snprintf(outBuffer, bufferSize, "%s %s", given, surname);
}

} // namespace Core
