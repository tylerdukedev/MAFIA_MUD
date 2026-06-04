#include "sim/economy_system.h"
#include "game/economy_constants.h"
#include "game/housing_living_costs.h"
#include "game/player_employment.h"
#include "game/player_wallet.h"
#include "world/city_control.h"
#include "world/landmark_table.h"

namespace Core {

void EconomySystem::bind(const SimWorldBindings& inputBindings) {
    bindings = inputBindings;
    hasAppliedStartingInfluence = false;
}

const char* EconomySystem::getName() const {
    return "EconomySystem";
}

void EconomySystem::recomputeIncomeRates() {
    if (bindings.playerWallet == nullptr || bindings.playerProfile == nullptr || bindings.cityControlStore == nullptr) {
        return;
    }
    PlayerWallet& wallet = *bindings.playerWallet;
    const PlayerProfile& profile = *bindings.playerProfile;
    float legitRate = 0.0f;
    if (bindings.playerOperationsStore != nullptr) {
        legitRate = computeEmployedLegitIncomePerTickCents(*bindings.playerOperationsStore);
    }
    if (legitRate > 0.0f && profile.draft.backgroundId == BackgroundId::Bookkeeper) {
        legitRate *= BOOKKEEPER_LEGIT_INCOME_SCALE;
    }
    float crimeRate = 0.0f;
    const int32_t ownedCities = countPlayerOwnedCities(*bindings.cityControlStore);
    if (ownedCities > 0) {
        crimeRate = CRIME_INCOME_BASE_CENTS_PER_TICK * static_cast<float>(ownedCities);
        crimeRate += profile.opportunityPaths.streetCrimePath * 0.35f;
        if (profile.draft.generationId == GenerationId::Immigrant) {
            crimeRate *= IMMIGRANT_CRIME_INCOME_SCALE;
        }
        if (profile.draft.backgroundId == BackgroundId::StreetHustler) {
            crimeRate *= STREET_HUSTLER_CRIME_INCOME_SCALE;
        }
    }
    wallet.legitIncomePerTickCents = legitRate;
    wallet.crimeIncomePerTickCents = crimeRate;
}

void EconomySystem::applyAccruedIncome() {
    if (bindings.playerWallet == nullptr) {
        return;
    }
    PlayerWallet& wallet = *bindings.playerWallet;
    wallet.accruedLegitCents += wallet.legitIncomePerTickCents;
    wallet.accruedCrimeCents += wallet.crimeIncomePerTickCents;
}

void EconomySystem::applyStartingBoroughInfluence() {
    if (hasAppliedStartingInfluence || bindings.chunkStore == nullptr || bindings.playerProfile == nullptr) {
        return;
    }
    const int32_t landmarkIndex = bindings.playerProfile->draft.startingCityLandmarkIndex;
    const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
    if (landmark == nullptr) {
        hasAppliedStartingInfluence = true;
        return;
    }
    const int32_t radius = static_cast<int32_t>(landmark->heatRadiusTiles) + 2;
    for (int32_t offsetY = -radius; offsetY <= radius; ++offsetY) {
        for (int32_t offsetX = -radius; offsetX <= radius; ++offsetX) {
            const WorldCoord coord{landmark->tileX + offsetX, landmark->tileY + offsetY};
            if (bindings.chunkStore->getTerrainAt(coord) == TerrainId::Water) {
                continue;
            }
            const uint8_t influence = static_cast<uint8_t>(std::min(24, static_cast<int32_t>(bindings.chunkStore->getPlayerInfluenceAt(coord)) + 6));
            bindings.chunkStore->setPlayerInfluenceAt(coord, influence);
        }
    }
    hasAppliedStartingInfluence = true;
}

void EconomySystem::applyMonthlyLivingCosts(uint64_t tickCount) {
    if (bindings.playerOperationsStore == nullptr || bindings.playerWallet == nullptr || bindings.characterAgentStore == nullptr) {
        return;
    }
    const int32_t employedIndex = bindings.playerOperationsStore->employedBusinessIndices[0];
    applyMonthlyLivingLedger(
        *bindings.playerOperationsStore,
        *bindings.playerWallet,
        *bindings.characterAgentStore,
        employedIndex,
        tickCount);
}

void EconomySystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    applyStartingBoroughInfluence();
    recomputeIncomeRates();
    applyAccruedIncome();
    applyMonthlyLivingCosts(tickCount);
    if (tickCount % static_cast<uint64_t>(ECONOMY_INCOME_APPLY_INTERVAL_TICKS) != 0ULL) {
        return;
    }
    if (bindings.playerWallet == nullptr) {
        return;
    }
    PlayerWallet& wallet = *bindings.playerWallet;
    const int64_t legitPayout = static_cast<int64_t>(wallet.accruedLegitCents);
    const int64_t crimePayout = static_cast<int64_t>(wallet.accruedCrimeCents);
    wallet.accruedLegitCents = 0.0f;
    wallet.accruedCrimeCents = 0.0f;
    if (legitPayout > 0) {
        creditLegitCash(wallet, legitPayout);
    }
    if (crimePayout > 0) {
        creditCrimeCash(wallet, crimePayout);
    }
}

} // namespace Core
