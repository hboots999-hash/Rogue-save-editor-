# PKSM Rogue Edition — experimental Phase 7 CI

This branch contains a **source-only, not-for-installation** integration experiment.

- Upstream: [FlagBrew/PKSM](https://github.com/FlagBrew/PKSM), pinned at `9cad15694c1e90cc058ddd285eb68d56c7ad9ac8`.
- The proprietary/game ROM and personal save files are **not** included.
- The 7 `archive/*.b64` files form one XZ-compressed TAR of Phase 7 code only, required to be verified as SHA-256 `9d1877a76942d90c5579015349455462bfcc6121469536cccce6346c0a01ed63` before extraction.
- CI clones PKSM into a disposable checkout, verifies a clean matching revision and required build tools, applies the separate Rogue inspect screen, and attempts a native 3DS compilation. It never accesses an installed game save.
- The experimental fork uses unique ID `0xF73A2` rather than stock PKSM `0xEC100` or the old RogueSM `0xF73A1`. This does not prove an installed-title ID is globally safe.
- CI logs and experimental outputs, if generated, are stored as a 7-day artifact named `phase7-EXPERIMENTAL-NOT-FOR-INSTALL`. No GitHub release is created.
- Compile success alone does **not** establish startup stability, emulator or hardware compatibility, Emerald Rogue ROM compatibility, or safe save restore.

**Do not install experimental CI outputs on a 3DS or use them to restore an edited save.** A real emulator launch test, exact ROM/save-layout validation, and 3DS recovery plan are required first.
