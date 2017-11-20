#include "stdafx.h"  

#include <winsock2.h>  
#include <ws2tcpip.h>  
#include "Stdio.h"  
#include <iostream>  
#include "IOE_Timer.h"
#pragma comment(lib, "Ws2_32.lib")
using namespace std;


#define BUFSIZE     1024 //max size of incoming data buffer  

#define DEFAULT_GROUP_ADDRESS   "192.168.0.121"  
#define DEFAULT_PORT            6666   

typedef struct
{
	OVERLAPPED Overlapped;
	WSABUF DataBuf;

	CHAR   Buffer[BUFSIZE];
	//DWORD  BytesSEND;   
	//DWORD  BytesRECV;  
	SOCKADDR_IN SAddr;
} stnIocpData, *pStnIocpData;

typedef struct
{
	u_short l_onoff;
	u_short l_linger;
}stnLinger;

HANDLE g_hIocp;

DWORD WINAPI WorkerThread(LPVOID lpParam);

bool  CreateNetSocket();
BOOL  CreateWorkers(UINT);
void InitWinsock2();
void UnInitWinsock2();

//SOCKET            g_hSocket;  
u_short         nPort = DEFAULT_PORT;

void InitWinsock2()
{
	WSADATA data;
	WORD version;
	int ret = 0;

	version = (MAKEWORD(2, 2));
	ret = WSAStartup(version, &data);
	if (ret != 0)
	{
		ret = WSAGetLastError();
		if (ret == WSANOTINITIALISED)
		{
			printf("not initialised");
		}
	}
}

void UnInitWinsock2()
{
	WSACleanup();
}

bool CreateNetSocket(void)
{
	BOOL fFlag = TRUE;
	int nRet = 0;
	SOCKADDR_IN stLclAddr;

	SOCKET nSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSocket == INVALID_SOCKET)
	{
		printf("socket() failed, Err: %d\n", WSAGetLastError());
		return false;
	}


	nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag));
	if (nRet == SOCKET_ERROR)
	{
		printf("setsockopt() SO_REUSEADDR failed, Err: %d\n", WSAGetLastError());
	}

	/*
	stnLinger Linger;
	Linger.l_onoff = 1;
	Linger.l_linger = 0;
	*/

	/*
	struct linger lng;
	lng.l_onoff = 0;

	nRet = setsockopt(g_hSocket,SOL_SOCKET,SO_LINGER, (char*)&lng, sizeof(lng));
	if (nRet == SOCKET_ERROR)
	{
	nRet = WSAGetLastError();
	printf ("setsockopt() SO_REUSEADDR failed, Err: %d\n",WSAGetLastError());
	}
	*/


	stLclAddr.sin_family = AF_INET;
	stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	stLclAddr.sin_port = htons(nPort);
	nRet = bind(nSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
	if (nRet == SOCKET_ERROR)
	{
		printf("bind() port: %d failed, Err: %d\n", nPort, WSAGetLastError());
	}

	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 10);
	if (!g_hIocp)
	{
		fprintf(stdout, "g_hCompletionPort Create Failed\n");
		return false;
	}

	CreateIoCompletionPort((HANDLE)nSocket, g_hIocp, (DWORD)nSocket, 10);

	pStnIocpData PerIoData;
	if ((PerIoData = new stnIocpData()) == NULL)
	{
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return false;
	}


	ZeroMemory(&(PerIoData->Overlapped), sizeof(OVERLAPPED));
	PerIoData->DataBuf.len = BUFSIZE;
	PerIoData->DataBuf.buf = PerIoData->Buffer;

	DWORD Flags = 0;
	DWORD RecvBytes;

	int iLen = sizeof(PerIoData->SAddr);
	if (WSARecvFrom(nSocket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags,
		(sockaddr*)&(PerIoData->SAddr), &iLen,
		(LPWSAOVERLAPPED)PerIoData, NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			printf("WSARecv() failed with error %d\n", WSAGetLastError());
		}
	}

	return true;
}

BOOL CreateWorkers(UINT uiCount)
{
	DWORD ThreadId;
	HANDLE ThreadHandle;
	DWORD i;

	for (i = 0; i < uiCount; i++)
	{
		ThreadHandle = CreateThread(NULL, 0, WorkerThread, g_hIocp, 0, &ThreadId);
		if (!ThreadHandle)
		{
			fprintf(stdout, "Create Worker Thread Failed\n");
			return FALSE;
		}

		CloseHandle(ThreadHandle);
	}

	return TRUE;
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{

	HANDLE CompletionPort = (HANDLE)lpParam;
	DWORD BytesTransferred;
	LPOVERLAPPED Overlapped;
	memset(&Overlapped, 0, sizeof(Overlapped));
	pStnIocpData PerIoData;
	DWORD RecvBytes;
	DWORD Flags;
	DWORD nSocket;

	while (TRUE)
	{

		if (GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
			&nSocket, (LPOVERLAPPED *)&PerIoData, INFINITE) == 0)
		{
			printf("GetQueuedCompletionStatus failed with error %d\n", GetLastError());
			return 0;
		}

		if (BytesTransferred == 0)
		{
			printf("Closing socket %d\n", nSocket);

			if (closesocket(nSocket) == SOCKET_ERROR)
			{
				return 0;
			}

			GlobalFree(PerIoData);

			continue;
		}

		//BytesTransferred = 0;
		//cout << "nSocket = " << nSocket << endl;

		//cout << "接收的数据: " << PerIoData->DataBuf.buf
		//<< "长度: " << BytesTransferred/*PerIoData->DataBuf.len*/ << endl;

		/*for (int i = 0; i < BytesTransferred; i++)
		{
			printf("%02X ", (BYTE)PerIoData->DataBuf.buf[i]);
		}*/
		//sendto(nSocket, PerIoData->DataBuf.buf, BytesTransferred, 0, (sockaddr*)&(PerIoData->SAddr), sizeof(PerIoData->SAddr));

		/**/
		static bool bHead = false;
		static bool bFirst = false;
		static int nlength = 0;
		static int nIndex = 0;
		static BYTE b1(0), b2(0), b3(0), b4(0);
		static BYTE pB1, pB2, pB3, pB4;
		static BYTE yB1, yB2, yB3, yB4;

		static float fRoll(0.0f), fPitch(0.0f), fYaw(0.0f);
		static float fAx(0.0f), fAy(0.0f), fAz(0.0f);
		for (int i = 0; i < BytesTransferred; i++)
		{
			//printf("%02X ", (BYTE)PerIoData->DataBuf.buf[i]);
			nlength++;
			if ((BYTE)PerIoData->DataBuf.buf[i] == 0xc2)
			{
				if (!bFirst)
				{
					nlength = 0;
					bFirst = true;
				}
				else
				{
					if (nlength == 31)
					{
						bHead = true;
					}
					else
					{
						nlength = 0;
						bFirst = false;
					}
				}
			}

			if (bHead)
			{
				nIndex = nIndex++ % 31;
				if (nIndex == 2)
					b1 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 3)
					b2 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 4)
					b3 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 5)
				{
					b4 = (BYTE)PerIoData->DataBuf.buf[i];
					long int Roll = b1 << 24 | b2 << 16 | b3 << 8 | b4;
					fRoll = *(float*)&Roll;
				}
				

				//Pitch
				if (nIndex == 6)
					pB1 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 7)
					pB2 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 8)
					pB3 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 9)
				{
					pB4 = (BYTE)PerIoData->DataBuf.buf[i];
					long int Pitch = pB1 << 24 | pB2 << 16 | pB3 << 8 | pB4;
					fPitch = *(float*)&Pitch;
				}

				//Yaw
				if (nIndex == 10)
					yB1 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 11)
					yB2 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 12)
					yB3 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 13)
				{
					yB4 = (BYTE)PerIoData->DataBuf.buf[i];
					long int Yaw = yB1 << 24 | yB2 << 16 | yB3 << 8 | yB4;
					fYaw = *(float*)&Yaw;
				}

				//AngRatex
				if (nIndex == 14)
					b1 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 15)
					b2 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 16)
					b3 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 17)
				{
					b4 = (BYTE)PerIoData->DataBuf.buf[i];
					long int AngRateX = b1 << 24 | b2 << 16 | b3 << 8 | b4;
					fAx = *(float*)&AngRateX;
				}

				//AngRatey
				if (nIndex == 18)
					b1 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 19)
					b2 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 20)
					b3 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 21)
				{
					b4 = (BYTE)PerIoData->DataBuf.buf[i];
					long int AngRateX = b1 << 24 | b2 << 16 | b3 << 8 | b4;
					fAy = *(float*)&AngRateX;
				}
				//AngRatez
				if (nIndex == 22)
					b1 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 23)
					b2 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 24)
					b3 = (BYTE)PerIoData->DataBuf.buf[i];
				if (nIndex == 25)
				{
					b4 = (BYTE)PerIoData->DataBuf.buf[i];
					long int AngRateX = b1 << 24 | b2 << 16 | b3 << 8 | b4;
					fAz = *(float*)&AngRateX;

					printf("Accelx = %f, Accely = %f, Accelz = %f, AngRateX = %f, AngRateY = %f, AngRateZ = %f \n ", fRoll, fPitch, fYaw, fAx, fAy, fAz);
				}

			}
		}

		/**/
		Flags = 0;
		ZeroMemory(&(PerIoData->Overlapped), sizeof(OVERLAPPED));

		PerIoData->DataBuf.len = BUFSIZE;
		PerIoData->DataBuf.buf = PerIoData->Buffer;

		int iLen = sizeof(PerIoData->SAddr);
		if (WSARecvFrom(nSocket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, (sockaddr*)&(PerIoData->SAddr), &iLen,
			&(PerIoData->Overlapped), NULL) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
			}
			if (WSAGetLastError() == 10054)
			{
				WSARecvFrom(nSocket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, (sockaddr*)&(PerIoData->SAddr), &iLen,
					&(PerIoData->Overlapped), NULL);
			}
		}

	}

	return 0;

}

//-----------------------------------------------------------------------------  

int main(int argc, char* argv[])
{
	printf("************************************************\n");
	printf("请将IP配置为192.168.1.100，当前监听端口6666\n");
	printf("************************************************\n");

	Timer::InitializeTimerSystem();

	InitWinsock2();

	HANDLE hWait2Exit = CreateEvent(NULL, FALSE, TRUE, _T("MCLIENT"));
	ResetEvent(hWait2Exit);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	if (!CreateNetSocket())
	{
		printf("Error condition CreateNetConnections, exiting\n");
		return 1;
	}

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	if (!CreateWorkers(SystemInfo.dwNumberOfProcessors * 2))
	{
		printf("Error condition CreateWorkers, exiting\n");
		return 1;
	}

	WaitForSingleObject(hWait2Exit, INFINITE);

	UnInitWinsock2();

	Timer::ShutdownTimerSystem();
	return 1;
}