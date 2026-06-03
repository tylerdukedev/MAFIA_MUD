#include "game/player_information_feed.h"
#include <cstdio>
#include <cstring>

namespace Core {

void resetPlayerInformationFeedStore(PlayerInformationFeedStore& store) {
    store.itemCount = 0;
}

void pushInformationFeedItem(
    PlayerInformationFeedStore& store,
    InformationChannel channel,
    const char* headline,
    const char* body,
    uint64_t tickCount,
    bool pausesSimulation) {
    if (headline == nullptr || headline[0] == '\0') {
        return;
    }
    if (store.itemCount >= MAX_INFORMATION_FEED_ITEMS) {
        for (int32_t shiftIndex = 1; shiftIndex < MAX_INFORMATION_FEED_ITEMS; ++shiftIndex) {
            store.items[shiftIndex - 1] = store.items[shiftIndex];
        }
        store.itemCount = MAX_INFORMATION_FEED_ITEMS - 1;
    }
    InformationFeedItem& item = store.items[store.itemCount];
    std::snprintf(item.headline, sizeof(item.headline), "%s", headline);
    if (body != nullptr) {
        std::snprintf(item.body, sizeof(item.body), "%s", body);
    } else {
        item.body[0] = '\0';
    }
    item.channel = channel;
    item.createdTick = tickCount;
    item.pausesSimulation = pausesSimulation;
    item.isRead = false;
    item.isDismissed = false;
    store.itemCount += 1;
}

int32_t getUnreadInformationFeedCount(const PlayerInformationFeedStore& store) {
    int32_t unreadCount = 0;
    for (int32_t itemIndex = 0; itemIndex < store.itemCount; ++itemIndex) {
        if (!store.items[itemIndex].isDismissed && !store.items[itemIndex].isRead) {
            unreadCount += 1;
        }
    }
    return unreadCount;
}

} // namespace Core
