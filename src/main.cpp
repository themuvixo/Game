// Lumo and the Shattered Dream - entry point.
#include <exception>
#include <filesystem>
#include <string>
#include "Game.h"
#include "Utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static std::filesystem::path executableDir(const char* argv0) {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return std::filesystem::path(buf).parent_path();
#endif
    std::error_code ec;
    std::filesystem::path p = std::filesystem::absolute(argv0 ? argv0 : "", ec);
    return ec ? std::filesystem::current_path(ec) : p.parent_path();
}

static void fatal(const std::string& msg) {
    rayman::logLine("[fatal] " + msg);
#ifdef _WIN32
    MessageBoxA(nullptr, msg.c_str(), "Lumo and the Shattered Dream", MB_OK | MB_ICONERROR);
#endif
}

int main(int argc, char** argv) {
    (void)argc;
    try {
        rayman::Game game(executableDir(argc > 0 ? argv[0] : nullptr));
        return game.run();
    } catch (const std::exception& e) {
        fatal(std::string("Unexpected error: ") + e.what());
    } catch (...) {
        fatal("Unexpected unknown error.");
    }
    return 1;
}
