# Prince of Persia Classic - PortMaster release package

This is the data-free ARM32 PortMaster package for RG34XX SP and compatible Linux handhelds. It requires your own matching Android 1.0 game files. The ZIP contains no APK, OBB, or extracted game data. It includes a 640×480 cover image and a separate 640×480 screenshot.

After installing `popclassic.zip` through PortMaster, put these files in `ports/popclassic/gamedata/` on the same SD card:

- Your APK, under any `.apk` filename (SHA-256 `8691634981b8b79266e73af13c35fc47ab0584fb620b5c5d5f745cb4e6d2372e`)
- `main.1.org.ubisoft.premium.POPClassic.obb` (SHA-256 `80cd03b025806e49c98503628c976812faee4a89d7bd75761f0f8b604226817e`)

Launch **Prince of Persia Classic** from Ports. On first boot, the package verifies and extracts the files on the handheld. Progress is sent to PortMaster's progress display, with the selected console as a fallback; leave the port running until setup completes. Allow several minutes and enough free space for the extracted files. Later launches use `ports/popclassic/donor/`. Saves are in `ports/popclassic/saves/`; the log is `ports/popclassic/log.txt`.

RG34XX SP controls follow the Xperia PLAY/Vita layout by physical button position: **B (bottom)** confirms a focused menu item, jumps/attacks; **Y (left)** crouches/defends; **X (top)** interacts/sheathes; and **A (right)** crouches. D-pad/left stick moves (hold left/right to run; Up jumps/climbs; Down crouches/climbs down); Start pauses/back; Select toggles mouse mode and shows/hides the cursor (off by default). The APK's native `SetControlInVisible()` method hides the Android touch overlay at startup. Press **L1+R1** together or **L2+R2** together to toggle the overlay; the paired Xperia PLAY key events are preserved. In mouse mode, use the right stick to move the cursor and either B (bottom) or A (right) to click. To override automatic display detection, write `720x480` in `ports/popclassic/resolution.txt`.

The game requests adaptive VSync when available, then regular VSync; if neither is supported, rendering uses a 60 FPS timer cap. Detailed debug logging is on by default for now. To disable it, edit `Prince of Persia Classic.sh` and change `DEBUG_LOGGING=1` to `DEBUG_LOGGING=0`; change it back to `1` to re-enable it. The setting takes effect on the next launch. Debug mode adds loader and controller-input traces to `ports/popclassic/log.txt`; standard warnings and errors are still recorded while it is off.

On Rocknix RG DS, the launcher detects Wayland and selects the firmware's 32-bit Mesa/Panfrost path, checks its graphics context, and keeps the firmware `libmali.so.1` available to Rocknix's GBM hook. It also handles raw joystick events when Rocknix does not provide a complete SDL game-controller map. This path needs a device test.

The port was locally tested under QEMU with the supplied 1.0 data at 720x480. PortMaster's first-boot progress display, revised frame pacing, and both shoulder/trigger pairs still need RG34XX SP hardware verification.

The launcher records the selected SDL audio driver, ALSA device, and ARM32 audio module paths in `ports/popclassic/log.txt`. If audio cannot open, the game continues without sound so the rest of the port can still be tested; report that log when testing audio on hardware.
