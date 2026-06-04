#include "game/npc_decision.h"
#include "sim/character_agent.h"
#include "world/chunk_store.h"
#include "world/world_config.h"

namespace Core {

namespace {

constexpr int32_t NPC_DECISION_INTERVAL_TICKS = 50;
constexpr int32_t NPC_WORK_START_HOUR = 9;
constexpr int32_t NPC_WORK_END_HOUR = 17;
constexpr int32_t NPC_TRAVEL_SPEED_TILES_PER_TICK = 2;

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

bool shouldLeaveForWork(const CharacterAgentState& agent, const GameCalendarStore& calendar) {
    // Only leave if at home and it's work hours on a weekday
    if (agent.currentActivity != AgentActivity::AtHome) {
        return false;
    }
    if (isWeekend(calendar)) {
        return false;
    }
    if (calendar.hourOfDay < NPC_WORK_START_HOUR || calendar.hourOfDay >= NPC_WORK_END_HOUR) {
        return false;
    }
    return true;
}

bool shouldReturnHome(const CharacterAgentState& agent, const GameCalendarStore& calendar) {
    // Only return if at work and it's evening or weekend
    if (agent.currentActivity != AgentActivity::AtWork) {
        return false;
    }
    if (calendar.hourOfDay >= NPC_WORK_END_HOUR || isWeekend(calendar)) {
        return true;
    }
    return false;
}

bool shouldGoOutDuringOffHours(const CharacterAgentState& agent, uint64_t worldSeed, int32_t agentIndex) {
    // Only if at home during off hours
    if (agent.currentActivity != AgentActivity::AtHome) {
        return false;
    }

    // Random chance to go out (roughly 20% chance per decision tick)
    const uint64_t rollSeed = worldSeed ^ agent.currentTileX ^ agent.currentTileY;
    const uint32_t roll = static_cast<uint32_t>(rollSeed % 100);
    return roll < 20;
}

void updateNpcTravel(CharacterAgentState& agent, int32_t ticksElapsed) {
    if (agent.currentActivity != AgentActivity::Traveling) {
        return;
    }
    if (agent.destinationTileX < 0 || agent.destinationTileY < 0) {
        // No destination, become idle
        setAgentActivity(agent, AgentActivity::Idle, 0);
        return;
    }

    const int32_t dx = agent.destinationTileX - agent.currentTileX;
    const int32_t dy = agent.destinationTileY - agent.currentTileY;

    if (dx == 0 && dy == 0) {
        // Arrived at destination
        agent.currentActivity = AgentActivity::Idle;
        agent.destinationTileX = -1;
        agent.destinationTileY = -1;
        return;
    }

    // Move toward destination
    const int32_t stepX = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
    const int32_t stepY = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;

    agent.currentTileX += stepX;
    agent.currentTileY += stepY;
}

} // namespace

void tickNpcDecisions(
    CharacterAgentStore& agentStore,
    const NPCDecisionContext& context,
    int32_t ticksSinceLastDecision) {

    const GameCalendarStore* calendar = context.calendarStore;
    if (calendar == nullptr) {
        return;
    }

    const int32_t hourOfDay = calendar->hourOfDay;

    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        CharacterAgentState& agent = agentStore.states[agentIndex];

        if (!agent.isActive) {
            continue;
        }

        // Update travel progress
        updateNpcTravel(agent, ticksSinceLastDecision);

        // Decision making (throttled to avoid CPU overhead)
        const uint64_t decisionSeed = context.worldSeed ^ agentIndex ^ (context.tickCount / NPC_DECISION_INTERVAL_TICKS);
        const uint32_t decisionRoll = static_cast<uint32_t>(decisionSeed % 100);

        // Only make decisions if something should change
        if (decisionRoll > 10) {
            continue;
        }

        // Priority 1: Work schedule
        if (shouldLeaveForWork(agent, *calendar)) {
            if (agent.workplaceBusinessIndex >= 0) {
                // TODO: Get workplace location from business table
                // For now, NPCs stay at home without assigned workplaces
            }
        } else if (shouldReturnHome(agent, *calendar)) {
            // Return to home
            if (agent.homePropertyIndex >= 0) {
                setAgentActivity(agent, AgentActivity::Traveling, context.tickCount);
                // destination will be set from property lookup in next phase
            } else {
                setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
            }
        }
        // Priority 2: Random off-hours activities
        else if (shouldGoOutDuringOffHours(agent, context.worldSeed, agentIndex)) {
            // Random nearby location (park, bar, etc.)
            // For now, just mark as idle outside
            setAgentActivity(agent, AgentActivity::Idle, context.tickCount);
        }
    }
}

const char* getActivityDisplayLabel(AgentActivity activity) {
    return getActivityDisplayLabelInternal(activity);
}

} // namespace Core