# Prince of Persia Classic for ARM Linux

This source builds a data-free PortMaster release package for the Android 1.0 version of Prince of Persia Classic. The original game remains the property of its rights holders. No APK, OBB, game library, or extracted asset is included in the ZIP.

## Game files

Use your own matching Android 1.0 files:

- APK under any `.apk` filename, SHA-256 `8691634981b8b79266e73af13c35fc47ab0584fb620b5c5d5f745cb4e6d2372e`
- `main.1.org.ubisoft.premium.POPClassic.obb`, SHA-256 `80cd03b025806e49c98503628c976812faee4a89d7bd75761f0f8b604226817e`

For a local development run, put both in `gamedata/` and import them:

```sh
python3 tools/eapx.py check --recipe package/popclassic/popclassic.eapx.json
python3 tools/eapx.py install --recipe package/popclassic/popclassic.eapx.json --game-dir build/extracted --input gamedata/original.apk --input gamedata/main.1.org.ubisoft.premium.POPClassic.obb --no-adopt
```

## Build the PortMaster release ZIP

Install Docker Desktop with Linux containers enabled. From this source directory in PowerShell, run:

```powershell
.\build.ps1
```

The script builds the ARM loader and required libraries in Docker, then creates `dist/popclassic.zip`. Use `.\build.ps1 -NoCache` to rebuild the Docker image without its cache. The ZIP root contains only `Prince of Persia Classic.sh` and `popclassic/`; metadata and runtime files are inside `popclassic/`. The package includes a 640×480 cover and a separate 640×480 gameplay screenshot. Building and packaging do not need game files. The package script rejects proprietary game data in the stage. The port uses the device's SDL2 and graphics stack; other required ARM libraries are bundled.

## Install and test on RG34XX SP

Install `dist/popclassic.zip` through PortMaster. Copy the matching APK and OBB into `ports/popclassic/gamedata/` on the SD card. The APK can keep its original filename; the OBB needs the exact filename above. Launch the port from Ports. The first launch verifies and extracts the files on the handheld and reports progress through PortMaster, with a console display as a fallback. Keep the port running and allow time and free space for setup. Saves are kept in `ports/popclassic/saves/`, and the log is `ports/popclassic/log.txt`.

Display resolution is detected automatically. For a manual override, put `720x480` in `ports/popclassic/resolution.txt`. Rendering requests adaptive VSync when the driver supports it, then regular VSync; if neither is available, it uses a 60 FPS timer cap.

Detailed debug logging is on by default for now. To disable it, edit `Prince of Persia Classic.sh` and change `DEBUG_LOGGING=1` to `DEBUG_LOGGING=0`; change it back to `1` to re-enable it. The setting applies on the next launch. Debug mode adds loader and controller-input traces to `ports/popclassic/log.txt`; normal warnings and errors are still recorded when debug mode is off.

### RG34XX SP controls

The RG34XX SP prints its face buttons in a Nintendo layout, so the Vita control scheme maps by button position:

| RG34XX SP button | Vita equivalent | Action |
|---|---|---|
| B (bottom) | Cross | Confirm focused menu item; jump/attack; skip a cutscene |
| Y (left) | Square | Crouch; defend/parry in combat |
| X (top) | Triangle | Interact; sheath sword in combat |
| A (right) | Circle | Crouch/down |
| D-pad / left stick | D-pad / left stick | Move; hold left/right to run; up jumps/climbs; down crouches/climbs down |
| Start | Start | Pause/back; skip a cutscene |
| Select | Select | Toggle mouse mode and show/hide the cursor (off at startup) |
| L1 + R1 | Shoulder keys | Toggle the Android touch overlay; both shoulder key events are still sent |
| L2 + R2 | Trigger keys | Also toggle the Android touch overlay; both trigger key events are still sent |
| Right stick | — | Move the cursor while mouse mode is enabled; B (bottom) or A (right) clicks |

At startup, the port calls the game's native `SetControlInVisible()` method after game initialization and once after the first rendered frame, so the touch controls start hidden. Press **L1+R1** together or **L2+R2** together to toggle the overlay's visibility; both inputs in the chosen pair still send their Android key events. The mouse cursor stays hidden unless Select enables mouse mode. D-pad navigation followed by B (bottom) confirms the highlighted menu item. In mouse mode, move the cursor with the right stick and press either B (bottom) or A (right) to click; press Select again to hide the cursor. Confirm both overlay shortcuts on the RG34XX SP.

### Rocknix on RG DS

The launcher detects an active Wayland session and prefers its 32-bit Mesa EGL/GLES stack, as required by Rocknix's Sway/Panfrost setup. It checks that the Mesa driver and SDL graphics context load, and retries the driver with matching firmware libraries if the bundled C++ runtime masks a newer Mesa dependency. The launcher preserves Rocknix's real `libmali.so.1` name for its GBM hook. Input accepts both SDL game-controller mappings and raw joystick buttons, hats, and axes when the firmware has no complete game-controller mapping. This compatibility path is built in, but still needs RG DS/Rocknix device testing.

## Local validation

With owned 1.0 data, the ARM32 loader reached the main menu and first level at 720x480 under QEMU; MP3 and all three MP4/M4A audio assets decoded; a clean run created saves and a second run read them; shutdown exited cleanly. A short QEMU input test confirmed both shoulder-pair shortcuts alternate the game's native overlay visibility and preserve the individual key events. Adaptive pacing, PortMaster extraction progress, the launch debug toggle, handheld graphics, and controller behavior still need device testing. See `STATUS.md` and `package/testing_thread.txt`.

Porter: Pixelforge Ports (Ronax). Copyright (c) 2026 Pixelforge Ports contributors. Component licenses are under `package/popclassic/licenses/`.
