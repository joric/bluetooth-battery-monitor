#include <stdio.h>
#include <Windows.h>
#include <BluetoothAPIs.h>
#pragma comment(lib, "Bthprops")

char * fclass(char * buf, int CoD) {
	int major = (CoD >> 8) & 0x1f;
	int minor = (CoD >> 2) & 0x3f;
	char * cc[] = {"Miscellaneous", "Computer", "Phone", "LAN/Network Access Point", "Audio/Video", "Peripheral", "Imaging", "Wearable", "Toy", "Health"};
	int cc_count = sizeof(cc)/sizeof(*cc);
	char * cat = major<cc_count ? cc[major] : "Unknown";
	char * sub = 0;
	char * cc4[] = {"Uncategorized", "Wearable Headset Device", "Hands-free Device", "(Reserved)", "Microphone", "Loudspeaker", "Headphones",
		"Portable Audio", "Car audio", "Set-top box", "HiFi Audio Device", "VCR", "Video Camera", "Camcorder", "Video Monitor",
		"Video Display and Loudspeaker", "Video Conferencing", "(Reserved)", "Gaming/Toy"};
	char * cc5[] = {"Unknown", "Keyboard", "Mouse", "Keyboard/Mouse Combo"};
	switch (major) {
		case 4: sub = cc4[minor]; break;
		case 5: sub = cc5[minor>>4]; break;
	}
	sprintf(buf, "%s%s%s", cat, sub ? ": ":"", sub ? sub : "");
	return buf;
}

char * ftime(char * buf, SYSTEMTIME * t) {
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", t->wYear, t->wMonth, t->wDay, t->wHour, t->wMinute, t->wSecond);
	return buf;
}

int FindDevices(HANDLE hRadio=NULL) {
	BLUETOOTH_DEVICE_SEARCH_PARAMS sp;
	HBLUETOOTH_DEVICE_FIND df;
	BLUETOOTH_DEVICE_INFO di;

	sp.dwSize = sizeof(sp);
	sp.hRadio = hRadio;
	sp.fReturnAuthenticated = TRUE;
	sp.fReturnRemembered = TRUE;
	sp.fReturnConnected = TRUE;
	sp.fReturnUnknown = TRUE;
	sp.fIssueInquiry = FALSE;
	sp.cTimeoutMultiplier = 0;

	di.dwSize = sizeof(di);
	df = BluetoothFindFirstDevice(&sp, &di);

	do {
		printf("\t%ws\n", di.szName);

		char buf[256];
		BYTE * a = di.Address.rgBytes;
		printf("\t\tAddress: %02X:%02X:%02X:%02X:%02X:%02X\r\n", a[5], a[4], a[3], a[2], a[1], a[0]);
		printf("\t\tClass: 0x%08x (%s)\r\n", di.ulClassofDevice, fclass(buf, di.ulClassofDevice));
		printf("\t\tConnected: %s\r\n", di.fConnected ? "true" : "false");
		printf("\t\tRemembered: %s\r\n", di.fRemembered ? "true" : "false");
		printf("\t\tAuthenticated: %s\r\n", di.fAuthenticated ? "true" : "false");
		printf("\t\tLastSeen: %s\r\n", ftime(buf, &di.stLastSeen) );
		printf("\t\tLastUsed: %s\r\n", ftime(buf, &di.stLastUsed) );

	} while ( BluetoothFindNextDevice(df, &di) );

	BluetoothFindDeviceClose(df);

	return 0;
}

int FindRadios() {
	BLUETOOTH_FIND_RADIO_PARAMS rp;
	HBLUETOOTH_RADIO_FIND rf;
	BLUETOOTH_RADIO_INFO ri;

	HANDLE hRadio = NULL;
	rp.dwSize = sizeof(rp);
	rf = BluetoothFindFirstRadio(&rp, &hRadio);

	do {
		ri.dwSize = sizeof(ri);
		if (BluetoothGetRadioInfo(hRadio, &ri))
			return -1;
		printf("%ws\n", ri.szName);
		FindDevices(hRadio);
	} while ( BluetoothFindNextRadio(&rp, &hRadio) );

	BluetoothFindRadioClose(rf);

	return 0;
}


int main() {
	FindRadios();
	//FindDevices();
}



