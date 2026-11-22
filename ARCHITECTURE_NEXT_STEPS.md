# Post-baseline implementation queue

**Gate:** Do not begin feature integration until baseline commit `9625030f59c432a301b1118895edecd4baee41a3` has passing pre-commit checks and the firmware Tests workflow has completed successfully. BLE Tests and Docs Checks must remain green.

**Working branch:** `architecture/studio-mouse-combo-backend`

## Queue

- [x] **1. Apply baseline formatter corrections.** The pre-commit diff was inspected; the exact `clang-format` changes in `central.h`, `central.c`, and `service.c`, plus the `prettier` Markdown table/list formatting, were applied on `architecture/dongle-core-contract` in commit `d3544292742623e076dd55096a63545842198346`. **Still gated:** wait for fresh pre-commit checks on this commit and completion of both firmware test runs. Docs Checks and BLE Tests passed on the earlier baseline; they must remain green.
- [ ] **2. Port mouse settings backend (after baseline CI gate).** The historical `hana-mouse-test` branch contains a usable reference implementation, but it must be treated as a port reference—not copied wholesale. Current architecture branch `app/west.yml` pins upstream Studio messages at `6cb4c283e76209d59c45fbcb218800cd19e9339d`; historical branch uses the custom messages fork's moving `hana-combo-names` branch. Resolve and pin an immutable custom messages commit that contains the exact `pointing.proto` schema expected by the deployed Studio frontend. Then:
  - Add `pointing.proto` to nanopb generation in `app/CMakeLists.txt`, and conditionally compile `pointing_subsystem.c` in `app/src/studio/CMakeLists.txt` when pointing is enabled.
  - Port `pointing_settings.c/.h`; review settings record version/length handling, bounds, persistence errors, defaults, and reset semantics rather than assuming the historical code is production-safe.
  - Port secured `pointing.getSettings` and `pointing.setSettings` RPC handlers plus pointing settings-reset registration.
  - Port the actual runtime application in `behavior_input_two_axis.c`: XY movement uses configured move speed and acceleration; wheel/hwheel uses configured scroll speed; unrelated two-axis behaviors retain their Devicetree-requested speed/acceleration. Ensure smooth-scrolling resolution multipliers remain authoritative where applicable.
  - Verify frontend field names/types/defaults against the pinned proto, RPC security, invalid values, persisted reload, reset-to-default, and live effect on cursor and scroll motion.
- [ ] **3. Restore combo Studio RPC compatibility.** The historical `hana-mouse-test` branch contains `combo_subsystem.c`; the current architecture branch does not compile it and does not generate `combos.proto`. Audit that implementation against the current `zmk_combos` API and exact custom frontend message schema before porting. Integrate schema generation and conditional subsystem compilation, then verify secured get/set/add/remove, validation and behavior local-ID mapping, save/discard, unsaved-change notifications, capacity reporting, and settings reset. Do not port unrelated combo storage/runtime changes as part of this RPC task.
- [ ] **4. Audit runtime-editable metadata.** Mouse settings are a dedicated pointing RPC path, not generic hold-tap metadata; the custom frontend panel calls `pointing.getSettings` / `pointing.setSettings`. The current firmware branch is missing that backend, so do not claim mouse runtime settings currently work. Keep Mod-Tap/Layer-Tap timing firmware-owned. Remove timing runtime exposure and the Bluetooth profile long-hold duplicate entry only after checking all firmware, protocol, keymap, and frontend consumers. Preserve mouse runtime settings and the keymap-assignable display/halves sleep-timeout cycling functions. Do not remove generic metadata declarations if any remaining consumers require them.
- [ ] **5. Validate firmware.** Build the Chocofi central/peripheral targets against the pinned dependency. Inspect every failing job's logs. Do not treat partial artifact presence as a successful matrix.
- [ ] **6. Validate display module.** Only after firmware validation, update `Sukumar002/zmk-dongle-display` to the verified firmware SHA; validate physical LEFT/RIGHT battery-row mapping and both display resolutions; commit and run its CI.
- [ ] **7. Validate Chocofi integration.** Only after display validation, update the Chocofi manifest to the validated firmware and display SHAs; run the seven-target matrix: settings reset, central-left, peripheral-right, peripheral-left, central dongle, 128x32 dongle, and 128x64 dongle.

## Parallel audit findings (no feature code changed)

- The historical `hana-mouse-test` implementation is the source reference for pointing settings and combo Studio RPCs; the current architecture branch lacks both RPC backends.
- Historical pointing settings defaults are move speed 600, scroll speed 10, acceleration 1; historical validation bounds are move 100–2000, scroll 1–50, acceleration 0–3. Treat these as reference values and confirm them against the deployed frontend before adopting them.
- Historical `behavior_input_two_axis.c` applies move speed only to `INPUT_REL_X/Y`, scroll speed only to `INPUT_REL_HWHEEL/WHEEL`, and configured acceleration to cursor movement. Preserve configured speed for unrelated two-axis devices and avoid overriding smooth-scroll resolution profiles.
- The custom Studio message dependency currently differs between branches: architecture uses upstream SHA `6cb4c283e76209d59c45fbcb218800cd19e9339d`; historical branch references the custom fork's moving branch `hana-combo-names`. Do not change the dependency until the exact schema commit is identified and pinned.
- The formatter-only commit above is the only new change made during this parallel audit. No firmware feature code, manifest pin, display module, keyboard configuration, PR base, or default branch was changed.

## Guardrails

- Chocofi remains the only validation target in this phase.
- Keep implementation isolated on the architecture branch and existing draft PRs.
- No merges, releases, default-branch changes, or expansion to other keyboards without explicit approval.
- Do not claim a feature works until code inspection and relevant CI/device validation support it.
