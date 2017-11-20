// TestSerialComm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SerialPort.h"
#include <iostream>
#include "Message.h"
using namespace std;
#if 0
#using "SerialComm.dll"
using namespace SerialComm;


typedef struct FDCommand
{
	BYTE _type;
	BYTE _id;
	BYTE* _command;
	BYTE _ck;
	int _length;
};

enum CommandType
{
	_stopStreamMode = 0x00,
	_startSteamMode,
	_stopFreezeMode,
	_startFreezeMode,
	_pollPostion = 0xD1,
	_requestSysStatus,
	_requestSysParam,
	_requestFirstMarker,
	_requestNextMarker,
	_requestFirstImg,
	_requestNextImg,
	_requestNextROMData,
	_requestCameraCalibratVal = 0xDA,
	_requestDiagnosticMode
};

void CallBacK(int a)
{
	int sum = a;
}

int _tmain(int argc, _TCHAR* argv[])
{
	Class1 ^c = gcnew Class1();
	c->SetCallBack(CallBacK);
	c->WriteData();
	return 0;
}
#endif

int _tmain(int argc, _TCHAR* argv[])    
{    

	CSerialPort mySerialPort;    

	if (!mySerialPort.InitPort(2))    
	{    
		std::cout << "initPort fail !" << std::endl;    
	}    
	else   
	{    
		std::cout << "initPort success !" << std::endl;    
	}    

	if (!mySerialPort.OpenListenThread())    
	{    
		std::cout << "OpenListenThread fail !" << std::endl;    
	}    
	else   
	{    
		std::cout << "OpenListenThread success !" << std::endl;    
	}

	//Write Message:
	Message *msg = new Message();
	while (true)
	{
		BYTE info[1] = { _requestSysStatus};
		BYTE* data = msg->CreateMessage(_pollAndCommand,0x01, info, sizeof(info));
		mySerialPort.WriteData(data, sizeof(info) + 3);
	}

	int temp;    
	std::cin >> temp;    

	return 0;    
}    


