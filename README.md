Bluetooth Battery Monitor
=========================

Open source Bluetooth battery monitor for Windows. Very alpha stage, supports only BLE reports for now.

## Enumerating Bluetooth devices and services

I'm not using UWP, only classic C/C++ API. Obtaining battery status is easy for BLE,
but HID and HFP devices use their own approach
(see HID Usage Tables "Battery Strength" and HFPGetBatteryLevel accordingly).
HFP devices also expose HID interface, so maybe it's doable with a system-wide HID-intercepting dll hook.

### API calls:

* `SetupDiGetClassDevs` / `SetupDiEnumDeviceInfo` for getting a list of all bluetooth devices
* `SetupDiGetDeviceRegistryProperty` / `SPDRP_HARDWAREID` for filtering by device address
* `BluetoothGATTGetServices` / `BluetoothGATTGetCharacteristics` for BLE properties
* `SetupDiGetDeviceProperty` / `DEVPKEY_DeviceContainer_IsConnected` for connection status

### UUIDs:

* `{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}` (Bluetooth Device) for `SetupDiGetClassDevs`
* `{00001800-0000-1000-8000-00805F9B34FB}` (Generic Attribute) for Appearance (`0x2A01`)
* `{0000180F-0000-1000-8000-00805F9B34FB}` (Battery Service) for Battery Level (`0x2A19`)
* `{0000180A-0000-1000-8000-00805F9B34FB}` (Device Information) for Manufacturer (`0x2A29`)

## References

* [How to get connection status of bluetooth LE device](https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/7b21b52f-bf85-4643-a717-9d62e15ffb51/how-to-get-connection-status-of-bluetooth-le-device-in-windows-81?forum=wdk)
* [Universal Serial Bus (USB) - HID Usage Tables (Hut1_12v2.pdf)](http://www.usb.org/developers/hidpage/Hut1_12v2.pdf)
* [HANDS-FREE PROFILE 1.5 (HFP 1.5_SPEC_V10.pdf)](https://www.bluetooth.org/docman/handlers/DownloadDoc.ashx?doc_id=41181)
* [HFPApi: HFPGetBatteryLevel](https://msdn.microsoft.com/en-us/library/cc510716.aspx)

