# Mastervolt HID CAN Gateway

STM32/PlatformIO firmware for a Mastervolt-compatible HID CAN gateway used with
MasterAdjust.

This project is an independent, unofficial MasterBus-compatible USB CAN gateway.
It is not affiliated with, endorsed by, or sponsored by Mastervolt. Mastervolt
and MasterAdjust are trademarks of their respective owners.

## Hardware

This firmware was successfully tested on a CANable V1.0 board from AliExpress:
https://fr.aliexpress.com/item/1005005721849902.html

The tested board uses an STM32F072CB-class MCU and the project builds with the
custom PlatformIO board definition in `boards/canable_f072cb.json`.

## Build

The default PlatformIO environment is `canable_gateway_f072cb`.

```powershell
pio run -e canable_gateway_f072cb
```

## Flash

Put the board into STM32 DFU mode, then run:

```powershell
pio run -e canable_gateway_f072cb -t upload
```

## Notes

MasterAdjust depends on non-obvious HID report sideband metadata during the
startup scan. The firmware intentionally preserves the reserved tail bytes,
per-frame metadata bytes, and startup context frames observed from the original
gateway capture.
