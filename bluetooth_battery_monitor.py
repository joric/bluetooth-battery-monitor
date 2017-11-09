#!/bin/python3

debug = 0

import json
from ctypes import HRESULT, resize, string_at, wstring_at, CDLL, windll, Structure, Union, POINTER, c_ulong, c_ulonglong, c_int, c_char, byref, sizeof, create_unicode_buffer, WinError, c_byte
from ctypes.wintypes import DWORD, WORD, BOOL, BOOLEAN, BYTE, USHORT, ULONG, WCHAR, HANDLE

SAPI = windll.SetupAPI
BT = windll.BluetoothAPIs

def Appearance(a):
    app = {960:'HID', 961:'Keyboard', 962:'Mouse', 963:'Joystick'}
    return app[a] if a in app else 'Bluetooth Device'

def new(structure, **kwargs):
    s = structure()
    for k, v in kwargs.items(): setattr(s, k, v)
    return s

class GUID(Structure):_fields_ = [('data1', DWORD), ('data2', WORD),('data3', WORD), ('data4', c_byte * 8)]
class SP_DEVINFO_DATA(Structure): _fields_ = [('cbSize', DWORD), ('ClassGuid', GUID), ('DevInst', DWORD), ('Reserved', POINTER(c_ulong))]
class SP_DEVICE_INTERFACE_DATA(Structure): _fields_ = [('cbSize', DWORD),('InterfaceClassGuid', GUID),('Flags', DWORD),('Reserved', POINTER(c_ulong))]
class PSP_DEVICE_INTERFACE_DETAIL_DATA(Structure): _fields_ = [('cbSize', DWORD), ('DevicePath',WCHAR * 1)]
class BTH_LE_UUID(Structure):_fields_ = [ ('isShortUuid', BOOL), ('ShortUuid', USHORT), ('LongUuid', GUID)]
class BTH_LE_GATT_SERVICE(Structure): _fields_ = [('ServiceUuid', BTH_LE_UUID),('AttributeHandle', USHORT)]

def parse_uuid(s):
    s = s.strip('{}').replace('-', '')
    return GUID(int(s[:8],16),int(s[8:12],16),int(s[12:16],16),(c_byte*8).from_buffer_copy(bytearray.fromhex(s[16:])))

class BTH_LE_GATT_CHARACTERISTIC(Structure): _pack_ = 4; _fields_ = [
    ('ServiceHandle', USHORT),
    ('CharacteristicUuid', BTH_LE_UUID),
    ('AttributeHandle', USHORT),
    ('CharacteristicValueHandle', USHORT),
    ('IsBroadcastable', BOOL),
    #('IsReadable', BOOLEAN),  #TODO: investigate structure alignment here
    #('IsWritable', BOOL),
    #('IsWritableWithoutResponse', BOOL),
    #('IsSignedWritable', BOOL),
    #('IsNotifiable', BOOL),
    #('IsIndicatable', BOOL),
    #('HasExtendedProperties', BOOL),
]

class BTH_LE_GATT_CHARACTERISTIC_VALUE(Structure): _fields_= [('DataSize', ULONG), ('Data', BYTE*1)]

BTH_LE_GATT_DESCRIPTOR_TYPE = BYTE

class BTH_LE_GATT_DESCRIPTOR(Structure): _fields_ = [
    ('ServiceHandle', USHORT),
    ('CharacteristicHandle', USHORT),
    ('DescriptorType', BTH_LE_GATT_DESCRIPTOR_TYPE),
    ('DescriptorUuid', BTH_LE_UUID),
    ('AttributeHandle', USHORT)
]

class BTH_LE_GATT_DESCRIPTOR_VALUE(Structure): _fields_ = [('DescriptorType', USHORT),
    ('DescriptorUuid', BTH_LE_UUID),
    ('DataSize', ULONG),
    ('Data', WCHAR*1)
]

DIGCF_PRESENT = 0x02
DIGCF_DEVINTERFACE = 0x10

GENERIC_WRITE = 1073741824
GENERIC_READ = -2147483648
FILE_SHARE_READ = 1
FILE_SHARE_WRITE = 2
OPEN_EXISTING = 3

BLUETOOTH_GATT_FLAG_NONE = 0

def DeviceConnect(path, db, field, cid):
    if debug: print('Connecting to:', path)

    hDevice = windll.kernel32.CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, None, OPEN_EXISTING, 0, None)
    ServicesBufferCount = USHORT()
    BT.BluetoothGATTGetServices(hDevice, 0, None, byref(ServicesBufferCount), BLUETOOTH_GATT_FLAG_NONE)
    ServicesBuffer = new(BTH_LE_GATT_SERVICE * ServicesBufferCount.value)   
    BT.BluetoothGATTGetServices(hDevice, ServicesBufferCount, byref(ServicesBuffer), byref(ServicesBufferCount), BLUETOOTH_GATT_FLAG_NONE)

    for Service in ServicesBuffer:
        CharacteristicsBufferCount = USHORT(0)
        hr = BT.BluetoothGATTGetCharacteristics(hDevice, byref(Service), 0, None, byref(CharacteristicsBufferCount), BLUETOOTH_GATT_FLAG_NONE)
        CharacteristicsBuffer = new(BTH_LE_GATT_CHARACTERISTIC * CharacteristicsBufferCount.value)
        CharacteristicsBufferActual = USHORT(0)
        BT.BluetoothGATTGetCharacteristics(hDevice, byref(Service), CharacteristicsBufferCount, byref(CharacteristicsBuffer), byref(CharacteristicsBufferActual), BLUETOOTH_GATT_FLAG_NONE)

        for Characteristic in CharacteristicsBuffer:
            uuid = Characteristic.CharacteristicUuid.ShortUuid
            charValueDataSize = USHORT()
            hr = BT.BluetoothGATTGetCharacteristicValue(
                hDevice,
                byref(Characteristic),
                0,
                None,
                byref(charValueDataSize),
                BLUETOOTH_GATT_FLAG_NONE)

            pCharValueBuffer = BTH_LE_GATT_CHARACTERISTIC_VALUE()
            resize(pCharValueBuffer, charValueDataSize.value)

            hr = BT.BluetoothGATTGetCharacteristicValue(
                hDevice,
                byref(Characteristic),
                charValueDataSize,
                byref(pCharValueBuffer),
                None,
                BLUETOOTH_GATT_FLAG_NONE)

            StringValue = string_at(byref(pCharValueBuffer, sizeof(ULONG)))
            arr = bytearray(StringValue)

            if uuid==0x2A19: # battery level
                value = arr[0]
            elif uuid==0x2A01: # appearance
                value = int.from_bytes(arr, byteorder='little')
                value = Appearance(value)
            else: # string
                value = str(StringValue.decode(errors='ignore'))

            if debug: print ( '\t%X' % uuid, value )

            for j in range(len(db)):
                address = db[j]['hwid'].split('_')[1]
                if address in path and uuid==cid:
                    db[j][field] = value
    return db


DEVPROPGUID = GUID
DEVPROPID = ULONG
class DEVPROPKEY(Structure):
    _fields_ = [
        ('fmtid', DEVPROPGUID),
        ('pid', DEVPROPID)]

LPTSTR = POINTER(c_char)
WBAse = windll.Kernel32
ERROR_INSUFFICIENT_BUFFER = 0x7A
ERROR_INVALID_DATA = 0xD
GetLastError = WBAse.GetLastError

SPDRP_FRIENDLYNAME = DWORD(0x0000000C)
SPDRP_DEVICEDESC = DWORD(0x00000000)
SPDRP_ENUMERATOR_NAME = DWORD(0x00000016)
SPDRP_HARDWAREID = DWORD(0x00000001)

def GetConnectedStatus(hdi, devinfo_data):
    DEVPROPTYPE = ULONG
    DN_DEVICE_DISCONNECTED = 0x2000000
    ulPropertyType = DEVPROPTYPE()
    dwSize = DWORD()
    devst = (ULONG*1)()
    key = DEVPROPKEY(parse_uuid('{4340a6c5-93fa-4706-972c-7b648008a5a7}'), 2)
    SAPI.SetupDiGetDevicePropertyW(hdi, byref(devinfo_data), byref(key),byref(ulPropertyType),
        byref(devst), sizeof(devst), byref(dwSize), DWORD(0))
    return not (devst[0] & DN_DEVICE_DISCONNECTED)

def GetDeviceNames(uuid='{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}'):
    db = []
    hdi = SAPI.SetupDiGetClassDevsW(byref(parse_uuid(uuid)), None, None, DIGCF_PRESENT)
    devinfo_data = new(SP_DEVINFO_DATA, cbSize=sizeof(SP_DEVINFO_DATA))
    i = 0
    while SAPI.SetupDiEnumDeviceInfo(hdi, i, byref(devinfo_data)):
        i += 1
        hwid = get_device_info(hdi, i, devinfo_data, SPDRP_HARDWAREID)
        if not (hwid.startswith('BTHENUM\\Dev_') or hwid.startswith('BTHLE\\Dev_')):
            continue
        name = get_device_info(hdi, i, devinfo_data, SPDRP_FRIENDLYNAME)
        desc = get_device_info(hdi, i, devinfo_data, SPDRP_DEVICEDESC)
        enum = get_device_info(hdi, i, devinfo_data, SPDRP_ENUMERATOR_NAME)
        status = 'connected' if GetConnectedStatus(hdi, devinfo_data) else 'paired'
        db.append( { 'name':name, 'status':status, 'hwid':hwid } )
    return db


def get_device_info(hdi, i, devinfo_data, propertyname):
    data_t = DWORD()
    buffer = LPTSTR()
    buffersize = DWORD(0)
    while ( not SAPI.SetupDiGetDeviceRegistryPropertyW( hdi, byref(devinfo_data), propertyname, data_t, byref(buffer), buffersize, byref(buffersize))):
        err = GetLastError()
        if err == ERROR_INSUFFICIENT_BUFFER:
            buffer = create_unicode_buffer(buffersize.value)
        elif err == ERROR_INVALID_DATA:
            break
        else:
            raise WinError(err)
    try:
        return str(buffer.value)
    except:
        return ''

def GetDevicePaths(uuid, db, field, cid):
    guid = parse_uuid(uuid)
    hdi = SAPI.SetupDiGetClassDevsW(byref(guid), None, None, DIGCF_DEVINTERFACE | DIGCF_PRESENT)
    dd = SP_DEVINFO_DATA(sizeof(SP_DEVINFO_DATA))
    did = SP_DEVICE_INTERFACE_DATA(sizeof(SP_DEVICE_INTERFACE_DATA))
    i = 0
    while SAPI.SetupDiEnumDeviceInterfaces(hdi, None, byref(guid), i, byref(did)):
        i += 1
        size = DWORD(0)
        SAPI.SetupDiGetDeviceInterfaceDetailW(hdi, byref(did), None, 0, byref(size), 0)
        didd = PSP_DEVICE_INTERFACE_DETAIL_DATA()
        resize(didd, size.value)
        didd.cbSize = sizeof(PSP_DEVICE_INTERFACE_DETAIL_DATA) - sizeof(WCHAR * 1)
        SAPI.SetupDiGetDeviceInterfaceDetailW(hdi, byref(did), byref(didd), size, byref(size), byref(dd))
        path = wstring_at(byref(didd, sizeof(DWORD)))
        db = DeviceConnect(path, db, field, cid)
    return db

def main():
    db = GetDeviceNames()

    db = GetDevicePaths('{0000180F-0000-1000-8000-00805F9B34FB}', db, 'battery_level', 0x2A19)
    db = GetDevicePaths('{00001800-0000-1000-8000-00805F9B34FB}', db, 'appearance', 0x2A01)
    db = GetDevicePaths('{0000180A-0000-1000-8000-00805F9B34FB}', db, 'manufacturer', 0x2A29)
    #db = GetDevicePaths('{0000180A-0000-1000-8000-00805F9B34FB}', db, 'firmware', 0x2A26)
    #db = GetDevicePaths('{0000180A-0000-1000-8000-00805F9B34FB}', db, 'serial', 0x2A25)

    if not debug:
        print ( json.dumps(db, sort_keys=True, indent=4) )

if __name__=='__main__':
    main()

