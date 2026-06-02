#include "core/application.h"
#include "ui/game_ui.h"
#include "procgen/world_generator.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#define GL_SILENCE_DEPRECATION
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <cstdio>

namespace Core {

namespace {
constexpr int32_t WINDOW_WIDTH = 1280;
constexpr int32_t WINDOW_HEIGHT = 800;
constexpr int32_t MIN_WINDOW_WIDTH = 960;
constexpr int32_t MIN_WINDOW_HEIGHT = 540;
constexpr const char* WINDOW_TITLE = "CapitalVice";
constexpr const char* GLSL_VERSION = "#version 130";
constexpr const char* IMGUI_INI_PATH = "capitalvice_layout.ini";
} // namespace

void Application::glfwErrorCallback(int errorCode, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", errorCode, description);
}

Application::Application()
    : window(nullptr)
    , imguiContext(nullptr)
    , chunkStore(worldConfig)
    , simClock(WorldConfig::DEFAULT_TICK_RATE_HZ)
    , worldSeed(DEFAULT_WORLD_SEED)
    , isRunning(false) {
}

Application::~Application() {
    shutdownImGui();
    shutdownWindow();
}

int Application::run() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        return 1;
    }
    if (!initializeWindow()) {
        glfwTerminate();
        return 1;
    }
    if (!initializeImGui()) {
        shutdownWindow();
        glfwTerminate();
        return 1;
    }
    systemRegistry.initialize();
    WorldGenerator worldGenerator;
    worldGenerator.generate(worldConfig, chunkStore, worldSeed);
    isRunning = true;
    while (isRunning && !glfwWindowShouldClose(window)) {
        processFrame();
    }
    shutdownImGui();
    shutdownWindow();
    glfwTerminate();
    return 0;
}

bool Application::initializeWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (window == nullptr) {
        return false;
    }
    glfwSetWindowSizeLimits(window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return true;
}

bool Application::initializeImGui() {
    IMGUI_CHECKVERSION();
    imguiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = IMGUI_INI_PATH;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(GLSL_VERSION)) {
        return false;
    }
    return true;
}

void Application::shutdownImGui() {
    if (imguiContext == nullptr) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    imguiContext = nullptr;
}

void Application::shutdownWindow() {
    if (window != nullptr) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
}

void Application::processFrame() {
    glfwPollEvents();
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        simClock.togglePaused();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        simClock.stepOneTick();
    }
    updateSimulation();
    renderFrame();
}

void Application::updateSimulation() {
    const double deltaSeconds = ImGui::GetIO().DeltaTime;
    simClock.update(deltaSeconds);
    runSimulationTicks();
}

void Application::runSimulationTicks() {
    const int32_t ticksThisFrame = simClock.getTicksThisFrame();
    if (ticksThisFrame <= 0) {
        return;
    }
    const uint64_t tickCount = simClock.getTickCount();
    const uint64_t firstTick = tickCount - static_cast<uint64_t>(ticksThisFrame) + 1U;
    for (uint64_t tickIndex = firstTick; tickIndex <= tickCount; ++tickIndex) {
        systemRegistry.runTick(tickIndex);
    }
}

void Application::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    renderGameUi(simClock, worldConfig, chunkStore, systemRegistry, viewportPickState, worldSeed);
    ImGui::Render();
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    renderClearBackground();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void Application::renderClearBackground() {
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace Core
