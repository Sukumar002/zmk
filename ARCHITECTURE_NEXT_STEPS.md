# Post-baseline implementation queue

**Gate:** Do not begin feature integration until baseline commit `9625030f59c432a301b1118895edecd4baee41a3` has passing pre-commit checks and the firmware Tests workflow has completed successfully. BLE Tests and Docs Checks must remain green.

**Working branch:** `architecture/studio-mouse-combo-backend`

## Queue

- [ ] **1. Repair baseline CI failures.** Reproduce the `clang-format` and `prettier` changes reported by pre-commit, inspect the complete diff, commit only intended formatting changes, and rerun checks. Confirm the queued firmware Tests jobs complete successfully.
- [ ] **2. Port mouse settings backend.** Pin the custom Studio messages dependency to an immutable commit containing `pointing.proto`; add nanopb generation/CMake integration; port settings storage/API and secured `pointing.getSettings` / `pointing.setSettings` RPC handlers; wire move speed, scroll speed, and acceleration into `behavior_input_two_axis.c`; verify validation, persistence, defaults, and settings reset.
- [ ] **3. Restore combo Studio RPC compatibility.** Integrate `combos.proto` generation and the combo RPC subsystem; verify get/set/add/remove, save/discard, unsaved-change notifications, and settings reset against the current combo implementation and exact protocol schema.
- [ ] **4. Audit runtime-editable metadata.** Keep mouse settings on the dedicated pointing RPC path. Remove runtime-editable typing/timing exposure and the Bluetooth profile long-hold duplicate entry only after checking all consumers. Preserve firmware-owned Mod-Tap/Layer-Tap timing and keymap-assignable display/halves timeout cycling.
- [ ] **5. Validate firmware.** Build the Chocofi central/peripheral targets against the pinned dependency. Inspect every failing job's logs. Do not treat partial artifact presence as a successful matrix.
- [ ] **6. Validate display module.** Only after firmware validation, update `Sukumar002/zmk-dongle-display` to the verified firmware SHA; validate physical LEFT/RIGHT battery-row mapping and both display resolutions; commit and run its CI.
- [ ] **7. Validate Chocofi integration.** Only after display validation, update the Chocofi manifest to the validated firmware and display SHAs; run the seven-target matrix: settings reset, central-left, peripheral-right, peripheral-left, central dongle, 128x32 dongle, and 128x64 dongle.

## Guardrails

- Chocofi remains the only validation target in this phase.
- Keep implementation isolated on the architecture branch and existing draft PRs.
- No merges, releases, default-branch changes, or expansion to other keyboards without explicit approval.
- Do not claim a feature works until code inspection and relevant CI/device validation support it.
