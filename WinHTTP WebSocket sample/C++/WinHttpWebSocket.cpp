// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

// Thread ref: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-threads
// WinHTTP WebSocket sample  ref: https://code.msdn.microsoft.com/windowsdesktop/WinHTTP-WebSocket-sample-50a140b5
// https://docs.microsoft.com/en-us/windows/win32/websock/web-socket-protocol-component-api-portal
// https://docs.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpopenrequest

#include <Windows.h>
#include <WinHttp.h>
#include <stdio.h>

#include <tchar.h>
#include <strsafe.h>
#include "../../CLog.h"

#define MAX_THREADS 50 //MAXIMUM_WAIT_OBJECTS
#define BUF_SIZE 255

#define HTTP_TIMEOUT_RESOLVE	120000	// nResolveTimeout Default o time-out (infinite).
#define HTTP_TIMEOUT_CONNECT	120000	// nConnectTimeout Default 60000 (60 seconds).		
#define HTTP_TIMEOUT_SEND		90000	// nSendTimeout  Default 60000 (60 seconds).	
#define HTTP_TIMEOUT_RECEIVE	90000	// nReceiveTimeout  Default 60000 (60 seconds).	

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).
typedef struct MyData {
	int idArray;
	CLog *p_log;
} MYDATA, *PMYDATA;

String ErrorHandler(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code.

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	
	//MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);	
	//wprintf(L"\n[%u] Error - Message: [%s]\n", GetCurrentThreadId() ,(LPCTSTR)lpDisplayBuf);
	String returnString = String(_T("ERROR ")) + (LPCTSTR)lpDisplayBuf + _T(" ThreadId:") +  to_String(GetCurrentThreadId());

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	return returnString;
}

DWORD WINAPI threadWebSocket(LPVOID lpParam)
{
	DWORD dwError = ERROR_SUCCESS;
	BOOL fStatus = FALSE;
	HINTERNET hSessionHandle = NULL;
	HINTERNET hConnectionHandle = NULL;
	HINTERNET hRequestHandle = NULL;
	HINTERNET hWebSocketHandle = NULL;
	BYTE rgbCloseReasonBuffer[123];
	BYTE rgbBuffer[1024];
	BYTE *pbCurrentBufferPointer = rgbBuffer;
	DWORD dwBufferLength = ARRAYSIZE(rgbBuffer);
	DWORD dwBytesTransferred = 0;
	DWORD dwCloseReasonLength = 0;
	USHORT usStatus = 0;
	WINHTTP_WEB_SOCKET_BUFFER_TYPE eBufferType;
	INTERNET_PORT Port = INTERNET_DEFAULT_HTTP_PORT;
	//const WCHAR *pcwszServerName = L"localhost";
	//const WCHAR *pcwszPath = L"/WinHttpWebSocketSample/EchoWebSocket.ashx";

	const WCHAR *pcwszServerName = L"localhost";
	// ws://xwcteste.xgen.com.br/iis-module/websocket
	const WCHAR *pcwszPath = L"/iis-module/websocket";
	const WCHAR *pcwszMessage = L"Hello world";
	const DWORD cdwMessageLength = ARRAYSIZE(L"Hello world") * sizeof(WCHAR);

	CLog *p_log;
	OStringstream strLog;

	PMYDATA pDataArray;
	pDataArray = (PMYDATA)lpParam;

	// Print the parameter values using thread-safe functions.

	//StringCchPrintf(msgBuf, BUF_SIZE, TEXT("Parameters = %d, %d\n"), pDataArray->val1, pDataArray->val2);
	DWORD threadId = GetCurrentThreadId();
	DWORD tick;
	
	TCHAR myPath[_MAX_PATH + 1];
	GetModuleFileName(NULL, myPath, _MAX_PATH);
	String appPath = myPath;
	appPath = appPath.substr(0, appPath.rfind(_T('\\'))) + _T("\\");
	String appPrefix (_T("websocket-client-") + to_String(pDataArray->idArray) + _T("-") + to_String(threadId));
	std::string appPrefixSTDstring("websocket-client-" + std::to_string(pDataArray->idArray) + "-" + std::to_string(threadId));

	if (pDataArray->p_log) {
		p_log = pDataArray->p_log;
	}
	else {
		p_log = new CLog(appPath, appPrefix);
	}
	

	strLog << _T("START") << _T(" ") << appPrefix;
	p_log->write(&strLog);

	strLog << _T("Path: ") << appPath << _T(" appPrefix:") << appPrefix;
	p_log->write(&strLog);


	tick = GetTickCount();
	strLog << _T("Start WinHttpOpen [") << tick << _T("]");
	p_log->write(&strLog);
	
	// Create session, connection and request handles.
	// Use WinHttpOpen to obtain a session handle.
	hSessionHandle = WinHttpOpen(appPrefix.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
	
	strLog << _T("End WinHttpOpen [") << tick << _T("] duration:" << GetTickCount()-tick);
	p_log->write(&strLog);
	if (hSessionHandle == NULL)
	{
		strLog << ErrorHandler(L"WinHttpOpen");
		p_log->write(&strLog, true);
		goto quit;
	}

	// Use WinHttpSetTimeouts to set a new time-out values.
	// Set because stresstest execute many fast startups.
	if (!WinHttpSetTimeouts(hSessionHandle, HTTP_TIMEOUT_RESOLVE, HTTP_TIMEOUT_CONNECT, HTTP_TIMEOUT_SEND, HTTP_TIMEOUT_RECEIVE)) {		
		strLog << ErrorHandler(L"WinHttpSetTimeouts");
		p_log->write(&strLog, true);
	}

	tick = GetTickCount();
	strLog << _T("Start WinHttpConnect [") << tick << _T("]");
	p_log->write(&strLog);
	
	// Specify an HTTP server.
	hConnectionHandle = WinHttpConnect(hSessionHandle, pcwszServerName, Port, 0);

	strLog << _T("End WinHttpConnect [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);

	if (hConnectionHandle == NULL)
	{
		strLog << ErrorHandler(L"WinHttpConnect");
		p_log->write(&strLog, true);
		goto quit;
	}


	tick = GetTickCount();
	strLog << _T("Start WinHttpOpenRequest [") << tick << _T("]");
	p_log->write(&strLog);
	
	// Create an HTTP Request handle.
	hRequestHandle = WinHttpOpenRequest(hConnectionHandle, L"GET", pcwszPath, NULL, NULL, NULL, 0);

	strLog << _T("End WinHttpOpenRequest [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);

	if (hRequestHandle == NULL)
	{
		strLog << ErrorHandler(L"WinHttpOpenRequest");
		p_log->write(&strLog, true);
		goto quit;
	}

	//
	// Request protocol upgrade from http to websocket.
	//
#pragma prefast(suppress:6387, "WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET does not take any arguments.")
	tick = GetTickCount();
	strLog << _T("Start WinHttpSetOption [") << tick << _T("]");
	p_log->write(&strLog);
	
	fStatus = WinHttpSetOption(hRequestHandle, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);
	
	strLog << _T("End WinHttpSetOption [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);

	if (!fStatus)
	{
		strLog << ErrorHandler(L"WinHttpSetOption");
		p_log->write(&strLog, true);
		goto quit;
	}


	//
	// Perform websocket handshake by sending a request and receiving server's response.
	// Application may specify additional headers if needed.
	tick = GetTickCount();
	strLog << _T("Start WinHttpSendRequest handshake [") << tick << _T("]");
	p_log->write(&strLog);

	fStatus = WinHttpSendRequest(hRequestHandle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0);
	strLog << _T("End WinHttpSendRequest handshake [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);
	if (!fStatus)
	{
		dwError = GetLastError();

		strLog << ErrorHandler(L"WinHttpSendRequest handshake");
		p_log->write(&strLog, true);

		if (dwError == ERROR_WINHTTP_TIMEOUT) {
			strLog << L"WinHttpSendRequest handshake ERROR_WINHTTP_TIMEOUT " << ERROR_WINHTTP_TIMEOUT;
			p_log->write(&strLog);
		}

		goto quit;		
	}

	tick = GetTickCount();
	strLog << _T("Start WinHttpReceiveResponse handshake [") << tick << _T("]");
	p_log->write(&strLog);

	fStatus = WinHttpReceiveResponse(hRequestHandle, 0);

	strLog << _T("End WinHttpReceiveResponse handshake [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);
	if (!fStatus)
	{
		dwError = GetLastError();

		strLog << ErrorHandler(L"WinHttpReceiveResponse handshake");
		p_log->write(&strLog, true);

		if (dwError == ERROR_WINHTTP_TIMEOUT) {
			strLog << L"WinHttpReceiveResponse handshake ERROR_WINHTTP_TIMEOUT " << ERROR_WINHTTP_TIMEOUT;
			p_log->write(&strLog);
		}

		goto quit;
	}

	//
	// Application should check what is the HTTP status code returned by the server and behave accordingly.
	// WinHttpWebSocketCompleteUpgrade will fail if the HTTP status code is different than 101.
	//
	tick = GetTickCount();
	strLog << _T("Start WinHttpWebSocketCompleteUpgrade [") << tick << _T("]");
	p_log->write(&strLog);

	hWebSocketHandle = WinHttpWebSocketCompleteUpgrade(hRequestHandle, NULL);
	
	strLog << _T("End WinHttpWebSocketCompleteUpgrade [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);

	if (hWebSocketHandle == NULL)
	{
		strLog << ErrorHandler(L"WinHttpWebSocketCompleteUpgrade handshake");
		p_log->write(&strLog, true);
		goto quit;
	}

	//
	// The request handle is not needed anymore. From now on we will use the websocket handle.
	//
	WinHttpCloseHandle(hRequestHandle);
	hRequestHandle = NULL;

	strLog << appPrefix << _T(" Succesfully upgraded to websocket protocol\n")
		<< _T("Start Loop");
	
	p_log->write(&strLog, true);


	ZeroMemory(rgbBuffer, rgbBuffer[1024]);

	for (int i = 1; i <= 10000; i++)
	{

		::Sleep( (rand() % 100 * 10) );

		//
		// Send and receive data on the websocket protocol.
		//

		tick = GetTickCount();
		strLog << _T("Start WinHttpWebSocketSend [") << tick << _T("]");
		p_log->write(&strLog);

		strLog << _T("\tSent message[" << i << "] " << pcwszMessage);
		p_log->write(&strLog);
		dwError = WinHttpWebSocketSend(hWebSocketHandle,
			/*WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,*/ WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, 
			/*(PVOID)pcwszMessage,*/ (PVOID)appPrefixSTDstring.c_str(),
			/*cdwMessageLength);*/ appPrefixSTDstring.length());
		
		strLog << _T("End WinHttpWebSocketSend [") << tick << _T("] duration:" << GetTickCount() - tick);
		p_log->write(&strLog);

		if (dwError != ERROR_SUCCESS)
		{
			strLog << ErrorHandler(L"WinHttpWebSocketSend") << _T(" [") << tick << _T("]");
			p_log->write(&strLog, true);
			goto quit;
		}

		do
		{
			if (dwBufferLength == 0)
			{
				dwError = ERROR_NOT_ENOUGH_MEMORY;

				strLog << appPrefix << _T(" Receive ERROR_NOT_ENOUGH_MEMORY.");
				p_log->write(&strLog, true);

				goto quit;
			}

			tick = GetTickCount();
			strLog << _T("Start WinHttpWebSocketReceive [") << tick << _T("]");
			p_log->write(&strLog);

			dwError = WinHttpWebSocketReceive(hWebSocketHandle, pbCurrentBufferPointer, dwBufferLength, &dwBytesTransferred, &eBufferType);

			strLog << _T("End WinHttpWebSocketReceive [") << tick << _T("] duration:" << GetTickCount() - tick);
			p_log->write(&strLog);

			if (dwError != ERROR_SUCCESS)
			{
				if (dwError == ERROR_WINHTTP_OPERATION_CANCELLED) {
					strLog << _T("[ ERROR(ERROR_WINHTTP_OPERATION_CANCELLED) to Receive.] ");
				}

				strLog << ErrorHandler(L"WinHttpWebSocketReceive") << _T(" [") << tick << _T("]");
				p_log->write(&strLog, true);

				goto quit;
			}

			//
			// If we receive just part of the message restart the receive operation.
			//

			pbCurrentBufferPointer += dwBytesTransferred;
			dwBufferLength -= dwBytesTransferred;
		} while (eBufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE);

		//
		// We expected server just to echo single binary message.
		//

		if (eBufferType != WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
		{
			dwError = ERROR_INVALID_PARAMETER;
			
			strLog << ErrorHandler(L"Unexpected buffer type") << _T(" [") << tick << _T("]");
			p_log->write(&strLog, true);

			goto quit;
		}

		//wprintf(L"Received message from the server: '%.*s'\n", dwBufferLength, (WCHAR*)rgbBuffer);
		printf("[%u] Recebido bytes[%d] text['%.*s']\n", threadId, dwBytesTransferred, dwBytesTransferred, (char *)rgbBuffer);
		strLog << appPrefix << _T("\tRecebido bytes[") << dwBufferLength << _T("]  text[") << (char *)rgbBuffer << _T("]");
		p_log->write(&strLog);

		ZeroMemory(rgbBuffer, rgbBuffer[1024]);
		pbCurrentBufferPointer = rgbBuffer;
		dwBufferLength = ARRAYSIZE(rgbBuffer);
	}
	strLog << appPrefix << _T("FIM Loop");
	p_log->write(&strLog, true);

	//
	// Gracefully close the connection.
	//
	tick = GetTickCount();
	strLog << _T("Start WinHttpWebSocketClose [") << tick << _T("]");
	p_log->write(&strLog);

	dwError = WinHttpWebSocketClose(hWebSocketHandle, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
	
	strLog << _T("End WinHttpWebSocketClose [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);

	if (dwError != ERROR_SUCCESS)
	{
		strLog << ErrorHandler(L"WinHttpWebSocketClose");
		p_log->write(&strLog, true);
		goto quit;
	}

	//
	// Check close status returned by the server.
	//
	tick = GetTickCount();
	strLog << _T("Start WinHttpWebSocketQueryCloseStatus [") << tick << _T("]");
	p_log->write(&strLog);

	dwError = WinHttpWebSocketQueryCloseStatus(hWebSocketHandle, &usStatus, rgbCloseReasonBuffer, ARRAYSIZE(rgbCloseReasonBuffer), &dwCloseReasonLength);

	strLog << _T("End WinHttpWebSocketQueryCloseStatus [") << tick << _T("] duration:" << GetTickCount() - tick);
	p_log->write(&strLog);

	if (dwError != ERROR_SUCCESS)
	{
		strLog << ErrorHandler(L"WinHttpWebSocketQueryCloseStatus");
		p_log->write(&strLog, true);

		goto quit;
	}

	wprintf(L"The server closed the connection with status code: '%d' and reason: '%.*S'\n", (int)usStatus, dwCloseReasonLength, rgbCloseReasonBuffer);

	strLog << _T("The server closed the connection with status code: [") << usStatus << _T("] reason:" << rgbCloseReasonBuffer);
	p_log->write(&strLog);

quit:

	if (hRequestHandle != NULL)
	{
		WinHttpCloseHandle(hRequestHandle);
		hRequestHandle = NULL;
	}

	if (hWebSocketHandle != NULL)
	{
		WinHttpCloseHandle(hWebSocketHandle);
		hWebSocketHandle = NULL;
	}

	if (hConnectionHandle != NULL)
	{
		WinHttpCloseHandle(hConnectionHandle);
		hConnectionHandle = NULL;
	}

	if (hSessionHandle != NULL)
	{
		WinHttpCloseHandle(hSessionHandle);
		hSessionHandle = NULL;
	}

	if (dwError != ERROR_SUCCESS)
	{
		wprintf(L"Application failed with error: %u\n", dwError);
		ErrorHandler(L"Application failed ");
		return -1;
	}

	return 0;
}

int __cdecl wmain()
{
	
	CLog *p_log;
	OStringstream strLog;

	DWORD threadId = GetCurrentThreadId();

	TCHAR myPath[_MAX_PATH + 1];
	GetModuleFileName(NULL, myPath, _MAX_PATH);
	String appPath = myPath;
	appPath = appPath.substr(0, appPath.rfind(_T('\\'))) + _T("\\");
	String appPrefix(_T("websocket-client"));

	p_log = new CLog(appPath, appPrefix);

	strLog << _T("Path: ") << appPath << _T(" appPrefix:") << appPrefix;
	p_log->write(&strLog);


	PMYDATA pDataArray[MAX_THREADS];
	DWORD   dwThreadIdArray[MAX_THREADS];
	HANDLE  hThreadArray[MAX_THREADS];

	// Create MAX_THREADS worker threads.

	for (int i = 0; i < MAX_THREADS; i++)
	{
		// Allocate memory for thread data.
		pDataArray[i] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));

		if (pDataArray[i] == NULL)
		{
			// If the array allocation fails, the system is out of memory
			// so there is no point in trying to print an error message.
			// Just terminate execution.
			ExitProcess(2);
		}

		// Generate unique data for each thread to work with.

		pDataArray[i]->idArray = i;
		bool useIndividualLogPerClient = true;
		if (useIndividualLogPerClient) {
			pDataArray[i]->p_log = p_log;
		}
		else {
			pDataArray[i]->p_log = nullptr;
		}

		// Create the thread to begin execution on its own.

		hThreadArray[i] = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			threadWebSocket,       // thread function name
			pDataArray[i],          // argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[i]);   // returns the thread identifier 


		// Check the return value for success.
		// If CreateThread fails, terminate execution. 
		// This will automatically clean up threads and memory. 

		if (hThreadArray[i] == NULL)
		{
			ErrorHandler(TEXT("CreateThread"));
			ExitProcess(3);
		}

		// espera um pouco para nao gerar erro no inicio. 0 a 300 ms
		::Sleep( (rand() % 4 ) * 100 );
	} // End of main thread creation loop.

	// Wait until all threads have terminated.

	WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);

	// Close all thread handles and free memory allocations.

	for (int i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (pDataArray[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pDataArray[i]);
			pDataArray[i] = NULL;    // Ensure address is not reused.
		}
	}

	return 0;
}