#pragma once

#include <cstdint>

namespace Core {

struct HelpManualTopic {
    const char* id;
    const char* chapterTitle;
    const char* title;
    const char* summary;
    const char* const* paragraphs;
    int32_t paragraphCount;
};

struct HelpManualState {
    bool isOpen = false;
    int32_t selectedTopicIndex = 0;
};

int32_t getHelpManualTopicCount();
const HelpManualTopic* getHelpManualTopic(int32_t topicIndex);
int32_t findHelpManualTopicIndexById(const char* topicId);
void openHelpManualTopic(HelpManualState& state, const char* topicId);
void renderHelpManualWindow(HelpManualState& state);

} // namespace Core
