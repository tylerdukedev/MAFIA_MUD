#include "utils/app_paths.h"
#include <cstdio>
#include <filesystem>

namespace Core {

namespace {

bool trySetWorkingDirectory(const std::filesystem::path& directoryPath) {
    std::error_code errorCode;
    std::filesystem::create_directories(directoryPath, errorCode);
    if (errorCode) {
        return false;
    }
    std::filesystem::current_path(directoryPath, errorCode);
    return !errorCode;
}

std::filesystem::path resolveUserDataRootPath() {
#if defined(_WIN32)
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        return std::filesystem::path(localAppData) / "CapitalVice";
    }
    if (const char* userProfile = std::getenv("USERPROFILE")) {
        return std::filesystem::path(userProfile) / "AppData" / "Local" / "CapitalVice";
    }
    return std::filesystem::path("CapitalVice");
#else
    if (const char* homeDirectory = std::getenv("HOME")) {
        return std::filesystem::path(homeDirectory) / ".local" / "share" / "CapitalVice";
    }
    return std::filesystem::path("CapitalVice");
#endif
}

} // namespace

bool initializeApplicationUserDataDirectory() {
    const std::filesystem::path userDataPath = resolveUserDataRootPath();
    if (!trySetWorkingDirectory(userDataPath)) {
        std::fprintf(stderr, "Capital Vice: could not use save directory %s\n", userDataPath.string().c_str());
        return false;
    }
    return true;
}

} // namespace Core
