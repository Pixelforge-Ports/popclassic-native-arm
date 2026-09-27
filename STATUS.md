# Prince of Persia Classic release package status

Local ARM32/QEMU checks with the owned Android 1.0 APK and OBB passed at 720x480:

- Main menu, story, and first playable level rendered.
- The earlier QEMU input run checked menu progression, movement, jump, crouch, and pause using the previous mapping. The face-button layer now forwards Android gamepad keys in the Xperia PLAY/Vita scheme; that updated mapping has not been retested on hardware.
- MP3 menu music/effects and each of the three MP4/M4A audio tracks decoded to PCM. The local audio driver was a dummy device, so physical audio output still needs testing. The launcher now supplies ARM32 PipeWire/SPA/ALSA module paths and a `plug:dmix` fallback. SDL audio failures no longer stop the game; both working and unavailable audio drivers passed focused QEMU checks.
- Fresh saves were created in an isolated directory; a second launch reopened `pop_save_normal` and `pop_save_profile`.
- Shutdown ran the guest pause callback, flushed save files, and exited with code 0. The original guest static destructors corrupt the host heap, so the adapter uses `quick_exit` after cleanup.
- The 720x480 test ran to 850 frames without a process fault.

The source stages a PortMaster release package with a 640×480 cover and a separate 640×480 screenshot. First-boot extraction uses the same `pm_input`/`pm_done` channel and import message as the working N.O.V.A. 2 port. The input adapter calls the APK's exported `SetControlInVisible()` method after game initialization and after the first rendered frame. **L1+R1** and **L2+R2** each toggle the overlay through the APK's matching hide/show methods while preserving the paired Android key events. The QEMU input test verified each pair alternates the game's native hide/show calls and still forwards key codes 102/103 or 104/105; RG34XX SP behavior still needs hardware verification. Rendering requests adaptive VSync, then regular VSync, with a 60 FPS timer fallback. Detailed traces are enabled by default for now; set `DEBUG_LOGGING=0` in `Prince of Persia Classic.sh` to disable them. Check extraction progress, default overlay visibility, both overlay-toggle pairs, controls (including jump, crouch, pause, and quit), audio output, saves after a full level, frame pacing, and graphics. Video scenes are skipped by invoking the game's completion callback; cinematic playback is not implemented.

Detailed traces are enabled by default for now. Change `DEBUG_LOGGING=1` to `DEBUG_LOGGING=0` in the launcher to turn them off; set it back to `1` to enable them again.

The RG34XX SP log after the first audio change showed `plug:dmix` failing, then SDL selecting the HDMI device, followed by an EGL/GL library-load failure. The launcher now uses the default PCM when PipeWire is present, avoids selecting HDMI as an automatic audio fallback, and locates the device's ARM32 EGL/GLES libraries. Graphics and speaker audio still require device verification.
