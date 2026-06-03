#pragma once

namespace Core {

void requestDefaultGameDockLayout();
void requestResetGameDockLayout();
void beginMainDockSpace();
void finalizeGameDockLayoutForFrame();

void requestDefaultDockLayoutOnNextFrame();
void resetDockLayout();

} // namespace Core
