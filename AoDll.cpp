// AoDll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "detours.h"
#include <string>
#include <comutil.h>
#include "AoDll.h"

 
#pragma comment(lib, "comsuppw.lib")

using namespace std;

//
//// Methods
//
VOID WINAPI MyRecvData(BSTR recvData);		// My DataHandler Method
VOID WINAPI MySendData(BSTR* sendData);		// My SendData Method
int WINAPI MyLoop();						// My Loop Method

//
//// Auxiliar Methods
//
string ConvertWCSToMBS(const wchar_t* pstr, long wslen);
BSTR ConvertStringToBSTR(const std::string & str);
BOOL StartsWith(BSTR sValue, const WCHAR* pszSubValue);
string ConvertBSTRToString(BSTR bstr);
VOID SendToClient(string message);
VOID SendToServer(string message);

//
//// Pointer Functions Declarations
//
typedef VOID(WINAPI *PRecvData)(BSTR data);    //Pointer Definition - Takes BSTR and returns VOID
PRecvData PFunctionRecv = (PRecvData)0x655B10; //Pointer to where the original HandleData() starts

typedef VOID(WINAPI *PSendData)(BSTR* data);   //Pointer Definition - Takes BSTR* and returns VOID
PSendData PFunctionSend = (PSendData)0x6A21C0; //Pointer to where the original SendData() starts

typedef int(WINAPI *PLoop)();
HMODULE dllModule = LoadLibraryA("MSVBVM60.DLL");							// The Address for the Loop Function.
PLoop PFunctionLoop = (PLoop)GetProcAddress(dllModule, "rtcDoEvents");		//  We get it dinamically as it is a Windows Function



//
//// Main Methods
//
void Hooks()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)PFunctionRecv, &MyRecvData);      // Hook DataHandle() to MyRecvData()
	DetourAttach(&(PVOID&)PFunctionSend, &MySendData);      // Hook SendData() to MySendData()
	DetourAttach(&(PVOID&)PFunctionLoop, &MyLoop);          // Hook DoEvents AKA Loop() to MyLoop()
	DetourTransactionCommit();
}

VOID WINAPI MyRecvData(BSTR dataRecv)
{
	__asm PUSHAD;
	__asm PUSHFD;

	//
	//// Prints paquets in Recv(Paquets from server to client)
	//
	OutputDebugStringW(ConvertStringToBSTR("Recv: " + ConvertBSTRToString(dataRecv)));

	//
	//// Returns control to the Original Function
	//
	PFunctionRecv(dataRecv);


	__asm POPFD;
	__asm POPAD;

}

VOID WINAPI MySendData(BSTR* dataSend)
{

	__asm PUSHAD;
	__asm PUSHFD;

	//
	//// Prints paquets in Send(Paquets from Client to Server)
	//
	OutputDebugStringW(ConvertStringToBSTR("Send: " + ConvertBSTRToString(*dataSend)));

	//
	//// Returns control to the Original Function
	//
	PFunctionSend(dataSend);

	__asm POPFD;
	__asm POPAD;

}

int WINAPI MyLoop()
{
	__asm PUSHAD;
	__asm PUSHFD;

	//
	//// Returns control to the Original Function
	//
	PFunctionLoop();

	__asm POPFD;
	__asm POPAD;
}

VOID SendToClient(string message)
{
	try
	{
		//
		//// Create Packet
		//
		BSTR recvPacket = ConvertStringToBSTR(message);
		OutputDebugStringW(ConvertStringToBSTR("SendToClient: " + ConvertBSTRToString(recvPacket)));
		//
		//// Send Packet
		//
		PFunctionRecv(recvPacket);
		//
		//// Free Packet
		//
		SysFreeString(recvPacket);
	}
	catch (int e)
	{

	}
}

VOID SendToServer(string message)
{
	try
	{
		//
		//// Create Packet
		//
		BSTR convertedSend = ConvertStringToBSTR(message);
		OutputDebugStringW(ConvertStringToBSTR("SendToServer: " + ConvertBSTRToString(convertedSend)));
		//
		//// Send Packet
		//
		PFunctionSend(&convertedSend);
		//
		//// Free Packet
		//
		SysFreeString(convertedSend);
	}
	catch (int e)
	{
	}
}


//
//// Auxiliar Methods
//

BOOL StartsWith(BSTR sValue, const WCHAR* pszSubValue)
{
	if (!sValue)
		return FALSE;
	return wcsncmp(sValue, pszSubValue, wcslen(pszSubValue)) == 0;
}

string ConvertWCSToMBS(const wchar_t* pstr, long wslen)
{
	int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

	std::string dblstr(len, '\0');
	len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
		pstr, wslen /* not necessary NULL-terminated */,
		&dblstr[0], len,
		NULL, NULL /* no default char */);

	return dblstr;
}

string ConvertBSTRToString(BSTR bstr)
{
	int wslen = ::SysStringLen(bstr);
	return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}

BSTR ConvertStringToBSTR(const std::string & str)
{
	int wslen = ::MultiByteToWideChar(CP_ACP, 0 /* no flags */,
		str.data(), str.length(),
		NULL, 0);

	BSTR wsdata = ::SysAllocStringLen(NULL, wslen);
	::MultiByteToWideChar(CP_ACP, 0 /* no flags */,
		str.data(), str.length(),
		wsdata, wslen);
	return wsdata;
}

