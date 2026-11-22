# Modular Dongle Platform — Architecture RFC v0.1

Status: implementation baseline / draft  
Branch: architecture/dongle-modular-v1  
Firmware base: 0e82b8fb198088c9242cad7fb341e83573da65dc

## 1. Objective

A supported split-board shield must be able to consume the same reusable dongle capabilities without embedding board identity or MCU assumptions in shared modules. A supported dongle controller must be replaceable independently of the shield.

The compatibility model is two-dimensional:

- **Board contract:** physical matrix, keymap, split role, peripheral count, board identity, optional peripherals and board-specific behaviour bindings.
- **MCU/platform contract:** SoC, board target, GPIO/I2C/SPI/UART controllers, power/battery sensors, flash/storage and available hardware capabilities.
- **Shared modules:** generic state, protocol, display model/widgets, reusable behaviours and Studio compatibility. They consume declared interfaces; they do not select a board or MCU.

“Pluggable” means a board supplies the required board contract and selects compatible optional capabilities. It does not mean hardware-specific facts can be inferred or omitted.

## 2. Non-negotiable rules

1. No shared module may hardcode a keyboard/shield name, key position, matrix size, GPIO number, polarity, MCU family, OLED controller, bus instance or physical wiring.
2. Board adapters and MCU adapters are separate. A shield must not select a dongle MCU; an MCU adapter must not know the shield identity.
3. Optional modules must remain optional. A display must not be required for battery reporting; Studio must not be required for normal typing; charging detection must not be required for battery percentage.
4. Every dependency is explicit and pinned to an immutable revision or release.
5. Each module owns its CMake, Kconfig, devicetree bindings, source, metadata, tests and usage documentation.
6. Public interfaces are versioned. Unsupported capabilities are represented as unavailable/unknown, never guessed.
7. Existing known-good releases are read-only references. Integration happens on isolated branches.
8. A successful compile is necessary but not sufficient: configuration, event routing, runtime behaviour and artifact contents must be checked.
9. No generated build or test result may be described as passed unless it was actually executed and its logs inspected.

## 3. Responsibility boundaries

### 3.1 Firmware compatibility layer

Owns compatibility with the selected ZMK/Zephyr API surface and shared integration hooks. It must not contain shield-specific wiring or a second implementation of functionality already provided by ZMK without a documented gap.

### 3.2 Split identity and connection state

Owns generic peripheral identity, connection lifecycle and stable mapping between a peripheral and its declared logical identity. It must not derive left/right identity from connection order. Identity must be validated, duplicate identities rejected or quarantined, and disconnected state represented explicitly.

Physical side identifiers are configuration data supplied by a board/peripheral contract. They are not keyboard names and do not depend on MCU type.

### 3.3 Battery state and transport

Prefer existing ZMK battery sensor, battery state event and split transport facilities. Add only missing generic capabilities.

Separate these concepts:

- Battery percentage / state of charge.
- Battery presence or measurement availability.
- Charging state.
- USB/VBUS presence.
- Source identity (central or a validated split peripheral).

A missing charging sensor is unsupported or unknown, not not-charging. A disconnected peripheral battery is unknown/unavailable, not zero percent. The UI must not interpret a USB-powered voltage rail as proof that a battery is installed.

### 3.4 Display model and widgets

Owns a hardware-neutral model of status data and reusable widgets. It consumes connection, battery, USB, layer, WPM and other events through public interfaces.

It must not read GPIOs, probe a battery sensor, own split identity discovery, or assume a specific MCU, keyboard, OLED controller or wiring.

### 3.5 Display hardware adapters

Own controller/bus/panel configuration, reset pins, dimensions, orientation, offsets, supported pixel formats and device readiness. Resolution-specific layout/renderers (for example 128×32 and 128×64) are allowed, but must not be named after a keyboard.

A missing display is a valid build configuration.

### 3.6 Reusable behaviours

Own generic behaviour implementation and bindings only. Physical positions and keymap policy belong to the board's devicetree/keymap. A behaviour module must register once and expose one unambiguous Studio metadata representation.

### 3.7 Studio compatibility

Keep frontend, TypeScript client, message definitions and firmware RPC implementation separately versioned but governed by a compatibility matrix. Message definitions must be pinned during generation; package installation must not fetch an unpinned moving branch.

Studio runtime editing must not silently rewrite firmware-defined tapping terms unless a user explicitly changes a supported timing property. The first architecture release will preserve firmware timing as the authority.

For the first platform profile, ZMK_BEHAVIOR_RUNTIME_EDITING is explicitly disabled by default even when Studio RPC is enabled. Combo runtime editing remains independently selectable. This preserves the firmware-defined hold-tap configuration and prevents Studio from mutating tapping-term values during ordinary remapping.

The selected firmware branch pins zmk-studio-messages to commit e416c02ad1a431de504270ed2477190a0ab5d5c2. The matching TypeScript client work branch uses the same submodule commit and no longer fetches the moving main branch during postinstall.

## 4. Board and MCU adapter contracts

A board configuration declares only facts it owns:

- split role and topology;
- logical peripheral identity/side, when applicable;
- matrix/keymap and physical layout;
- enabled shared features;
- optional board-level behaviours and bindings;
- references to required hardware capability providers.

An MCU/platform adapter declares only facts it owns:

- Zephyr board/SoC target and supported toolchain;
- GPIO, I2C, SPI, UART and other controller availability;
- battery measurement provider and measurement method, if present;
- charging/USB detection provider and its validity semantics, if present;
- display controller and bus wiring, if present;
- flash/settings/storage capabilities.

A shared module depends on a capability interface, not on a board or MCU label. If two MCU adapters expose the same capability contract, the consumer should not need conditional logic for those MCU families.

Capability states must distinguish at least: present and operational, present but unavailable/error, unsupported, and not configured. Do not collapse these into boolean false.

## 5. Build composition

The consuming firmware configuration selects:

1. one board configuration;
2. one MCU/Zephyr board target;
3. a set of optional shared modules;
4. zero or more hardware capability adapters;
5. a pinned compatible firmware/platform revision.

The build must fail early with a clear diagnostic when a selected feature has a required capability that is not supplied. Optional features must not be pulled in transitively unless the dependency is declared and documented.

Every module must provide:
- Zephyr module metadata;
- CMake and Kconfig integration;
- devicetree bindings where needed;
- an isolated sample or test target;
- declared required/optional dependencies;
- supported ZMK/Zephyr compatibility range;
- immutable release/revision instructions.

## 6. Initial compatibility matrix

| Board family | Split MCU | Dongle MCU | Initial role |
|---|---|---|---|
| Hana | XIAO nRF52840 | Pro Micro / nice!nano v2-compatible nRF52840 | First integration target |
| Chocofi | Existing supported split MCU | Pro Micro / nice!nano v2-compatible nRF52840 | Second board integration |
| Sweep | Existing supported split MCU | Pro Micro / nice!nano v2-compatible nRF52840 | Follow-up integration |
| Sofle | Existing supported split MCU | Pro Micro / nice!nano v2-compatible nRF52840 | Follow-up integration |
| Totem | XIAO-based configuration | Compatible dongle target selected by build | Portability qualification |
| Future boards | Declared by board adapter | Any compatible MCU adapter | No shared-module changes for board identity alone |

The table is a target matrix, not a claim that every combination has already been built or validated. Exact board target identifiers and hardware capabilities must be recorded from each configuration before build jobs are enabled.

## 7. Regression controls for known failure areas

### OLED / display
- Build no-display, 128×32 and 128×64 variants independently.
- Check panel dimensions, orientation, offsets, font metrics and clipping.
- Verify the UI does not silently squeeze a 64-pixel layout into a 32-pixel panel.
- Verify display absence does not break non-display features.

### Bluetooth identity and connection state
- Validate identity before assigning physical side.
- Never infer side from BLE connection order.
- Reject duplicate/invalid identities and clear identity on disconnect/reconnect.
- Verify a connection event cannot be mistaken for a keyboard identity or active-device selection event.

### Battery and charging
- Test central battery, each peripheral battery, disconnected peripheral and unknown source.
- Test unsupported charging detection.
- Test USB-powered dongle with and without a physically present battery where hardware permits.
- Ensure stale battery values are cleared to unknown on disconnect.

### ZMK module integration
- Audit Kconfig symbol visibility/defaults, CMake source inclusion, devicetree compatible names/bindings, init ordering, event declarations/definitions and dependency metadata.
- Build with each optional feature both enabled and disabled.
- Check for duplicate symbols, duplicate behaviour registration and accidental module inclusion.

### Studio and bindings
- Verify each custom behaviour appears exactly once in Studio.
- Verify keymap binding parameters, runtime-editable pools and metadata are consistent.
- Verify protocol/client/firmware versions are pinned and compatible.
- Verify backup/restore and keymap import do not silently change behaviour timing.

### Hold-tap / tapping terms
- Preserve the selected firmware hold-tap implementation as the baseline.
- Capture all hold-tap devicetree properties and defaults in build artifacts.
- Test tap, hold, interruption, quick-tap, prior-idle, retro-tap and split-source cases.
- Compare timing configuration and behaviour against the baseline before and after Studio integration.
- Do not change tapping-term defaults or hold-tap decision logic as part of unrelated modularization.

The inspected Hana and Chocofi keymaps both currently declare balanced Mod-Tap with a 200 ms tapping term and 175 ms quick-tap. These values are preserved in the integration branches. Experimental keymap branches that change the flavor to hold-preferred are not silently incorporated into the architecture baseline.

## 8. Baseline selection

The initial shared firmware baseline is 0e82b8fb198088c9242cad7fb341e83573da65dc.

Evidence:
- Hana currently pins 88d03e9a30d0433375ded7c07c82e5a61f023fbb.
- Chocofi pins 0e82b8fb198088c9242cad7fb341e83573da65dc.
- The latter is 12 commits ahead of the Hana pin.
- The 12-commit delta adds split side identity, side-aware battery routing and related display/configuration support.
- The audited behavior_hold_tap.c and hold-tap devicetree binding are unchanged between these two revisions.
- Therefore the later revision is the better common starting point for Hana/Chocofi integration without changing the audited hold-tap implementation.

This is a controlled engineering baseline, not a declaration that every feature in the revision is regression-free. It must pass the qualification matrix before becoming a release baseline.

The Studio README separately recommends b19edf953498bf9d0a4598eb39d04e7e43339b08; that older recommendation must not be used to downgrade the hold-tap implementation or replace the later split identity work. Studio compatibility will be established against the selected firmware revision through explicit protocol checks.

## 9. Delivery order

1. Lock this RFC and the common firmware baseline on an isolated branch.
2. Extract the display model from hardware-specific display and charging code.
3. Define connection identity and battery capability interfaces using existing ZMK events/transport wherever possible.
4. Add the first MCU adapters for XIAO nRF52840 and Pro Micro/nice!nano-compatible targets.
5. Add minimal dongle build profiles: no display, 128×32 and 128×64.
6. Add board integration profiles for Hana and Chocofi.
7. Add compile-time configuration checks and automated build matrix.
8. Qualify Studio and hold-tap compatibility before enabling runtime editing.
9. Port Sweep, Sofle and Totem only after the first two board integrations pass.

## 10. Acceptance criteria

The first architecture milestone is complete when:
- Hana and Chocofi consume the same shared module revisions;
- each can be paired with the supported dongle MCU adapter without board-specific shared-module code;
- display, battery, identity and Studio features are independently selectable;
- no-display and both OLED resolutions build independently;
- hold-tap source and timing configuration match the selected baseline;
- build artifacts and logs prove the matrix passed;
- all dependencies are immutable and documented.

No stable branch, tag, release or existing firmware artifact is changed by this RFC.
