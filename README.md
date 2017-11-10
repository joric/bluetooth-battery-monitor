Bluetooth Battery Monitor
=========================

Open source Bluetooth battery monitor for Windows

## Enumerating Bluetooth devices and services

I'm not using UWP, only classic C/C++ API (check out [misc](tree/master/misc) directory for more C++ examples).

### API calls:

* `SetupDiGetClassDevs` / `SetupDiEnumDeviceInfo` for getting a list of all bluetooth devices
* `SetupDiGetDeviceRegistryProperty` / `SPDRP_HARDWAREID` for filtering by device address
* `BluetoothGATTGetServices` / `BluetoothGATTGetCharacteristics` for BLE properties
* `SetupDiGetDeviceProperty` / `DEVPKEY_DeviceContainer_IsConnected` for connection status

### UUIDs:

* `{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}` - `Bluetooth Device` for `SetupDiGetClassDevs`
* `{00001800-0000-1000-8000-00805F9B34FB}` - `Generic Attribute` for `Appearance` (`0x2A01`)
* `{0000180F-0000-1000-8000-00805F9B34FB}` - `Battery Service` for `Battery Level` (`0x2A19`)
* `{0000180A-0000-1000-8000-00805F9B34FB}` - `Device Information` for `Manufacturer` (`0x2A29`)

## Problems and workarounds:

* The program supports only BLE reports for now. Obtaining battery status is easy for BLE,
but HID and HFP devices use their own approach (see HID Usage Tables "Battery Strength" and HFPGetBatteryLevel accordingly).
HFP devices also expose HID interface, probably it's all doable with HID API or a system-wide dll hook. Will implement later.
* `BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE` (`0x00000004`) timeouts on disconnected devices, so check their status.
* `{00001800-0000-1000-8000-00805F9B34FB}` (`Generic Attribute`) is read only, don't use `GENERIC_WRITE` in `CreateFile`.

## References

* [Simple example (desktop programming C++) for Bluetooth Low Energy devices](https://social.msdn.microsoft.com/Forums/en-US/bad452cb-4fc2-4a86-9b60-070b43577cc9/is-there-a-simple-example-desktop-programming-c-for-bluetooth-low-energy-devices?forum=wdk)
* [How to get connection status of bluetooth LE device](https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/7b21b52f-bf85-4643-a717-9d62e15ffb51/how-to-get-connection-status-of-bluetooth-le-device-in-windows-81?forum=wdk)
* [Universal Serial Bus (USB) - HID Usage Tables (Hut1_12v2.pdf)](http://www.usb.org/developers/hidpage/Hut1_12v2.pdf)
* [HANDS-FREE PROFILE 1.5 (HFP 1.5_SPEC_V10.pdf)](https://www.bluetooth.org/docman/handlers/DownloadDoc.ashx?doc_id=41181)
* [HFPApi: HFPGetBatteryLevel](https://msdn.microsoft.com/en-us/library/cc510716.aspx)

