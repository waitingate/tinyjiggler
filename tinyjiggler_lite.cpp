// TinyJiggler Lite - keeps Windows awake while you are away.
// No window, no tray icon, no settings screen.
//
//   Run it once:   it starts working, silently in the background.
//   Run it again:  it stops the running copy and says "stopped".
//   Is it on?      Task Manager > Details > TinyJigglerLite.exe
//
// How it works: tells Windows "don't sleep or turn off the screen", and when you
// have been idle for kIdleSeconds, sends a tiny fake mouse move. That resets
// Windows' idle timer: no screensaver, no auto-lock, no "Away" status.
//
// Build: build_lite.bat, or by hand (64-bit MinGW-w64 g++, no C runtime):
//   g++ -std=c++17 -Os -s -nostdlib -nostartfiles -e Start -Wl,--subsystem,windows -Wl,--gc-sections
//       -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-stack-protector
//       -o TinyJigglerLite.exe tinyjiggler_lite.cpp -lkernel32 -luser32

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---- Settings: change and rebuild ----
constexpr DWORD kIdleSeconds = 30; // jiggle after this long idle; keep below your screensaver/lock time
constexpr int   kMovePixels  = 4;  // size of the nudge; 0 = Zen mode (pointer never moves)
static_assert(kIdleSeconds > 0, "kIdleSeconds = 0 would jiggle nonstop");
static_assert(kIdleSeconds <= 0xFFFFFFFF / 1000, "kIdleSeconds * 1000 must fit in a DWORD");

constexpr DWORD kLimitMs = kIdleSeconds * 1000;

// Milliseconds since the last keyboard/mouse input (yours or ours).
// If the call fails, dwTime stays 0, which counts as idle: it keeps working.
static DWORD IdleMs()
{
    LASTINPUTINFO li{sizeof(LASTINPUTINFO), 0};
    GetLastInputInfo(&li);
    return GetTickCount() - li.dwTime;
}

static void Jiggle()
{
    static bool back;                          // every other nudge goes back, so the pointer doesn't drift
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;          // relative move; 0,0 still counts as input
    in.mi.dx = in.mi.dy = back ? -kMovePixels : kMovePixels;
    back = !back;
    SendInput(1, &in, sizeof(in));
}

// No C runtime: Windows starts the program here.
extern "C" void Start()
{
    // One named "stop" signal shared by every copy. If it already exists,
    // another copy is running: tell it to stop, and we are done.
    HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, L"Local\\TinyJigglerLite");
    if (!stop) {
        // Access denied: the running copy is an administrator's, which a
        // normal copy may not signal.
        bool admin = GetLastError() == ERROR_ACCESS_DENIED;
        MessageBoxW(nullptr, admin ? L"TinyJiggler is running as administrator. "
                                     L"Run TinyJigglerLite.exe as administrator to stop it."
                                   : L"TinyJiggler failed to start (could not create stop event).",
                    L"TinyJiggler", admin ? MB_ICONINFORMATION : MB_ICONERROR);
        ExitProcess(1);
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        SetEvent(stop);
        CloseHandle(stop);
        MessageBoxW(nullptr, L"TinyJiggler stopped.", L"TinyJiggler", MB_ICONINFORMATION);
        ExitProcess(0);
    }

    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

    // Give back the memory that starting up used. Nearly all of it is data of
    // Windows' own DLLs, which the loop below never touches again: Task Manager
    // shows about 0.25 MB instead of 2 MB. -1, -1: drop as many pages as possible.
    SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));

    // Sleep until you would reach the idle limit, or until told to stop.
    DWORD wait = kLimitMs;
    while (WaitForSingleObject(stop, wait) == WAIT_TIMEOUT) {
        DWORD idle = IdleMs();
        if (idle >= kLimitMs) {                // you are idle: nudge, check again later
            Jiggle();
            wait = kLimitMs;
        } else {
            wait = kLimitMs - idle;            // not yet: sleep until you would be
        }
    }
    ExitProcess(0);                            // Windows frees everything, sleep allowed again
}
