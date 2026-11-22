# Split Peripheral Physical Identity Contract

This contract is the transport-level source of physical LEFT/RIGHT identity for the shared dongle core. Connection order and BLE source indexes are transport details only; they are never physical identity.

## GATT characteristic

- Service: existing ZMK split service
- Characteristic UUID suffix: `00000007` in the existing ZMK split 128-bit UUID family
- Properties: encrypted read
- Payload: exactly one byte

| Value | Meaning                  |
| ----: | ------------------------ |
|     0 | Unknown / not configured |
|     1 | Physical LEFT half       |
|     2 | Physical RIGHT half      |

A peripheral reports the value configured by `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_SIDE`. Dongle-side peripheral builds must explicitly set 1 or 2. The default is 0 so unconfigured firmware cannot silently claim a side.

## Central API and event

- `zmk_split_central_get_peripheral_side(source, &side)` returns `-EAGAIN` until the identity read completes and the peripheral is connected.
- `zmk_dongle_peripheral_identity_changed` carries the transport source, physical side, identity validity, and connection state.
- On disconnect, the event is raised before the slot's identity is cleared.
- Invalid or absent identity is not replaced with connection-order inference.

## Consumer requirements

The dongle core must:

1. Maintain state by physical side, not by source index.
2. Reject or quarantine duplicate simultaneous LEFT or RIGHT claims.
3. Keep an unknown-side peripheral out of the LEFT/RIGHT battery display slots.
4. Clear connected state on disconnect while treating any retained battery value as stale.
5. Distinguish unavailable battery data from a valid 0% reading.

## Keyboard configuration

The peripheral side is configured by the keyboard build, not inferred from the keymap or connection order:

- LEFT peripheral: `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_SIDE=1`
- RIGHT peripheral: `CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_SIDE=2`

All peripheral firmware participating in a dongle pair must include this characteristic. Old peripherals without the characteristic remain usable for split input, but their physical side is unknown and they must not be used for side-specific battery reporting.
