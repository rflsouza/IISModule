#ifndef __IIS_HELPERS_H__
#define __IIS_HELPERS_H__
#pragma once

#define WIN32_LEAN_AND_MEAN

//  IIS7 Server API header file
#include <Windows.h>
#include <sal.h>
#include <strsafe.h>
#include "httpserv.h"

#include "winsock2.h"


#include <string>
#include <map>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <atomic>

enum EHTTPSamples
{
	WEB_SOCKET = 0,
	WEB_REQUEST_GET = 1,
	WEB_REQUEST_POST_DATA = 2,
	WEB_REQUEST_POST_FILE = 3,
	WEB_REQUEST_ASYNC_THREAD = 4

} EHTTPSamples;

struct IISCounter {
	std::atomic_ullong requests;
	std::atomic_ullong websockets;
	std::atomic_ullong websocketsRead;
	std::atomic_ullong websocketsWrite;
	IISCounter() :
		requests(ATOMIC_VAR_INIT(0)),
		websockets(ATOMIC_VAR_INIT(0)),
		websocketsRead(ATOMIC_VAR_INIT(0)),
		websocketsWrite(ATOMIC_VAR_INIT(0)) {}
};

class IISHelpers
{
public:
	//To information for an application running on WOW64
	SYSTEM_INFO			systemInfo;
	DWORD				dwPageSizeSystem;	

	IISHelpers();
	~IISHelpers();

	/*
	 * This is the function invoked by RegisterModule
	 * when the agent module DLL is loaded at startup.
	 * */
	BOOL RegisterIIHelpersModule();

	static std::string GetIpAddr(PSOCKADDR pAddr);
	static SOCKADDR_IN ToIpAddr(std::string textIp);
	static std::string ConvertUTF16ToUTF8(__in const WCHAR * pszTextUTF16, size_t cchUTF16);
	
	/*
	 * Retrives the server variables using GetServerVariable.
	 *   //sample: Get the original headers from the request
	 *   status = GetVariable(pHttpContext,"ALL_RAW", &httpHeaders, &httpHeadersSize, TRUE);
	*/
	static bool GetVariable(IHttpContext* pHttpContext, PCSTR varName, PCSTR* pVarVal, DWORD* pVarValSize, BOOL isRequired);
	/*
	* Retrieves entity data from the request.	
	* */
	static bool GetEntity(IHttpContext* pHttpContext, std::string& data);

	/*
	* write data from the request.
	* */
	static HRESULT WriteResponse(IHttpContext* pHttpContext, const std::string &content);
};


std::string ErrorHandler(LPTSTR lpszFunction);

#endif