#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t MAX_INFORMATION_FEED_ITEMS = 24;
constexpr int32_t MAX_INFORMATION_HEADLINE_LENGTH = 96;
constexpr int32_t MAX_INFORMATION_BODY_LENGTH = 192;

enum class InformationChannel : uint8_t {
    Newspaper = 0,
    Rumor = 1,
    Intel = 2,
};

struct InformationFeedItem {
    char headline[MAX_INFORMATION_HEADLINE_LENGTH]{};
    char body[MAX_INFORMATION_BODY_LENGTH]{};
    InformationChannel channel = InformationChannel::Rumor;
    uint64_t createdTick = 0;
    bool pausesSimulation = false;
    bool isRead = false;
    bool isDismissed = false;
};

struct PlayerInformationFeedStore {
    InformationFeedItem items[MAX_INFORMATION_FEED_ITEMS]{};
    int32_t itemCount = 0;
};

void resetPlayerInformationFeedStore(PlayerInformationFeedStore& store);
void pushInformationFeedItem(
    PlayerInformationFeedStore& store,
    InformationChannel channel,
    const char* headline,
    const char* body,
    uint64_t tickCount,
    bool pausesSimulation);
int32_t getUnreadInformationFeedCount(const PlayerInformationFeedStore& store);

} // namespace Core
