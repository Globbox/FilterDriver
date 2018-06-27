

#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <fltuser.h>

#pragma comment (lib, "fltLib.lib")

using namespace std;


#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

#define BUFFER_SIZE   512
#define PORT_NAME L"\\ScannerPort"


typedef struct _SCANER_DATA_IN
{
	UCHAR f_path[BUFFER_SIZE];
	UCHAR f_pross[BUFFER_SIZE];
	UCHAR act[BUFFER_SIZE];

} SC_DATA, *PSC_DATA;

typedef struct _DataMessage

{

	FILTER_MESSAGE_HEADER   MsgHeader;

	SC_DATA data;

}MESSDATA, *PMESSDATA;

HANDLE port;
int main()
{
	PMESSDATA udtData = NULL;
	PSC_DATA tmp = NULL;



	HRESULT res;
	res = FilterConnectCommunicationPort(PORT_NAME,
		0,
		NULL,
		0,
		NULL,
		&port);

	if (IS_ERROR(res)) {
		printf("Driver error!");
		//_getch();
		return 0;
	}




	udtData = (PMESSDATA)malloc(sizeof(MESSDATA));
	tmp = (PSC_DATA)malloc(sizeof(SC_DATA));
	while (true)
	{

		FilterGetMessage(port, &udtData->MsgHeader, sizeof(MESSDATA), NULL);
		tmp = &udtData->data;


		printf("Path: %s\n", tmp->f_path);
		printf("Prosses: %s\n", tmp->f_pross);
		printf("Action: %s\n", tmp->act);
		
	
		
		
		
		if (_kbhit())
		{
			if (_getche() == 'q')
					break;
		}
	}




	free(udtData);
	_getch();
	return 0;

}


