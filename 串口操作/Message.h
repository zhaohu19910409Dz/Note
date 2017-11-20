#pragma once

#ifndef _MESSAGE_H__
#define _MESSAGE_H__

enum CommandType
{
	_stopStreamMode = 0x00,
	_startSteamMode,
	_stopFreezeMode,
	_startFreezeMode,
	_pollAndCommand = 0xD0,
	_pollPostion,
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

class Message
{
public:
	typedef struct FDInfo
	{
		CommandType _type;
		BYTE _id;
		BYTE* _data;
		BYTE _ck;
	}_fDInfo;

	Message():_msgbuff(NULL), _pMSGBuff(NULL), _msgsize(0) { }
	~Message() { Reset(); }

	void Reset()
	{
		if (_msgbuff)
		{
			delete [] _msgbuff;
			_msgbuff = NULL;
		}

		_pMSGBuff = NULL;
		_msgsize = 0;
	}

	//从字符串中解析出消息。
	int ParseFromStream(BYTE* buff, unsigned int len, unsigned int* processed)
	{
		if (buff == NULL)
			return 0;
	}

	BYTE* CreateMessage(CommandType type, BYTE _id, BYTE* message, int length)
	{
		//构建消息
		Reset();
		int _length = 1 + 1 + length + 1; //Type + CameraID + Data + Checksum
		_msgbuff = new BYTE[_length];
		//type
		_msgbuff[0] = type;
		//id
		_msgbuff[1] = _id;
		//message
		memcpy(_msgbuff + 2, message, length);
		//checksum
		_msgbuff[_length - 1] = 0xff;

		return _msgbuff;
	}
	//根据传入的消息体构建消息
	void CreateMessage()
	{
		Reset();

		//构建消息

	}

	//获取头信息
	FDInfo* GetMessageHead() { return &_head; };

	//获取消息体
	const BYTE* GetMessageBody() { return _msgbuff; }
protected:
	FDInfo			_head;
	BYTE*			_msgbuff;
	BYTE*			_pMSGBuff;
	unsigned int	_msgsize;

};
#endif //_MESSAGE_H__