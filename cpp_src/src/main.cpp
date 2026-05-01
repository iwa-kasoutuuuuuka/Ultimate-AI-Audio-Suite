#include "GUIManager.hpp"
#include <iostream>
#include <memory>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep) {
    FILE* f = fopen("crash_log.txt", "w");
    if (f) {
        fprintf(f, "CRASH DETECTED!\n");
        fprintf(f, "Exception Code: 0x%08X\n", ep->ExceptionRecord->ExceptionCode);
        fprintf(f, "Exception Address: 0x%p\n", ep->ExceptionRecord->ExceptionAddress);
        fclose(f);
    }
    MessageBoxA(NULL, "Crash logged to crash_log.txt", "Crash", MB_OK);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetUnhandledExceptionFilter(CrashHandler);
#endif

    std::cout << "=== STARTUP (HEAP ALLOCATED) ===" << std::endl;

    // Use HEAP to ensure proper memory alignment
    auto gui = std::make_unique<GUIManager>();
    
    if (!gui->init()) {
        std::cerr << "Failed to initialize GUI." << std::endl;
        return 1;
    }

    gui->run();

    return 0;
}
