# Mastervolt HID CAN Gateway

STM32/PlatformIO firmware for a Mastervolt-compatible HID CAN gateway used with
MasterAdjust.

This project is an independent, unofficial MasterBus-compatible USB CAN gateway.
It is not affiliated with, endorsed by, or sponsored by Mastervolt. Mastervolt
and MasterAdjust are trademarks of their respective owners.

## Compatible boards

| Board | Chip | Link |
|-------|------|------|
| CANable V1.0 (STM32F072CB variant) | STM32F072CB | [AliExpress](https://aliexpress.com/item/1005005721849902.html) |

The project builds with the custom PlatformIO board definition in `boards/canable_f072cb.json`.

## Build

The default PlatformIO environment is `canable_gateway_f072cb`.

```powershell
pio run -e canable_gateway_f072cb
```

## Flash

### With PlatformIO

Put the board into STM32 DFU mode, then run:

```powershell
pio run -e canable_gateway_f072cb -t upload
```

### With STM32CubeProgrammer

Download the latest `firmware.bin` from the [Releases](../../releases/latest) page,
then flash it using [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html):

1. Hold the **BOOT** button on the board while plugging in USB to enter DFU mode.
   The board should appear as `STM32 BOOTLOADER` in Device Manager.
2. Open STM32CubeProgrammer and select **USB** as the connection type.
3. Click **Connect**.
4. Go to the **Erasing & Programming** tab.
5. Under **File Path**, browse to `firmware.bin`.
6. Set **Start address** to `0x08000000`.
7. Check **Verify programming** and **Run after programming**.
8. Click **Start Programming**.
9. Unplug and replug the board — it will boot the new firmware.

## Notes

MasterAdjust depends on non-obvious HID report sideband metadata during the
startup scan. The firmware intentionally preserves the reserved tail bytes,
per-frame metadata bytes, and startup context frames observed from the original
gateway capture.
