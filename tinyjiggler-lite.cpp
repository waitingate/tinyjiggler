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
// Build (64-bit MinGW-w64 g++, no C runtime):
//   g++ -Os -s -nostdlib -nostartfiles -e Start -Wl,--subsystem,windows
//       -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-stack-protector
//       -o TinyJigglerLite.exe tinyjiggler-lite.cpp -lkernel32 -luser32 -lmsvcrt

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---- Settings: change and rebuild ----
constexpr DWORD kIdleSeconds = 30; // jiggle after this long idle; keep below your screensaver/lock time
constexpr int   kMovePixels  = 4;  // size of the nudge; 0 = Zen mode (pointer never moves)

constexpr DWORD kLimitMs = kIdleSeconds * 1000;

// Milliseconds since the last keyboard/mouse input (yours or ours).
// If the call fails, dwTime stays 0, which counts as idle: it keeps working.
static DWORD IdleMs()
{
    LASTINPUTINFO li{sizeof(LASTINPUTINFO), 0};
    GetLastInputInfo(&li);
    return GetTickCount() - li.dwTime;
}

static void Jiggle(int dir)
{
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;          // relative move; 0,0 still counts as input
    in.mi.dx = in.mi.dy = kMovePixels * dir;
    SendInput(1, &in, sizeof(in));
}

// No C runtime: Windows starts the program here.
extern "C" void Start()
{
    // One named "stop" signal shared by every copy. If it already exists,
    // another copy is running: tell it to stop, and we are done.
    HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, L"Local\\TinyJigglerLite");
    if (!stop) {
        MessageBoxW(nullptr, L"TinyJiggler failed to start (could not create stop event).",
                    L"TinyJiggler", MB_ICONERROR);
        ExitProcess(1);
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        SetEvent(stop);
        CloseHandle(stop);
        MessageBoxW(nullptr, L"TinyJiggler stopped.", L"TinyJiggler", MB_ICONINFORMATION);
        ExitProcess(0);
    }

    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

    // Sleep until you would reach the idle limit, or until told to stop.
    DWORD wait = kLimitMs;
    int dir = 1;                               // nudge direction; flips so the pointer comes back
    while (WaitForSingleObject(stop, wait) == WAIT_TIMEOUT) {
        DWORD idle = IdleMs();
        if (idle >= kLimitMs) {                // you are idle: nudge, check again later
            Jiggle(dir);
            dir = -dir;
            wait = kLimitMs;
        } else {
            wait = kLimitMs - idle;            // not yet: sleep until you would be
        }
    }
    ExitProcess(0);                            // Windows frees everything, sleep allowed again
}
