# TinyJiggler Lite

A tiny exe that stops Windows from sleeping or locking while you're away. No window, no tray icon, no settings.

## Using it

Download `TinyJigglerLite.exe` and double-click it. Nothing shows up. That's normal, it's running.

Double-click it again to stop it. You'll get a "TinyJiggler stopped." message.

Not sure if it's on? Open Task Manager, go to Details, and look for `TinyJigglerLite.exe`.

If you want it to start with Windows, press Win+R, type `shell:startup`, and drop a shortcut to the exe in that folder.

## What it actually does

First it tells Windows not to sleep or turn the screen off (`SetThreadExecutionState`). Then it keeps an eye on how long you've been idle. After 30 seconds with no keyboard or mouse input, it nudges the mouse 4 pixels, and 4 pixels back the next time, so the pointer doesn't drift off. That resets Windows' idle timer: no screensaver, no auto-lock, and apps that go by Windows idle time won't mark you as Away.

While you're using the PC it does nothing at all. It just sleeps until the earliest moment you could hit 30 seconds idle.

Stopping works by running a second copy. Both copies share a named event, so the new one tells the old one to quit, then quits too. Once it exits, Windows is allowed to sleep again.

## Changing the timing

The two settings are at the top of `tinyjiggler-lite.cpp`:

```cpp
constexpr DWORD kIdleSeconds = 30; // jiggle after this long idle
constexpr int kMovePixels = 4;     // size of the nudge, 0 = pointer never moves
```

Keep `kIdleSeconds` lower than your screensaver or lock timeout. Setting `kMovePixels` to 0 still works, because Windows counts a zero-pixel move as input. The pointer just stays put.

## Building

You need 64-bit MinGW-w64 g++ (MSYS2 works). It's built without the C runtime, which is why the exe is so small:

```
g++ -Os -s -nostdlib -nostartfiles -e Start -Wl,--subsystem,windows -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-stack-protector -o TinyJigglerLite.exe tinyjiggler-lite.cpp -lkernel32 -luser32 -lmsvcrt
```

## Good to know

- SmartScreen will probably complain the first time, since the exe isn't signed. Click "More info" then "Run anyway", or build it yourself.
- It won't stop you from locking the PC yourself with Win+L.
- If it's a work laptop, check your IT policy first.

## License

MIT, see [LICENSE](LICENSE).
