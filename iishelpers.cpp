#include "precomp.h"
#include "iishelpers.h"


IISHelpers::IISHelpers()
{
	urlmap["/websocket"] = EHTTPSamples::WEB_SOCKET;
	urlmap["/get_test"] = EHTTPSamples::WEB_REQUEST_GET;
	urlmap["/post_test"] = EHTTPSamples::WEB_REQUEST_POST_DATA;
	urlmap["/post_file"] = EHTTPSamples::WEB_REQUEST_POST_FILE;
	urlmap["/post_async"] = EHTTPSamples::WEB_REQUEST_ASYNC_THREAD;
}

IISHelpers::~IISHelpers()
{
	urlmap.clear();
}

BOOL IISHelpers::RegisterIIHelpersModule()
{
	GetSystemInfo(&systemInfo);
	dwPageSizeSystem = systemInfo.dwPageSize;
	
	return TRUE;
}

std::string IISHelpers::GetIpAddr(PSOCKADDR pAddr)
{
	SOCKADDR_IN  addr;	
	
	char   AddrString[256] = "Some dummy value";
	DWORD   dwSizeOfStr = sizeof(AddrString);

	if (WSAAddressToStringA((LPSOCKADDR)&addr, sizeof(addr), NULL, AddrString, &dwSizeOfStr) != 0)
	{
		printf("WSAAddressToStringA() failed miserably with error code %ld\n", WSAGetLastError());
	}
	else
		printf("Address string = %s\n", AddrString);

	return std::string(AddrString);
}

SOCKADDR_IN IISHelpers::ToIpAddr(std::string textIp)
{
	SOCKADDR_IN  addr;
	char   AddrValue[256] = "216.239.61.104";
	int    nSizeOfInput = sizeof(AddrValue);

	if (WSAStringToAddressA(AddrValue, AF_INET, NULL, (LPSOCKADDR)&addr, &nSizeOfInput) != 0)
	{
		printf("\nWSAStringToAddressA failed with error num %ld\n", WSAGetLastError());
	}
	else
		printf("\nAddress in value = %ul\n", addr.sin_addr);

	return addr;
}

std::string IISHelpers::ConvertUTF16ToUTF8(const WCHAR * pszTextUTF16, size_t cchUTF16)
{
	//
	// Special case of NULL or empty input string
	//
	if ((pszTextUTF16 == NULL) || (*pszTextUTF16 == L'\0') || cchUTF16 == 0)
	{
		// Return empty string
		return "";
	}

	//
	// Get size of destination UTF-8 buffer, in CHAR's (= bytes)
	//
	int cbUTF8 = ::WideCharToMultiByte(
		CP_UTF8,					// convert to UTF-8
		0,							// specify conversion behavior
		pszTextUTF16,				// source UTF-16 string
		static_cast<int>(cchUTF16), // total source string length, in WCHAR's, 
		NULL,						// unused - no conversion required in this step
		0,							// request buffer size
		NULL, NULL					// unused
	);

	if (cbUTF8 == 0)
	{
		return "";
	}

	//
	// Allocate destination buffer for UTF-8 string
	//
	int cchUTF8 = cbUTF8; // sizeof(CHAR) = 1 byte
	char *pszUTF8 = (char *)malloc(cchUTF8 + 1);

	//
	// Do the conversion from UTF-16 to UTF-8
	//
	int result = ::WideCharToMultiByte(
		CP_UTF8,                // convert to UTF-8
		0,      // specify conversion behavior
		pszTextUTF16,           // source UTF-16 string
		static_cast<int>(cchUTF16),   // total source string length, in WCHAR's, 
										// including end-of-string \0
		pszUTF8,                // destination buffer
		cbUTF8,                 // destination buffer size, in bytes
		NULL, NULL              // unused
	);

	if (result == 0)
	{
		return "";
	}

	pszUTF8[cchUTF8] = 0;

	std::string ret(pszUTF8);

	if (pszUTF8)
		free(pszUTF8);

	return ret;
}

HRESULT IISHelpers::WriteResponse(IHttpContext* pHttpContext, const std::string &content)
{
	HRESULT hr = S_OK;

	IHttpResponse* pHttpResponse = pHttpContext->GetResponse();
	if (pHttpResponse != NULL)
	{							
		//we can manipulate the original data and change the content. We must get IIS-managed memory chunk
		void *pBuffer = pHttpContext->AllocateRequestMemory(content.length());

		HTTP_DATA_CHUNK dataChunk;
		dataChunk.DataChunkType = HttpDataChunkFromMemory;
		
		memcpy(pBuffer, content.data(), content.length());

		dataChunk.FromMemory.pBuffer = (PVOID)pBuffer;
		dataChunk.FromMemory.BufferLength = (ULONG)content.length();

		hr = pHttpResponse->WriteEntityChunkByReference(&dataChunk, -1);
		return hr;
	}
	else
	{
		hr = E_POINTER;
	}	

	return hr;
}

bool IISHelpers::GetVariable(IHttpContext * pHttpContext, PCSTR varName, PCSTR * pVarVal, DWORD * pVarValSize, BOOL isRequired)
{
	const char* thisfunc = "GetVariable()";
	std::string strLog;
	bool status = true;
	DWORD VarValSize = 0;

	if (pVarValSize == NULL) {
		pVarValSize = &VarValSize;
	}
	if (S_OK == (pHttpContext->GetServerVariable(varName, pVarVal, pVarValSize)))
	{
		*pVarVal = (PCSTR)pHttpContext->AllocateRequestMemory((*pVarValSize) + 1);
		if (*pVarVal == NULL) {
			strLog = "Could not allocate memory";
			status = false;
		}
		else {
			if (S_OK != (pHttpContext->GetServerVariable(varName, pVarVal, pVarValSize)))
			{
				strLog = std::string(varName) + "is not a valid server variable.";
				status = false;
			}
			else {
				if (*pVarVal != NULL && strlen(*pVarVal) > 0) {
					strLog = std::string(varName) + " = " + std::string(*pVarVal);
				}
				else {
					strLog = std::string(varName);
					if (*pVarVal != NULL && strlen(*pVarVal) == 0) {
						*pVarVal = NULL;
					}
				}
			}
		}
	}
	else {
		if (isRequired) {
			strLog = std::string(varName) + ": Server variable %s is not found in HttpContext.";			
			status = false;
		}
		else {
			strLog = std::string(varName) + ": Server variable %s is not found in HttpContext.";
		}
	}
	return status;
}

/*
* Retrieves entity data from the request.
* */
bool IISHelpers::GetEntity(IHttpContext * pHttpContext, std::string & data)
{
	bool status = true;
	HRESULT hr;
	IHttpRequest* pHttpRequest = pHttpContext->GetRequest();
	DWORD cbBytesReceived = pHttpRequest->GetRemainingEntityBytes();
	int cb = (int)cbBytesReceived;
	void * pvRequestBody = pHttpContext->AllocateRequestMemory(cbBytesReceived);
	void * entityBody;
	data.clear();

	if (cbBytesReceived > 0)
	{
		while (pHttpRequest->GetRemainingEntityBytes() != 0)
		{
			hr = pHttpRequest->ReadEntityBody(pvRequestBody, cbBytesReceived, false,
				&cbBytesReceived, NULL);
			if (FAILED(hr)) {
				return false;
			}
			data.append((char*)pvRequestBody, (int)cbBytesReceived);
		}
	}

	//set it back in the request
	entityBody = pHttpContext->AllocateRequestMemory(data.length());
	strcpy((char*)entityBody, data.c_str());
	pHttpRequest->InsertEntityBody(entityBody, strlen((char*)entityBody));

	return status;

	/* https://forums.iis.net/t/1196227.aspx?C+module+ReadEntityBody+0+bytes+error+0x80070006
	DWORD SZMAX_READ = httpRequest->BytesReceived;

	char* pvRequestBody = (char *)pHttpContext->AllocateRequestMemory(SZMAX_READ);

	DWORD cbBytesReceived = SZMAX_READ;
	DWORD cbTotBytes = 0;
	DWORD cbTestExtra = pHttpRequest->GetRemainingEntityBytes();
	while (hr == S_OK && cbTestExtra > 0) {
		if (cbTotBytes + cbTestExtra > SZMAX_READ)
			hr = STG_E_INSUFFICIENTMEMORY;
		else {
			hr = pHttpRequest->ReadEntityBody(pvRequestBody + cbTotBytes, SZMAX_READ, false, &cbBytesReceived, NULL);
			cbTotBytes += cbBytesReceived;
			cbTestExtra = pHttpRequest->GetRemainingEntityBytes();
		}
	}
	*/
}

std::string  ErrorHandler(LPTSTR lpszFunction)
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
	//wprintf(L"\n[%u] Error - Message: [%s]\n", GetCurrentThreadId(), (LPCTSTR)lpDisplayBuf);
	std::string returnString = std::to_string(GetCurrentThreadId()) + " " + (LPCTSTR)lpDisplayBuf;

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	return returnString;
}


