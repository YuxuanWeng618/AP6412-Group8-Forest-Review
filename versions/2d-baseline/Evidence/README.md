# Verification record

Date: 2026-11-12 (Asia/Hong_Kong). Engine: UE 5.8.1 CL 56057345. Compiler: MSVC 14.51.36260; Windows SDK 10.0.28000.0. UE reports that this compiler is newer than its preferred version; the recorded builds succeeded.

## Completed checks

- Unreal Editor Development compilation succeeded.
- `/Game/Maps/Forest` was created and saved through Unreal Editor's Python scripting interface.
- Three UE automation suites passed: `OrderAndInvalidActions`, `MovementPauseAndItems`, `CompleteOrderWithoutTeleport`. Raw exported report: `automation-report.json`.
- The full order also passed in a rendered UE game process, using the same movement and interaction methods as gameplay: 75 replay steps, final stage 9, zero teleports. Result: `editor-order-replay.json`.
- `start-menu.png`, `forest-world.png` and `order-complete.png` were captured by Unreal's screenshot system and visually inspected. These are actual runtime frames, not mockups.
- Win64 Development BuildCookRun completed successfully (exit code 0). The packaged executable independently completed the same 75-step order with zero teleports and exited with status 0. Result: `packaged-order-replay.json`. No editor process or UnrealSharp plugin was required by this executable.

The complete-order test originally reported failure after successful delivery because it waited for a movement waypoint after the game had ended. Its terminal condition was corrected; the full suite was then rerun and passed. No expected gameplay outcome was weakened.

The replay operates at the planar input adapter and bypasses the physical keyboard and window-focus checks. The desktop capture helper timed out twice, so physical two-player keyboard testing is not claimed. Keyboard rollover and another-machine execution remain manual acceptance items. This record concerns only the new 2D version; it does not certify the existing 3D game.
