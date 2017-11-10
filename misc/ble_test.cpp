#include <stdio.h>
#include <Windows.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <BluetoothAPIs.h>
#include <BluetoothLEAPIs.h>

#pragma comment(lib, "Ole32")
#pragma comment(lib, "SetupAPI")
#pragma comment(lib, "BluetoothAPIs")

#define BTW_GATT_UUID_SERVCLASS_BATTERY                     0x180F    /* Battery Service  */
#define BTW_GATT_UUID_SERVCLASS_DEVICE_INFO                 0x180A    /* Device Information Service  */

//#define MOSS_DEVICE_UUID L"{00001800-0000-1000-8000-00805F9B34FB}" // generic access
//#define MOSS_DEVICE_UUID L"{00001801-0000-1000-8000-00805F9B34FB}" // generic attrubute
//#define MOSS_DEVICE_UUID L"{00001812-0000-1000-8000-00805F9B34FB}" // hid reports
//#define MOSS_DEVICE_UUID L"{0000180A-0000-1000-8000-00805F9B34FB}" // device information
#define MOSS_DEVICE_UUID L"{0000180F-0000-1000-8000-00805F9B34FB}" // battery service

int cached = 1;

void CALLBACK HandleBLENotification(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	printf("notification obtained ");
	PBLUETOOTH_GATT_VALUE_CHANGED_EVENT ValueChangedEventParameters = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;

	HRESULT hr;
	if (0 == ValueChangedEventParameters->CharacteristicValue->DataSize) {
		hr = E_FAIL;
		printf("datasize 0\n");
	}
	else {
		printf("got value ");
		for(int i=0; i<ValueChangedEventParameters->CharacteristicValue->DataSize;i++) {
			printf("%d",ValueChangedEventParameters->CharacteristicValue->Data[i]);
		}
		printf("\n");

		// if the first bit is set, then the value is the next 2 bytes.  If it is clear, the value is in the next byte
		//The Heart Rate Value Format bit (bit 0 of the Flags field) indicates if the data format of 
		//the Heart Rate Measurement Value field is in a format of UINT8 or UINT16. 
		//When the Heart Rate Value format is sent in a UINT8 format, the Heart Rate Value 
		//Format bit shall be set to 0. When the Heart Rate Value format is sent in a UINT16 
		//format, the Heart Rate Value Format bit shall be set to 1
		//from this PDF https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=239866
		//unsigned heart_rate;
		//if (0x01 == (ValueChangedEventParameters->CharacteristicValue->Data[0] & 0x01)) {
		//	heart_rate = ValueChangedEventParameters->CharacteristicValue->Data[1] * 256 + ValueChangedEventParameters->CharacteristicValue->Data[2];
		//}
		//else {
		//	heart_rate = ValueChangedEventParameters->CharacteristicValue->Data[1];
		//}
		//printf("%d\n", heart_rate);
	}
}

// This function works to get a handle for a BLE device based on its GUID
// Copied from http://social.msdn.microsoft.com/Forums/windowshardware/en-US/e5e1058d-5a64-4e60-b8e2-0ce327c13058/erroraccessdenied-error-when-trying-to-receive-data-from-bluetooth-low-energy-devices?forum=wdk
// From https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/bad452cb-4fc2-4a86-9b60-070b43577cc9/is-there-a-simple-example-desktop-programming-c-for-bluetooth-low-energy-devices?forum=wdk
// Credits to Andrey_sh
HANDLE GetBLEHandle(__in GUID AGuid) {
	HDEVINFO hDI;
	SP_DEVICE_INTERFACE_DATA did;
	SP_DEVINFO_DATA dd;
	GUID BluetoothInterfaceGUID = AGuid;
	HANDLE hComm = NULL;

	hDI = SetupDiGetClassDevs(&BluetoothInterfaceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

	if (hDI == INVALID_HANDLE_VALUE) return NULL;

	did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	dd.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDI, NULL, &BluetoothInterfaceGUID, i, &did); i++)
	{
		SP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;

		DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		DWORD size = 0;

		if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, NULL, 0, &size, 0))
		{
			int err = GetLastError();

			if (err == ERROR_NO_MORE_ITEMS) break;

			PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GPTR, size);

			pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, pInterfaceDetailData, size, &size, &dd))
				break;

			hComm = CreateFile(
				pInterfaceDetailData->DevicePath,
				GENERIC_WRITE | GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			printf("created hComm for %s\n", pInterfaceDetailData->DevicePath);

			GlobalFree(pInterfaceDetailData);
		}
	}

	SetupDiDestroyDeviceInfoList(hDI);
	return hComm;
}

int ConnectBLEDevice() {
	// Step 1: find the BLE device handle from its GUID
	GUID AGuid;
	// GUID can be constructed from "{xxx....}" string using CLSID
	CLSIDFromString(TEXT(MOSS_DEVICE_UUID), &AGuid);
	// Get the handle 
	HANDLE hLEDevice = GetBLEHandle(AGuid);

	printf("Handle: %d\n", hLEDevice);

	// Step 2: Get a list of services that the device advertises
	// first send 0, NULL as the parameters to BluetoothGATTServices inorder to get the number of
	// services in serviceBufferCount
	USHORT serviceBufferCount;
	////////////////////////////////////////////////////////////////////////////
	// Determine Services Buffer Size
	////////////////////////////////////////////////////////////////////////////

	HRESULT hr = BluetoothGATTGetServices( hLEDevice, 0, NULL, &serviceBufferCount, BLUETOOTH_GATT_FLAG_NONE );

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		//printf("BluetoothGATTGetServices - Buffer Size %d\n", hr);
	}

	PBTH_LE_GATT_SERVICE pServiceBuffer = (PBTH_LE_GATT_SERVICE)
	malloc(sizeof(BTH_LE_GATT_SERVICE) * serviceBufferCount);

	if (NULL == pServiceBuffer) {
		printf("pServiceBuffer out of memory\n");
	} else {
		RtlZeroMemory(pServiceBuffer,
			sizeof(BTH_LE_GATT_SERVICE) * serviceBufferCount);
	}

	////////////////////////////////////////////////////////////////////////////
	// Retrieve Services
	////////////////////////////////////////////////////////////////////////////

	USHORT numServices;
	hr = BluetoothGATTGetServices(
		hLEDevice,
		serviceBufferCount,
		pServiceBuffer,
		&numServices,
		BLUETOOTH_GATT_FLAG_NONE);


	printf("found %d services\n", numServices);

	if (S_OK != hr) {
		//printf("BluetoothGATTGetServices - Buffer Size %d\n", hr);
	}

	// Step 3: now get the list of charactersitics. note how the pServiceBuffer is required from step 2
	////////////////////////////////////////////////////////////////////////////
	// Determine Characteristic Buffer Size
	////////////////////////////////////////////////////////////////////////////

	USHORT charBufferSize=0;
	hr = BluetoothGATTGetCharacteristics(
		hLEDevice,
		pServiceBuffer,
		0,
		NULL,
		&charBufferSize,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		//printf("BluetoothGATTGetCharacteristics - Buffer Size %d\n", hr);
	}

	PBTH_LE_GATT_CHARACTERISTIC pCharBuffer=0;
	if (charBufferSize > 0) {
		pCharBuffer = (PBTH_LE_GATT_CHARACTERISTIC)
			malloc(charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));

		if (NULL == pCharBuffer) {
			printf("pCharBuffer out of memory\n");
		}
		else {
			RtlZeroMemory(pCharBuffer,
				charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));
		}

		////////////////////////////////////////////////////////////////////////////
		// Retrieve Characteristics
		////////////////////////////////////////////////////////////////////////////
		USHORT numChars;
		hr = BluetoothGATTGetCharacteristics(
			hLEDevice,
			pServiceBuffer,
			charBufferSize,
			pCharBuffer,
			&numChars,
			BLUETOOTH_GATT_FLAG_NONE);

		printf("found %d characteristics\n", numChars);


		if (S_OK != hr) {
			printf("BluetoothGATTGetCharacteristics - Actual Data %d\n", hr);
		}

		if (numChars != charBufferSize) {
			printf("buffer size and buffer size actual size mismatch\n");
		}
	}

	// Step 4: now get the list of descriptors. note how the pCharBuffer is required from step 3
	// descriptors are required as we descriptors that are notification based will have to be written
	// once IsSubcribeToNotification set to true, we set the appropriate callback function
	// need for setting descriptors for notification according to
	//http://social.msdn.microsoft.com/Forums/en-US/11d3a7ce-182b-4190-bf9d-64fefc3328d9/windows-bluetooth-le-apis-event-callbacks?forum=wdk
	PBTH_LE_GATT_CHARACTERISTIC currGattChar;
	for (int ii = 0; ii <charBufferSize; ii++) {
		currGattChar = &pCharBuffer[ii];
		USHORT charValueDataSize;
		PBTH_LE_GATT_CHARACTERISTIC_VALUE pCharValueBuffer;

		///////////////////////////////////////////////////////////////////////////
		// Determine Descriptor Buffer Size
		////////////////////////////////////////////////////////////////////////////
		USHORT descriptorBufferSize;
		hr = BluetoothGATTGetDescriptors(
			hLEDevice,
			currGattChar,
			0,
			NULL,
			&descriptorBufferSize,
			BLUETOOTH_GATT_FLAG_NONE);

		if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
			//printf("BluetoothGATTGetDescriptors - Buffer Size %d\n", hr);
		}

		PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer;
		if (descriptorBufferSize > 0) {
			pDescriptorBuffer = (PBTH_LE_GATT_DESCRIPTOR)
				malloc(descriptorBufferSize
					* sizeof(BTH_LE_GATT_DESCRIPTOR));

			if (NULL == pDescriptorBuffer) {
				printf("pDescriptorBuffer out of memory\n");
			}
			else {
				RtlZeroMemory(pDescriptorBuffer, descriptorBufferSize);
			}

			////////////////////////////////////////////////////////////////////////////
			// Retrieve Descriptors
			////////////////////////////////////////////////////////////////////////////

			USHORT numDescriptors;
			hr = BluetoothGATTGetDescriptors(
				hLEDevice,
				currGattChar,
				descriptorBufferSize,
				pDescriptorBuffer,
				&numDescriptors,
				BLUETOOTH_GATT_FLAG_NONE);

			printf("got %d descriptors\n", numDescriptors);

			if (S_OK != hr) {
				printf("BluetoothGATTGetDescriptors - Actual Data %d\n", hr);
			}

			if (numDescriptors != descriptorBufferSize) {
				printf("buffer size and buffer size actual size mismatch\n");
			}

			for (int kk = 0; kk<numDescriptors; kk++) {
				PBTH_LE_GATT_DESCRIPTOR  currGattDescriptor = &pDescriptorBuffer[kk];
				////////////////////////////////////////////////////////////////////////////
				// Determine Descriptor Value Buffer Size
				////////////////////////////////////////////////////////////////////////////
				USHORT descValueDataSize;
				hr = BluetoothGATTGetDescriptorValue(
					hLEDevice,
					currGattDescriptor,
					0,
					NULL,
					&descValueDataSize,
					cached ? BLUETOOTH_GATT_FLAG_NONE : BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE);

				if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
					printf("BluetoothGATTGetDescriptorValue - Buffer Size %d\n", hr);
				}

				PBTH_LE_GATT_DESCRIPTOR_VALUE pDescValueBuffer = (PBTH_LE_GATT_DESCRIPTOR_VALUE)malloc(descValueDataSize);

				if (NULL == pDescValueBuffer) {
					printf("pDescValueBuffer out of memory\n");
				}
				else {
					RtlZeroMemory(pDescValueBuffer, descValueDataSize);
				}

				////////////////////////////////////////////////////////////////////////////
				// Retrieve the Descriptor Value
				////////////////////////////////////////////////////////////////////////////

				hr = BluetoothGATTGetDescriptorValue(
					hLEDevice,
					currGattDescriptor,
					(ULONG)descValueDataSize,
					pDescValueBuffer,
					NULL,
					cached ? BLUETOOTH_GATT_FLAG_NONE : BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE);

				if (S_OK != hr) {
					printf("BluetoothGATTGetDescriptorValue - Actual Data %d\n", hr);
				}
				// you may also get a descriptor that is read (and not notify) andi am guessing the attribute handle is out of limits
				// we set all descriptors that are notifiable to notify us via IsSubstcibeToNotification
				if (currGattDescriptor->AttributeHandle < 255) {
					BTH_LE_GATT_DESCRIPTOR_VALUE newValue;

					RtlZeroMemory(&newValue, sizeof(newValue));

					newValue.DescriptorType = ClientCharacteristicConfiguration;
					newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = TRUE;

					hr = BluetoothGATTSetDescriptorValue(
						hLEDevice,
						currGattDescriptor,
						&newValue,
						BLUETOOTH_GATT_FLAG_NONE);
					if (S_OK != hr) {
						printf("BluetoothGATTGetDescriptorValue - Actual Data %d\n", hr);
					}
				}

			}
		}

		// set the appropriate callback function when the descriptor change value
		BLUETOOTH_GATT_EVENT_HANDLE EventHandle;

		if (currGattChar->IsNotifiable) {
			printf("Setting Notification for ServiceHandle %d\n", currGattChar->ServiceHandle);
			BTH_LE_GATT_EVENT_TYPE EventType = CharacteristicValueChangedEvent;

			BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION EventParameterIn;
			EventParameterIn.Characteristics[0] = *currGattChar;
			EventParameterIn.NumCharacteristics = 1;
			hr = BluetoothGATTRegisterEvent(
				hLEDevice,
				EventType,
				&EventParameterIn,
				HandleBLENotification,
				NULL,
				&EventHandle,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTRegisterEvent - Actual Data %d\n", hr);
			}
		}

		if (currGattChar->IsReadable) {
			////////////////////////////////////////////////////////////////////////////
			// Determine Characteristic Value Buffer Size
			////////////////////////////////////////////////////////////////////////////
			hr = BluetoothGATTGetCharacteristicValue(
				hLEDevice,
				currGattChar,
				0,
				NULL,
				&charValueDataSize,
				cached ? BLUETOOTH_GATT_FLAG_NONE : BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE);

			if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
				printf("BluetoothGATTGetCharacteristicValue - Buffer Size %d\n", hr);
			}

			pCharValueBuffer = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)malloc(charValueDataSize);

			if (NULL == pCharValueBuffer) {
				printf("pCharValueBuffer out of memory\n");
			}
			else {
				RtlZeroMemory(pCharValueBuffer, charValueDataSize);
			}

			////////////////////////////////////////////////////////////////////////////
			// Retrieve the Characteristic Value
			////////////////////////////////////////////////////////////////////////////

			hr = BluetoothGATTGetCharacteristicValue(
				hLEDevice,
				currGattChar,
				(ULONG)charValueDataSize,
				pCharValueBuffer,
				NULL,
				cached ? BLUETOOTH_GATT_FLAG_NONE : BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE);

			if (S_OK != hr) {
				printf("BluetoothGATTGetCharacteristicValue - Actual Data %d\n", hr);
			}

			// print the characteristic Value

			USHORT uuid = currGattChar->CharacteristicUuid.Value.ShortUuid;
			printf("Printing a read characterstic %0X: ", uuid);
			for (int iii = 0; iii < (int)pCharValueBuffer->DataSize; iii++) {
				if (uuid==0x2a50 || uuid==0x2a19)
				printf("%02d ", pCharValueBuffer->Data[iii]);
				else printf("%c", pCharValueBuffer->Data[iii]);
			}

			printf("\n");

			// Free before going to next iteration, or memory leak.
			free(pCharValueBuffer);
			pCharValueBuffer = NULL;
		}

	}

	// go into an inf loop that sleeps. you will ideally see notifications from the HR device
	while (1) {
		printf(".");
		Sleep(1000);
	}

	CloseHandle(hLEDevice);

	if (GetLastError() != NO_ERROR &&
		GetLastError() != ERROR_NO_MORE_ITEMS)
	{
		// Insert error handling here.
		return 1;
	}

	return 0;
}

int ScanBLEDevices() {
	HDEVINFO hDI;
	SP_DEVINFO_DATA did;
	DWORD i;

	// Create a HDEVINFO with all present devices.
	hDI = SetupDiGetClassDevs(&GUID_DEVCLASS_BLUETOOTH, NULL, NULL, DIGCF_PRESENT);

	if (hDI == INVALID_HANDLE_VALUE)
	{
		return 1;
	}

	// Enumerate through all devices in Set.
	did.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i = 0; SetupDiEnumDeviceInfo(hDI, i, &did); i++)
	{
		bool hasError = false;

		DWORD nameData;
		wchar_t * nameBuffer = NULL;
		DWORD nameBufferSize = 0;

		while (!SetupDiGetDeviceRegistryProperty(
			hDI,
			&did,
			SPDRP_FRIENDLYNAME,
			&nameData,
			(PBYTE)nameBuffer,
			nameBufferSize,
			&nameBufferSize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (nameBuffer) delete(nameBuffer);
				nameBuffer = new wchar_t[nameBufferSize * 2];
			}
			else
			{
				hasError = true;
				break;
			}
		}

		DWORD addressData;
		wchar_t * addressBuffer = NULL;
		DWORD addressBufferSize = 0;

		while (!SetupDiGetDeviceRegistryProperty(
			hDI,
			&did,
			SPDRP_HARDWAREID,
			&addressData,
			(PBYTE)addressBuffer,
			addressBufferSize,
			&addressBufferSize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (addressBuffer) delete(addressBuffer);
				addressBuffer = new wchar_t[addressBufferSize * 2];
			}
			else
			{
				hasError = true;
				break;
			}
		}

		LPSTR deviceIdBuffer = NULL;
		DWORD deviceIdBufferSize = 0;

		while (!SetupDiGetDeviceInstanceId(
			hDI,
			&did,
			deviceIdBuffer,
			deviceIdBufferSize,
			&deviceIdBufferSize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (deviceIdBuffer) delete(deviceIdBuffer);
				deviceIdBuffer = new char[deviceIdBufferSize * 2];
			}
			else
			{
				hasError = true;
				break;
			}
		}

		if (hasError)
		{
			continue;
		}

/*
		std::string name = util::to_narrow(nameBuffer);
		std::string address = util::to_narrow(addressBuffer);
		std::string deviceId = util::to_narrow(deviceIdBuffer);
		std::cout << "Found " << name << " (" << deviceId << ")" << std::endl;
*/
	}

	return 0;
}

int FindDevices2(){
	HBLUETOOTH_DEVICE_FIND founded_device;

	BLUETOOTH_DEVICE_INFO device_info;
	device_info.dwSize = sizeof(device_info);

	BLUETOOTH_DEVICE_SEARCH_PARAMS search_criteria;
	search_criteria.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
	search_criteria.fReturnAuthenticated = FALSE;
	search_criteria.fReturnRemembered = FALSE;
	search_criteria.fReturnConnected = TRUE;
	search_criteria.fReturnUnknown = FALSE;
	search_criteria.fIssueInquiry = FALSE;
	search_criteria.cTimeoutMultiplier = 0;

	founded_device = BluetoothFindFirstDevice(&search_criteria, &device_info);

	if (founded_device == NULL)
	{
		//_tprintf(TEXT("Error: \n%s\n"), getErrorMessage(WSAGetLastError(), error));
		printf("error\n");
		return -1;
	}

	do
	{
		//_tprintf(TEXT("found device: %s\n"), device_info.szName);

		printf("found device: %ws\n", device_info.szName);

	} while (BluetoothFindNextDevice(founded_device, &device_info));

	return 0;
}


int FindDevices() {
	BLUETOOTH_FIND_RADIO_PARAMS m_bt_find_radio = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

	BLUETOOTH_RADIO_INFO m_bt_info = { sizeof(BLUETOOTH_RADIO_INFO),0, };

	BLUETOOTH_DEVICE_SEARCH_PARAMS m_search_params;

	/*
	BLUETOOTH_DEVICE_SEARCH_PARAMS m_search_params = {
		sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
		1,
		0,
		1,
		1,
		1,
		15,
		NULL
	};
	*/

	BLUETOOTH_DEVICE_INFO m_device_info = { sizeof(BLUETOOTH_DEVICE_INFO),0, };

	HANDLE m_radio = NULL;
	HBLUETOOTH_RADIO_FIND m_bt = NULL;
	HBLUETOOTH_DEVICE_FIND m_bt_dev = NULL;
	int m_radio_id;
	int m_device_id;
	DWORD mbtinfo_ret;

	// Iterate for available bluetooth radio devices in range
	// Starting from the local
	while (TRUE)
	{
		m_bt = BluetoothFindFirstRadio(&m_bt_find_radio, &m_radio);
		if (!m_bt)
			printf("BluetoothFindFirstRadio() failed with error code %d\n", GetLastError());

		m_radio_id = 0;

		ZeroMemory(&m_device_info, sizeof(BLUETOOTH_DEVICE_INFO));
		m_device_info.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);

		do {
			// Then get the radio device info....
			mbtinfo_ret = BluetoothGetRadioInfo(m_radio, &m_bt_info);
			if (mbtinfo_ret != ERROR_SUCCESS)
				printf("BluetoothGetRadioInfo() failed wit herror code %d\n", mbtinfo_ret);

			wprintf(L"Radio %d: %s\r\n", m_radio_id, m_bt_info.szName);
			wprintf(L"\tAddress: %02X:%02X:%02X:%02X:%02X:%02X\r\n", m_bt_info.address.rgBytes[5],
				m_bt_info.address.rgBytes[4], m_bt_info.address.rgBytes[3], m_bt_info.address.rgBytes[2],
				m_bt_info.address.rgBytes[1], m_bt_info.address.rgBytes[0]);
			wprintf(L"\tClass: 0x%08x\r\n", m_bt_info.ulClassofDevice);
			wprintf(L"\tManufacturer: 0x%04x\r\n", m_bt_info.manufacturer);

			m_search_params.hRadio = m_radio;
			m_search_params.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
			m_search_params.fReturnAuthenticated = FALSE;//TRUE;
			m_search_params.fReturnRemembered = FALSE;//TRUE;
			m_search_params.fReturnConnected = TRUE;
			m_search_params.fReturnUnknown = TRUE;
			m_search_params.fIssueInquiry = FALSE;
			m_search_params.cTimeoutMultiplier = 0;

			// Next for every radio, get the device
			m_bt_dev = BluetoothFindFirstDevice(&m_search_params, &m_device_info);

			if (!m_bt_dev)
				printf("\nBluetoothFindFirstDevice() failed with error code %d\n", GetLastError());

			m_radio_id++;
			m_device_id = 0;

			// Get the device info
			do
			{
				wprintf(L"\tDevice %d: %s\r\n", m_device_id, m_device_info.szName);
				BYTE * a = m_device_info.Address.rgBytes;
				wprintf(L"\t\tAddress: %02X:%02X:%02X:%02X:%02X:%02X\r\n", a[5], a[4], a[3], a[2], a[1], a[0]);
				wprintf(L"\t\tClass: 0x%08x\r\n", m_device_info.ulClassofDevice);
				wprintf(L"\t\tConnected: %s\r\n", m_device_info.fConnected ? L"true" : L"false");
				wprintf(L"\t\tAuthenticated: %s\r\n", m_device_info.fAuthenticated ? L"true" : L"false");
				wprintf(L"\t\tRemembered: %s\r\n", m_device_info.fRemembered ? L"true" : L"false");
				m_device_id++;

			} while ( BluetoothFindNextDevice(m_bt_dev, &m_device_info) );

			// NO more device, close the device handle
			if (!BluetoothFindDeviceClose(m_bt_dev))
				printf("\nBluetoothFindDeviceClose(m_bt_dev) failed with error code %d\n", GetLastError());

		} while (BluetoothFindNextRadio(&m_bt_find_radio, &m_radio));

		// No more radio, close the radio handle
		if (!BluetoothFindRadioClose(m_bt))
			printf("BluetoothFindRadioClose(m_bt) failed with error code %d\n", GetLastError());

		// Exit the outermost WHILE and BluetoothFindXXXXRadio loops if there is no more radio
		if (!BluetoothFindNextRadio(&m_bt_find_radio, &m_radio))
			break;

		// Give some time for the 'signal' which is a typical for crap wireless devices
		Sleep(1000);
	}

	return 0;
}


int main() {
	//FindDevices();
	ConnectBLEDevice();

	//printf("hello\n");
}

