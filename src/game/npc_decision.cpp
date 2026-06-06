#include "game/npc_decision.h"
#include "game/housing_living_costs.h"
#include "game/property_store.h"
#include "game/shared_travel_state.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include "world/chunk_store.h"
#include "world/region_table.h"
#include "world/world_config.h"
#include <algorithm>

namespace Core {

namespace {

constexpr int32_t NPC_DECISION_INTERVAL_TICKS = 50;
constexpr int32_t NPC_WORK_START_HOUR = 9;
constexpr int32_t NPC_WORK_END_HOUR = 17;
constexpr int32_t NPC_DECISION_ACT_PERCENT = 30;
constexpr int32_t NPC_BROKE_THRESHOLD_CENTS = 1500;
constexpr int8_t NPC_LAY_LOW_WANTED_LEVEL = 3;
constexpr int32_t NPC_MAX_CASH_CENTS = 5000000;
constexpr int32_t NPC_MONTHLY_FOOD_CENTS = 600;
constexpr int8_t NPC_MAX_WANTED_LEVEL = 6;

int32_t racketMonthlyIncomeCents(AgentArchetype archetype, uint32_t roll) {
    switch (archetype) {
        case AgentArchetype::Racketeer: return 6000 + static_cast<int32_t>(roll % 6000U);
        case AgentArchetype::Bootlegger: return 5000 + static_cast<int32_t>(roll % 5000U);
        case AgentArchetype::Loanshark: return 5000 + static_cast<int32_t>(roll % 4000U);
        case AgentArchetype::Fence: return 3000 + static_cast<int32_t>(roll % 4000U);
        case AgentArchetype::Enforcer: return 3000 + static_cast<int32_t>(roll % 3000U);
        case AgentArchetype::NumbersRunner: return 2000 + static_cast<int32_t>(roll % 3000U);
        default: return 0;
    }
}

const char* getActivityDisplayLabelInternal(AgentActivity activity) {
    switch (activity) {
        case AgentActivity::Idle: return "Idle";
        case AgentActivity::AtHome: return "At Home";
        case AgentActivity::AtWork: return "At Work";
        case AgentActivity::Traveling: return "Traveling";
        case AgentActivity::InBuilding: return "In Building";
        case AgentActivity::Abroad: return "Abroad";
        case AgentActivity::Incarcerated: return "Incarcerated";
        default: return "Unknown";
    }
}

bool isWorkHours(const GameCalendarStore& calendar) {
    if (isWeekend(calendar)) {
        return false;
    }
    return calendar.hourOfDay >= NPC_WORK_START_HOUR && calendar.hourOfDay < NPC_WORK_END_HOUR;
}

bool tryGetPropertyTile(const PropertyStore& propertyStore, int32_t propertyIndex, int32_t& outTileX, int32_t& outTileY) {
    const PropertyRecord* property = getPropertyRecord(propertyStore, propertyIndex);
    if (property == nullptr || property->tileX < 0 || property->tileY < 0) {
        return false;
    }
    outTileX = property->tileX;
    outTileY = property->tileY;
    return true;
}

bool tryBeginAgentTravel(
    CharacterAgentState& agent,
    int32_t destTileX,
    int32_t destTileY,
    const NPCDecisionContext& context) {
    if (context.chunkStore == nullptr || context.worldConfig == nullptr) {
        return false;
    }
    if (destTileX < 0 || destTileY < 0) {
        return false;
    }
    return beginAgentRoadTravel(
        agent,
        destTileX,
        destTileY,
        context.tickCount,
        *context.chunkStore,
        *context.worldConfig);
}

bool tryBeginTravelHome(CharacterAgentState& agent, const NPCDecisionContext& context) {
    if (context.propertyStore == nullptr || agent.homePropertyIndex < 0) {
        return false;
    }
    int32_t homeTileX = -1;
    int32_t homeTileY = -1;
    if (!tryGetPropertyTile(*context.propertyStore, agent.homePropertyIndex, homeTileX, homeTileY)) {
        return false;
    }
    return tryBeginAgentTravel(agent, homeTileX, homeTileY, context);
}

bool tryBeginTravelToWorkplace(CharacterAgentState& agent, const NPCDecisionContext& context) {
    if (agent.workplaceBusinessIndex < 0) {
        return false;
    }
    const BusinessNodeDefinition* workplace = getBusinessNodeDefinition(agent.workplaceBusinessIndex);
    if (workplace == nullptr) {
        return false;
    }
    return tryBeginAgentTravel(agent, workplace->tileX, workplace->tileY, context);
}

bool tryBeginTravelToRegionBusiness(CharacterAgentState& agent, const NPCDecisionContext& context, uint64_t seed) {
    const RegionId region = static_cast<RegionId>(agent.homeRegionId);
    const int32_t businessCount = getBusinessNodeCount();
    int32_t matchCount = 0;
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        if (region != RegionId::None && getBusinessNodeRegionId(businessIndex) != region) {
            continue;
        }
        matchCount += 1;
    }
    if (matchCount <= 0) {
        return false;
    }
    const int32_t pick = static_cast<int32_t>(seed % static_cast<uint64_t>(matchCount));
    int32_t seen = 0;
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        if (region != RegionId::None && getBusinessNodeRegionId(businessIndex) != region) {
            continue;
        }
        if (seen == pick) {
            const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
            if (business == nullptr) {
                return false;
            }
            return tryBeginAgentTravel(agent, business->tileX, business->tileY, context);
        }
        seen += 1;
    }
    return false;
}

bool tryBeginTravelToTie(
    CharacterAgentState& agent,
    int32_t agentIndex,
    const CharacterAgentStore& agentStore,
    const NPCDecisionContext& context,
    uint64_t seed) {
    if (context.relationshipGraph == nullptr) {
        return false;
    }
    const int32_t tieCount = getAgentTieCount(*context.relationshipGraph, agentIndex);
    if (tieCount <= 0) {
        return false;
    }
    const int32_t startTie = static_cast<int32_t>(seed % static_cast<uint64_t>(tieCount));
    for (int32_t offset = 0; offset < tieCount; ++offset) {
        const int32_t tieIndex = (startTie + offset) % tieCount;
        const AgentTie* tie = getAgentTie(*context.relationshipGraph, agentIndex, tieIndex);
        if (tie == nullptr || tie->targetAgentIndex < 0) {
            continue;
        }
        const CharacterAgentState& target = agentStore.states[tie->targetAgentIndex];
        if (!target.isActive || target.currentTileX < 0 || target.currentTileY < 0) {
            continue;
        }
        return tryBeginAgentTravel(agent, target.currentTileX, target.currentTileY, context);
    }
    return false;
}

} // namespace

AgentObjective selectAgentObjective(
    const CharacterAgentState& agent,
    const GameCalendarStore& calendar,
    uint32_t decisionRoll) {
    if (agent.wantedLevel >= NPC_LAY_LOW_WANTED_LEVEL) {
        return AgentObjective::LayLow;
    }
    const bool workHours = isWorkHours(calendar);
    const bool isCriminal = isCriminalArchetype(agent.generatedArchetype);
    const bool isLawEnf = isLawEnforcementArchetype(agent.generatedArchetype);
    const bool isBroke = agent.cashCents < NPC_BROKE_THRESHOLD_CENTS;
    if (isLawEnf && workHours) {
        return AgentObjective::Patrol;
    }
    if (workHours && agent.workplaceBusinessIndex >= 0) {
        return AgentObjective::EarnWage;
    }
    if (isBroke) {
        return isCriminal ? AgentObjective::RunRacket : AgentObjective::SeekIncome;
    }
    if (isCriminal && (decisionRoll % 3U) == 0U) {
        return AgentObjective::RunRacket;
    }
    const uint32_t moodRoll = decisionRoll % 100U;
    if (moodRoll < 35U) {
        return AgentObjective::Socialize;
    }
    if (moodRoll < 70U) {
        return AgentObjective::RestAtHome;
    }
    return AgentObjective::Settle;
}

void tickNpcDecisions(
    CharacterAgentStore& agentStore,
    const NPCDecisionContext& context,
    int32_t ticksSinceLastDecision) {
    (void)ticksSinceLastDecision;

    const GameCalendarStore* calendar = context.calendarStore;
    if (calendar == nullptr) {
        return;
    }

    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        CharacterAgentState& agent = agentStore.states[agentIndex];

        if (!agent.isActive) {
            continue;
        }
        if (agent.currentActivity == AgentActivity::Traveling || agent.currentActivity == AgentActivity::Incarcerated) {
            continue;
        }

        const uint64_t decisionSeed =
            context.worldSeed ^ (static_cast<uint64_t>(agentIndex) * 0x9E3779B1U) ^ (context.tickCount / NPC_DECISION_INTERVAL_TICKS);
        const uint32_t decisionRoll = static_cast<uint32_t>(decisionSeed % 100);

        if (decisionRoll >= static_cast<uint32_t>(NPC_DECISION_ACT_PERCENT)) {
            continue;
        }

        const AgentObjective objective = selectAgentObjective(agent, *calendar, decisionRoll);
        agent.currentObjective = objective;
        const uint64_t actionSeed = decisionSeed * 0x2545F4914F6CDD1DULL + static_cast<uint64_t>(agentIndex);

        switch (objective) {
            case AgentObjective::EarnWage:
                if (!tryBeginTravelToWorkplace(agent, context)) {
                    setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
                }
                break;
            case AgentObjective::Patrol:
            case AgentObjective::SeekIncome:
            case AgentObjective::RunRacket:
                if (!tryBeginTravelToRegionBusiness(agent, context, actionSeed)) {
                    setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
                }
                break;
            case AgentObjective::Socialize:
                if (!tryBeginTravelToTie(agent, agentIndex, agentStore, context, actionSeed) &&
                    !tryBeginTravelToRegionBusiness(agent, context, actionSeed)) {
                    setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
                }
                break;
            case AgentObjective::RestAtHome:
            case AgentObjective::LayLow:
                if (agent.currentActivity != AgentActivity::AtHome && !tryBeginTravelHome(agent, context)) {
                    setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
                }
                break;
            case AgentObjective::Settle:
            default:
                if (agent.currentActivity == AgentActivity::AtHome) {
                    setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
                }
                break;
        }
    }
}

void tickNpcMonthlyLedger(
    CharacterAgentStore& agentStore,
    const PropertyStore& propertyStore,
    uint64_t tickCount,
    uint64_t worldSeed) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        CharacterAgentState& agent = agentStore.states[agentIndex];
        if (!agent.isActive || !agent.hasGeneratedIdentity) {
            continue;
        }
        if (agent.currentActivity == AgentActivity::Incarcerated) {
            continue;
        }
        if (agent.lastWageTick != 0 && (tickCount - agent.lastWageTick) < static_cast<uint64_t>(MONTHLY_LEDGER_INTERVAL_TICKS)) {
            continue;
        }
        agent.lastWageTick = tickCount;

        int64_t incomeCents = 0;
        if (agent.workplaceBusinessIndex >= 0) {
            const BusinessNodeDefinition* business = getBusinessNodeDefinition(agent.workplaceBusinessIndex);
            if (business != nullptr) {
                incomeCents += business->jobWageCents * static_cast<int64_t>(JOB_MONTHLY_WAGE_MULTIPLIER);
            }
        }
        const bool isCriminal = isCriminalArchetype(agent.generatedArchetype);
        const uint32_t roll = static_cast<uint32_t>((worldSeed ^ (static_cast<uint64_t>(agentIndex) * 0x9E3779B97F4A7C15ULL) ^ tickCount) % 100000ULL);
        if (isCriminal) {
            incomeCents += racketMonthlyIncomeCents(agent.generatedArchetype, roll);
        }

        int64_t rentCents = 0;
        if (agent.homePropertyIndex >= 0) {
            const PropertyRecord* home = getPropertyRecord(propertyStore, agent.homePropertyIndex);
            if (home != nullptr) {
                rentCents = home->monthlyCostCents;
            }
        }

        const int64_t netCents = incomeCents - rentCents - NPC_MONTHLY_FOOD_CENTS;
        const int64_t updated = static_cast<int64_t>(agent.cashCents) + netCents;
        agent.cashCents = static_cast<int32_t>(std::clamp<int64_t>(updated, 0, NPC_MAX_CASH_CENTS));

        if (isCriminal && incomeCents > 0) {
            agent.wantedLevel = static_cast<int8_t>(std::min<int32_t>(agent.wantedLevel + 1, NPC_MAX_WANTED_LEVEL));
        } else if (agent.wantedLevel > 0) {
            agent.wantedLevel = static_cast<int8_t>(agent.wantedLevel - 1);
        }
    }
}

const char* getActivityDisplayLabel(AgentActivity activity) {
    return getActivityDisplayLabelInternal(activity);
}

} // namespace Core
