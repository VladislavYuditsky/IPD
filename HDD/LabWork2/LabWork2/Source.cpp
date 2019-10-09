#include <iostream>
#include <iomanip>
#include <windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <conio.h>
#include <sstream>

using namespace std;

#define bThousand 1024
#define Hundred 100
#define BYTE_SIZE 8

string busType[] = { "UNKNOWN", "SCSI", "ATAPI", "ATA", "ONE_TREE_NINE_FOUR", "SSA", "FIBRE", "USB", "RAID", "ISCSI", "SAS", "SATA", "SD", "MMC" };

void getDeviceInfo(HANDLE diskHandle, STORAGE_PROPERTY_QUERY storageProtertyQuery)
{
	STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader = { 0 };
	DWORD dwBytesReturned = 0;
	if (!DeviceIoControl(diskHandle, IOCTL_STORAGE_QUERY_PROPERTY,
		&storageProtertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		&storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER),
		&dwBytesReturned, NULL))
	{
		printf("%d", GetLastError());
		CloseHandle(diskHandle);
		exit(-1);
	}


	// Alloc the output buffer
	const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
	BYTE* pOutBuffer = new BYTE[dwOutBufferSize];
	ZeroMemory(pOutBuffer, dwOutBufferSize);


	// Get the storage device descriptor
	if (!DeviceIoControl(diskHandle, IOCTL_STORAGE_QUERY_PROPERTY,
		&storageProtertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		pOutBuffer, dwOutBufferSize,
		&dwBytesReturned, NULL))
	{
		printf("%d", GetLastError());
		CloseHandle(diskHandle);
		exit(-1);
	}

	STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuffer;
	const DWORD dwSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;
	const DWORD dwFirmWareOffset = pDeviceDescriptor->BusType;
	const DWORD dwModelOffset = pDeviceDescriptor->ProductIdOffset;
	const DWORD dwProductRevisionOffset = pDeviceDescriptor->ProductRevisionOffset;
	const DWORD dwManufactureOffset = pDeviceDescriptor->VendorIdOffset;


	if (dwSerialNumberOffset != 0)
	{
		cout << "HDD INFO:" << endl << endl;
		cout << "Model: " << (char*)(pOutBuffer + dwModelOffset) << endl;
		cout << "Manufacture: " << "Seagate Technology LLC" << endl;
		cout << "Serial number: " << (char*)(pOutBuffer + dwSerialNumberOffset) << endl;
		cout << "Firmware: " << (char*)(pOutBuffer + dwProductRevisionOffset) << endl;
		cout << "Interface type: " << busType[pDeviceDescriptor->BusType] << endl;
	}
}

void getMemoryInfo() {

	int n;
	char dd[5];
	DWORD dr = GetLogicalDrives();

	unsigned __int64 totalNumberOfBytes = 0;
	unsigned __int64 totalNumberOfBytesOnDisk = 0;

	unsigned __int64 totalNumberOfFreeBytes = 0;
	unsigned __int64 totalNumberOfFreeBytesOnDisk = 0;

	unsigned __int64 freeBytesAvailable = 0;
	unsigned __int64 freeBytesAvailableOnDisk = 0;

	for (int i = 0; i < 26; i++)
	{
		n = ((dr >> i) & 0x00000001);
		if (n == 1)
		{
			dd[0] = char(65 + i); dd[1] = ':'; dd[2] = '\\'; dd[3] = '\\'; dd[4] = '\0';
			/*qDebug() << "Available disk drives : " << dd;*/
		}
		else continue;
		bool GetDiskFreeSpaceFlag = GetDiskFreeSpaceExA(dd,
			(PULARGE_INTEGER)& freeBytesAvailableOnDisk,
			(PULARGE_INTEGER)& totalNumberOfBytesOnDisk,
			(PULARGE_INTEGER)& totalNumberOfFreeBytesOnDisk);
		if (GetDiskFreeSpaceFlag != 0)
		{
			totalNumberOfBytes += totalNumberOfBytesOnDisk;
			totalNumberOfFreeBytes += totalNumberOfFreeBytesOnDisk;
		}
	}

	cout << "Total memory: " << totalNumberOfBytes << " bytes = " << (double)totalNumberOfBytes / (1024 * 1024 * 1024) << " Gb" << endl;
	cout << "Free memory: " << totalNumberOfFreeBytes << " bytes = " << (double)(totalNumberOfFreeBytes) / (1024 * 1024 * 1024) << " Gb" << endl;
	cout << "Occupied memory: " << totalNumberOfBytes - totalNumberOfFreeBytes << " bytes = " << (double)(totalNumberOfBytes - totalNumberOfFreeBytes) / (1024 * 1024 * 1024) << " Gb" << endl;
}

void getAtaPioDmaSupportStandarts(HANDLE diskHandle) {

	UCHAR identifyDataBuffer[512 + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };

	ATA_PASS_THROUGH_EX& PTE = *(ATA_PASS_THROUGH_EX*)identifyDataBuffer;	//Структура для отправки АТА команды устройству 
	PTE.Length = sizeof(PTE);
	PTE.TimeOutValue = 10;									//Размер структуры 
	PTE.DataTransferLength = 512;							//Размер буфера для данных 
	PTE.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);		//Смещение в байтах от начала структуры до буфера данных 
	PTE.AtaFlags = ATA_FLAGS_DATA_IN;						//Флаг, говорящий о чтении байтов из устройства 

	IDEREGS* ideRegs = (IDEREGS*)PTE.CurrentTaskFile;
	ideRegs->bCommandReg = 0xEC;

	//Производим запрос устройству 
	if (!DeviceIoControl(diskHandle,
		IOCTL_ATA_PASS_THROUGH,								//посылаем структуру с командами типа ATA_PASS_THROUGH_EX
		&PTE,
		sizeof(identifyDataBuffer),
		&PTE,
		sizeof(identifyDataBuffer),
		NULL,
		NULL)) {
		cout << GetLastError() << std::endl;
		return;
	}
	//Получаем указатель на массив полученных данных 
	WORD* data = (WORD*)(identifyDataBuffer + sizeof(ATA_PASS_THROUGH_EX));
	short ataSupportByte = data[80];
	int i = 2 * BYTE_SIZE;
	int bitArray[2 * BYTE_SIZE];
	//Превращаем байты с информацией о поддержке ATA в массив бит 
	while (i--) {
		bitArray[i] = ataSupportByte & 32768 ? 1 : 0;
		ataSupportByte = ataSupportByte << 1;
	}


	cout << "Supported modes: " << endl << "  - ATA Support: ";
	//Анализируем полученный массив бит. 
	for (int i = 8; i >= 4; i--) {
		if (bitArray[i] == 1) {
			cout << "ATA" << i;
			if (i != 4) {
				cout << ", ";
			}
		}
	}
	cout << endl;

	//Вывод поддерживаемых режимов DMA 
	unsigned short dmaSupportedBytes = data[63];
	int i2 = 2 * BYTE_SIZE;
	//Превращаем байты с информацией о поддержке DMA в массив бит
	while (i2--) {
		bitArray[i2] = dmaSupportedBytes & 32768 ? 1 : 0;
		dmaSupportedBytes = dmaSupportedBytes << 1;
	}


	//Анализируем полученный массив бит. 
	cout << "  - DMA Support: ";
	for (int i = 0; i < 8; i++) {
		if (bitArray[i] == 1) {
			cout << "DMA" << i;
			if (i != 2) cout << ", ";
		}
	}
	cout << endl;

	unsigned short pioSupportedBytes = data[64];
	int i3 = 2 * BYTE_SIZE;
	//Превращаем байты с информацией о поддержке PIO в массив бит 
	while (i3--) {
		bitArray[i3] = pioSupportedBytes & 32768 ? 1 : 0;
		pioSupportedBytes = pioSupportedBytes << 1;
	}

	//Анализируем полученный массив бит. 
	cout << "  - PIO Support: ";
	for (int i = 0; i < 2; i++) {
		if (bitArray[i] == 1) {
			cout << "PIO" << i + 3;
			if (i != 1) cout << ", ";
		}
	}
	cout << endl;
}

void getMemoryTransferMode(HANDLE diskHandle, STORAGE_PROPERTY_QUERY storageProtertyQuery) {
	STORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;			//Структура со свойствами устройства 
	if (!DeviceIoControl(diskHandle,
		IOCTL_STORAGE_QUERY_PROPERTY,						//Отправляем запрос на возврат свойств устройства. 
		&storageProtertyQuery,
		sizeof(storageProtertyQuery),
		&adapterDescriptor,
		sizeof(STORAGE_DESCRIPTOR_HEADER),
		NULL,
		NULL)) {
		cout << GetLastError();
		exit(-1);
	}
	else {
		//Вывод режима доступа к памяти
		cout << "  - Transfer mode: ";
		adapterDescriptor.AdapterUsesPio ? cout << "PIO" : cout << "DMA";
		cout << setw(29) << "|" << endl;
	}
}

void init(HANDLE& diskHandle, char* name) {
	//Открытие файла с информацией о диске 
	diskHandle = CreateFile(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (diskHandle == INVALID_HANDLE_VALUE) {
		cout << "Done..";
		_getch();
		return;
	}
}

int main()
{
	STORAGE_PROPERTY_QUERY storagePropertyQuery;				//Структура с информацией об запросе 
	storagePropertyQuery.QueryType = PropertyStandardQuery;		//Запрос драйвера, чтобы он вернул дескриптор устройства. 
	storagePropertyQuery.PropertyId = StorageDeviceProperty;	//Флаг, гооврящий мы хотим получить дескриптор устройства. 
	HANDLE diskHandle;

	const string name = "//./PhysicalDrive";
	int number = 0;

	stringstream* res_name = new stringstream;

	*res_name << name << number;

	init(diskHandle, (char*)res_name->str().c_str());

	delete res_name;
	res_name = new stringstream;

	getDeviceInfo(diskHandle, storagePropertyQuery);
	getMemoryInfo();
	getAtaPioDmaSupportStandarts(diskHandle);
	getMemoryTransferMode(diskHandle, storagePropertyQuery);

	_getch();
	cout << endl;

	return 0;
}