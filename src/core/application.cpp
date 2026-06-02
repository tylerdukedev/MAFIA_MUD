#include "core/application.h"
#include "ui/game_ui.h"
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
constexpr const char* WINDOW_TITLE = "CapitalVice";
constexpr const char* GLSL_VERSION = "#version 130";
} // namespace

void Application::glfwErrorCallback(int errorCode, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", errorCode, description);
}

Application::Application()
    : window(nullptr)
    , imguiContext(nullptr)
    , chunkStore(worldConfig)
    , simClock(WorldConfig::DEFAULT_TICK_RATE_HZ)
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
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return true;
}

bool Application::initializeImGui() {
    IMGUI_CHECKVERSION();
    imguiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
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
}

void Application::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    renderGameUi(simClock, worldConfig, chunkStore);
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
