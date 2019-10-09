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
	char dd[4];
	DWORD dr = GetLogicalDrives();

	unsigned __int64 totalNumberOfBytes = 0;
	unsigned __int64 totalNumberOfBytesOnDisk = 0;

	unsigned __int64 totalNumberOfFreeBytes = 0;
	unsigned __int64 totalNumberOfFreeBytesOnDisk = 0;

	for (int i = 0; i < 26; i++)
	{
		n = ((dr >> i) & 0x00000001);
		if (n == 1)
		{
			dd[0] = char(65 + i); dd[1] = ':'; dd[2] = '\\'; dd[3] = '\0';
		}
		else continue;
		bool GetDiskFreeSpaceFlag = GetDiskFreeSpaceExA(dd,
			0,
			(PULARGE_INTEGER)& totalNumberOfBytesOnDisk,
			(PULARGE_INTEGER)& totalNumberOfFreeBytesOnDisk);
		if (GetDiskFreeSpaceFlag != 0)
		{
			totalNumberOfBytes += totalNumberOfBytesOnDisk;
			totalNumberOfFreeBytes += totalNumberOfFreeBytesOnDisk;
			cout << "Total memory on " << dd << totalNumberOfBytesOnDisk << " bytes(" << (double)totalNumberOfBytesOnDisk / (1024 * 1024 * 1024) << " Gb)" << endl;
			cout << "Free memory on " << dd << totalNumberOfFreeBytesOnDisk << " bytes(" << (double)totalNumberOfFreeBytesOnDisk / (1024 * 1024 * 1024) << " Gb)" << endl;
			cout << "Occupied memory on " << dd << totalNumberOfBytesOnDisk - totalNumberOfFreeBytesOnDisk << " bytes(" << (double)(totalNumberOfBytesOnDisk - totalNumberOfFreeBytesOnDisk) / (1024 * 1024 * 1024) << " Gb)" << endl;
			cout << endl;
		}
	}

	cout << "Total memory: " << totalNumberOfBytes << " bytes = " << (double)totalNumberOfBytes / (1024 * 1024 * 1024) << " Gb" << endl;
	cout << "Free memory: " << totalNumberOfFreeBytes << " bytes = " << (double)(totalNumberOfFreeBytes) / (1024 * 1024 * 1024) << " Gb" << endl;
	cout << "Occupied memory: " << totalNumberOfBytes - totalNumberOfFreeBytes << " bytes = " << (double)(totalNumberOfBytes - totalNumberOfFreeBytes) / (1024 * 1024 * 1024) << " Gb" << endl;
}

void getAtaPioDmaSupportStandarts(HANDLE diskHandle) {

	UCHAR identifyDataBuffer[512 + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };

	ATA_PASS_THROUGH_EX& PTE = *(ATA_PASS_THROUGH_EX*)identifyDataBuffer;	//��������� ��� �������� ��� ������� ���������� 
	PTE.Length = sizeof(PTE);
	PTE.TimeOutValue = 10;									//������ ��������� 
	PTE.DataTransferLength = 512;							//������ ������ ��� ������ 
	PTE.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);		//�������� � ������ �� ������ ��������� �� ������ ������ 
	PTE.AtaFlags = ATA_FLAGS_DATA_IN;						//����, ��������� � ������ ������ �� ���������� 

	IDEREGS* ideRegs = (IDEREGS*)PTE.CurrentTaskFile;
	ideRegs->bCommandReg = 0xEC;

	//���������� ������ ���������� 
	if (!DeviceIoControl(diskHandle,
		IOCTL_ATA_PASS_THROUGH,								//�������� ��������� � ��������� ���� ATA_PASS_THROUGH_EX
		&PTE,
		sizeof(identifyDataBuffer),
		&PTE,
		sizeof(identifyDataBuffer),
		NULL,
		NULL)) {
		cout << GetLastError() << std::endl;
		return;
	}
	//�������� ��������� �� ������ ���������� ������ 
	WORD* data = (WORD*)(identifyDataBuffer + sizeof(ATA_PASS_THROUGH_EX));
	short ataSupportByte = data[80];
	int i = 2 * BYTE_SIZE;
	int bitArray[2 * BYTE_SIZE];
	//���������� ����� � ����������� � ��������� ATA � ������ ��� 
	while (i--) {
		bitArray[i] = ataSupportByte & 32768 ? 1 : 0;
		ataSupportByte = ataSupportByte << 1;
	}


	cout << "Supported modes: " << endl << "  - ATA Support: ";
	//����������� ���������� ������ ���. 
	for (int i = 8; i >= 4; i--) {
		if (bitArray[i] == 1) {
			cout << "ATA" << i;
			if (i != 4) {
				cout << ", ";
			}
		}
	}
	cout << endl;

	//����� �������������� ������� DMA 
	unsigned short dmaSupportedBytes = data[63];
	int i2 = 2 * BYTE_SIZE;
	//���������� ����� � ����������� � ��������� DMA � ������ ���
	while (i2--) {
		bitArray[i2] = dmaSupportedBytes & 32768 ? 1 : 0;
		dmaSupportedBytes = dmaSupportedBytes << 1;
	}

	//����������� ���������� ������ ���. 
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
	//���������� ����� � ����������� � ��������� PIO � ������ ��� 
	while (i3--) {
		bitArray[i3] = pioSupportedBytes & 32768 ? 1 : 0;
		pioSupportedBytes = pioSupportedBytes << 1;
	}

	//����������� ���������� ������ ���. 
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
	STORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;			//��������� �� ���������� ���������� 
	if (!DeviceIoControl(diskHandle,
		IOCTL_STORAGE_QUERY_PROPERTY,						//���������� ������ �� ������� ������� ����������. 
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
		//����� ������ ������� � ������
		cout << "  - Transfer mode: ";
		adapterDescriptor.AdapterUsesPio ? cout << "PIO" : cout << "DMA";
	}
}

void init(HANDLE& diskHandle, string name) {
	//�������� ����� � ����������� � ����� 
	diskHandle = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (diskHandle == INVALID_HANDLE_VALUE) {
		cout << "Done..";
		_getch();
		return;
	}
}

int main()
{
	STORAGE_PROPERTY_QUERY storagePropertyQuery;				//��������� � ����������� �� ������� 
	storagePropertyQuery.QueryType = PropertyStandardQuery;		//������ ��������, ����� �� ������ ���������� ����������. 
	storagePropertyQuery.PropertyId = StorageDeviceProperty;	//����, ��������� �� ����� �������� ���������� ����������. 
	HANDLE diskHandle;

	const string name = "//./PhysicalDrive0";

	init(diskHandle, name);

	getDeviceInfo(diskHandle, storagePropertyQuery);
	getMemoryInfo();
	getAtaPioDmaSupportStandarts(diskHandle);
	getMemoryTransferMode(diskHandle, storagePropertyQuery);

	_getch();
	cout << endl;

	return 0;
}