#ifndef __IIS_HELPERS_H__
#define __IIS_HELPERS_H__

#define WIN32_LEAN_AND_MEAN

//  IIS7 Server API header file
#include <Windows.h>
#include <sal.h>
#include <strsafe.h>
#include "httpserv.h"

#include "winsock2.h"


#include <string>
#pragma once
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
	static bool WriteResponse(IHttpContext* pHttpContext, const std::string &content);
};

#endif