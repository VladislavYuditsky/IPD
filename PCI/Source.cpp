#pragma comment (lib, "Setupapi.lib")

#include <stdio.h>

#include <Windows.h>
#include <SetupAPI.h>
#include <locale.h>

#include <iostream>
#include <iomanip> 
#include <string>
#include <regstr.h> 


using namespace std;

int main() {
	setlocale(0, "");
	const DWORD SIZE = 4000;

	HDEVINFO DeviceInfoSet;						// дескриптор набора информации об устройстве
	DeviceInfoSet = SetupDiGetClassDevs(NULL,	// указатель класса настройки устройства (необязательно)
		REGSTR_KEY_PCIENUM,					    //	Enumerator - идентификатор экземпляра устройства
		NULL,							        // hwndParent - окно верхнего уровня экземпляра 
										        // пользовательского интерфейса (необязательно)
		DIGCF_PRESENT | DIGCF_ALLCLASSES        // Flags - устройва доступные в данный момент | все установленные
	);

	SP_DEVINFO_DATA DeviceInfoData;			    // структура представляет информацию об устройстве
	ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	DWORD deviceNum = 0;

	cout << setw(3) << left << "#"
		<< setw(90) << left << " Device description"
		<< setw(10) << left << "Device ID"
		<< setw(10) << left << "Vendor ID" << endl << endl;

	while (SetupDiEnumDeviceInfo(				// выделяет устройства из полученного набора в DeviseInfoSet
		DeviceInfoSet,
		deviceNum,
		&DeviceInfoData)) {
		deviceNum++;

		TCHAR bufferID[SIZE];
		ZeroMemory(bufferID, sizeof(bufferID));

		TCHAR bufferName[SIZE];
		ZeroMemory(bufferName, sizeof(bufferName));

		SetupDiGetDeviceInstanceId(DeviceInfoSet,				// набор устройств
			&DeviceInfoData,									// конкретное устройство из набора
			bufferID,											// куда записать идентификатор 
			sizeof(bufferID),									// размер буфера для записи
			NULL												// сколько символов занимает идендификатор
		);

		SetupDiGetDeviceRegistryProperty(DeviceInfoSet,			// набор устройств
			&DeviceInfoData,									// конкретное устройство из набора
			SPDRP_DEVICEDESC,									// Property - получить описание устойства
			NULL,												// получаемый тип данных
			(PBYTE)bufferName,									// куда записать имя
			sizeof(bufferName),									// размер буфера
			NULL												// сколько символов занимает идендификатор 
		);

		string deviceName(bufferName);
		string deviceAndVendorID(bufferID);

		string vendorID = deviceAndVendorID.substr(8, 4);
		string deviceID = deviceAndVendorID.substr(17, 4);


		cout << setw(3) << left << deviceNum;
		cout << setw(90) << left << deviceName;
		cout << setw(10) << left << deviceID;
		cout << setw(10) << left << vendorID << endl;
	}

	if (DeviceInfoSet) {
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	}
	system("pause");
	return 0;
}