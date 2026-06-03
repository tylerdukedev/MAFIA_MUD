#include "game/player_organization_ui.h"

namespace Core {

const char* crewRecruitLockReasonToString(CrewRecruitLockReason reason) {
    switch (reason) {
    case CrewRecruitLockReason::None:
        return "Can recruit";
    case CrewRecruitLockReason::NotSoloOrCrewBuilding:
        return "Crew locked at organization tier";
    case CrewRecruitLockReason::AlreadyInCrew:
        return "Already in your crew";
    case CrewRecruitLockReason::AgentInactive:
        return "Contact unavailable";
    case CrewRecruitLockReason::NotRecruitableRole:
        return "Not a street contact";
    case CrewRecruitLockReason::LowOpinion:
        return "Opinion too low";
    case CrewRecruitLockReason::LowTrust:
        return "Trust too low";
    case CrewRecruitLockReason::LowRespect:
        return "Respect too low";
    case CrewRecruitLockReason::LowLoyalty:
        return "Loyalty too low";
    case CrewRecruitLockReason::CrewFull:
        return "Crew is full";
    default:
        return "Locked";
    }
}

const char* organizationFormLockReasonToString(OrganizationFormLockReason reason) {
    switch (reason) {
    case OrganizationFormLockReason::None:
        return "Ready to incorporate";
    case OrganizationFormLockReason::NotCrewTier:
        return "Formalize a crew first";
    case OrganizationFormLockReason::InsufficientMembers:
        return "Need more crew members";
    case OrganizationFormLockReason::InsufficientNetwork:
        return "Network too thin";
    case OrganizationFormLockReason::InsufficientStreetPath:
        return "Street path too weak";
    case OrganizationFormLockReason::InsufficientReputation:
        return "Reputation too low";
    case OrganizationFormLockReason::InsufficientCrimeRecord:
        return "Need more crime income history";
    case OrganizationFormLockReason::HeatTooHigh:
        return "Police heat too high";
    case OrganizationFormLockReason::ActiveWarrant:
        return "Active warrant blocks incorporation";
    case OrganizationFormLockReason::Incarcerated:
        return "Cannot incorporate while in custody";
    default:
        return "Locked";
    }
}

} // namespace Core
