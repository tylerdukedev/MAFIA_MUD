#pragma once

#include "sim/world_event_types.h"

namespace Core {

int32_t getWorldEventDefinitionCount();
const WorldEventDefinition* getWorldEventDefinition(int32_t definitionIndex);
int32_t findWorldEventDefinitionIndexById(const char* eventId);

} // namespace Core
