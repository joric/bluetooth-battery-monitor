Bluetooth Battery Monitor
=========================

Open source Bluetooth battery monitor for Windows. Very alpha stage, supports only BLE reports for now.

## Enumerating Bluetooth devices

There are a few ways. I'm using regular Win32 API (you don't need UWP for that)
with `SetupDiGetClassDevs` and `SetupDiEnumDeviceInfo` called with bluetooth device class uuid `{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}`.
Then I filter out unneccesary devices by their hardware id obtained with `SetupDiGetDeviceRegistryProperty`.

Apparently there's no easy way to list BLE services hierarchically (`Children` property in Device Manager).
The simplest way is to use `SetupDiGetClassDevs` and `SetupDiGetDeviceInterfaceDetail` with corresponding service uuids,
filter out by harware ids and collect characteristics you need. I'm using following services and characteristics:

* `{00001800-0000-1000-8000-00805F9B34FB}` (Generic Attribute) for Appearance (`0x2A01`)
* `{0000180F-0000-1000-8000-00805F9B34FB}` (Battery Service) for Battery Level (`0x2A19`)
* `{0000180A-0000-1000-8000-00805F9B34FB}` (Device Information) for Manufacturer (`0x2A29`)

To get connection status, I'm using approach mentioned [here](https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/7b21b52f-bf85-4643-a717-9d62e15ffb51/how-to-get-connection-status-of-bluetooth-le-device-in-windows-81?forum=wdk)
on the msdn forums. You simply call `SetupDiGetDeviceProperty` with the key `DEVPKEY_DeviceContainer_IsConnected` and
check the `DN_DEVICE_DISCONNECTED` flag (0x200000).

## Bluetooth battery status

Obtaining Bluetooth battery properties is very easy for BLE but pretty complicated for HID and HFP devices
They have their own way to report battery level (see HID Usage Tables "Battery Strength"
and HFPGetBatteryLevel accordingly). HFP devices also expose HID interface so it's probably
doable with a system-wide HID hook. I will investigate HID and HFP reports later.

## References

* [Universal Serial Bus (USB) - HID Usage Tables (Hut1_12v2.pdf)](http://www.usb.org/developers/hidpage/Hut1_12v2.pdf)
* [HANDS-FREE PROFILE 1.5 (HFP 1.5_SPEC_V10.pdf)](https://www.bluetooth.org/docman/handlers/DownloadDoc.ashx?doc_id=41181)
* [HFPApi: HFPGetBatteryLevel](https://msdn.microsoft.com/en-us/library/cc510716.aspx)

