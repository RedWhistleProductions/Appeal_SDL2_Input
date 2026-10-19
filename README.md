# Appeal_SDL2_Input

An Appeal plugin project.

## Development status

This repository builds the SDL2 input backend as an Appeal plugin shared library.

## Developer dependencies

Third-party libraries are not stored in this repository.

- Windows: run `powershell -ExecutionPolicy Bypass -File Scripts/Setup-Dependencies.ps1`. It downloads a pinned MinGW SDL2 development archive, verifies its SHA-256 checksum, and installs it under the Git-ignored `.deps` directory.
- Debian, Fedora, or Arch Linux: run `bash Scripts/setup-dependencies.sh`. It installs the distribution's SDL2 development package.

Dependency versions, official URLs, checksums, and package names are recorded in `dependencies.json`.

## Distribution

The Windows installer downloads and verifies the SDL2 runtime during installation. The Debian package declares SDL2 as a runtime dependency so the system package manager downloads it.

Generated binaries and intermediate objects are intentionally excluded from version control.

## Install from a Windows source checkout

The plugin should be under `Resources/Modules/Appeal_Module_Input/Appeal_SDL2_Input`.
From the Appeal root, reuse the SDL2 development files already used by Graphics:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Resources/Modules/Appeal_Module_Input/Appeal_SDL2_Input/Packaging/Windows/Install.ps1 -Build -DevelopmentRoot "$PWD/Resources/Modules/Appeal_Module_Graphics/Appeal_SDL2_Graphics/.deps/windows-x64"
```

Without DevelopmentRoot, the build uses existing local `.deps` files or runs
Scripts/Setup-Dependencies.ps1 for the pinned dependency. This checkout was
built and tested with SDL2 2.32.8 from Graphics, rather than downloading the
manifest's older 2.24.2 development package. The installer preserves an existing
shared SDL2.dll beside Appeal.exe and never replaces it with an older copy.
Use HostDirectory and AppealResources to select another host installation.
The parent Input module must also be built as Appeal_Module_Input.dll.

Enable it in Setup/Config.Appeal:

```text
Add_Dynamic_Module Input Appeal_SDL2_Input
```

Call `Input Update Default` once each frame to refresh state and dispatch bound
callbacks. It can run before or after Graphics Update. Input observes wheel and
controller events through an SDL event watch, leaving the event queue available
to Graphics/ImGui; it does not consume window-close or GUI events. Input alone
does not create a window: keyboard focus comes from the Graphics window.

```text
Define_Function Jump
    Print "Space pressed"
End_Function
Input Set_Key_Down Space Jump
// Inside the application's render/event loop:
Input Update Default
Graphics Render
Graphics Update
```

In the parent Appeal checkout, `Appeal.exe Test/Input_Install_Smoke` verifies
module/plugin loading, queries, and shutdown with a Graphics window. Run
`Tests/SDL2_Input_Events.ps1` for deterministic shared-event queue checks.
These checks do not establish physical keyboard, mouse, or gamepad operation.

## Axis mapping, dead zone, and button repeat

Controller axis queries and callbacks now use integer values from -100 to +100
instead of raw SDL values (-32768..32767). This also applies to bound Axis queries
and Set_Controller_Axis_Range thresholds. Triggers use the nonnegative portion.

`Input Set_Controller_Deadzone Percent` accepts 0..99 and returns 1 on success,
0 for an invalid setting without changing the previous value. Default: 24.
The percentage is measured from the center separately on each axis. Values
inside it map to zero; remaining travel is linearly rescaled to the full output
range. Analog callbacks run each Input Update while nonzero, and once with zero
when returning to center. The dead zone currently applies to all controllers.

Repeat is independently configurable and defaults to off:

```text
Input Set_Key_Repeat W 1
Input Set_Mouse_Repeat Left 1
Input Set_Controller_Button_Repeat DPad_Up 1
Input Set_Controller_Button_Repeat_For 0 DPad_Up 1
```

Use the same key/button spelling as its Down binding; pass 0 to disable repeat.
These commands affect Down callbacks only. Pressed/Released queries and Up
callbacks keep their original edge semantics. A press fires immediately;
enabled held bindings repeat at most once every 100 ms during Input Update.
There is no catch-up burst after a stalled frame. Global controller bindings and
controller-specific bindings have independent settings/timers; binding both can
intentionally dispatch both callbacks.

The combined example maps Adam's left stick directly into X/Y offsets using
Callback_Value, independently of his button speed slider. Adam's tab contains a
dead-zone slider and a D-pad repeat checkbox (initially off). Movement is still
per Input Update, not pixels per second; Mechanics/time-based speed adjustment
is deferred. At full deflection this is 100 pixels per update, so the character
can move offscreen quickly. No controller speed multiplier is applied yet.
