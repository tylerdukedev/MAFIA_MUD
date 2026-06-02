#pragma once

#include "core/sim_clock.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <GLFW/glfw3.h>
#include <cstdint>

struct ImGuiContext;

namespace Core {

class Application {
public:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    int run();

private:
    GLFWwindow* window;
    ImGuiContext* imguiContext;
    WorldConfig worldConfig;
    ChunkStore chunkStore;
    SimClock simClock;
    bool isRunning;
    static void glfwErrorCallback(int errorCode, const char* description);
    bool initializeWindow();
    bool initializeImGui();
    void shutdownImGui();
    void shutdownWindow();
    void processFrame();
    void updateSimulation();
    void renderFrame();
    void renderClearBackground();
};

} // namespace Core
