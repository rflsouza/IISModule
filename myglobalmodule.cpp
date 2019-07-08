#include "precomp.h"
#include "myglobalmodule.h"


MyGlobalModule::MyGlobalModule()
{
	OutputDebugString(__FUNCTION__ " \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	SYSTEM_INFO         sysInfo;
	GetSystemInfo(&sysInfo);
	m_dwPageSize = sysInfo.dwPageSize;
}


MyGlobalModule::~MyGlobalModule()
{
	OutputDebugString(__FUNCTION__ " \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);
}


GLOBAL_NOTIFICATION_STATUS
MyGlobalModule::OnGlobalConfigurationChange(
	IN IGlobalConfigurationChangeProvider * pProvider
)
{
	UNREFERENCED_PARAMETER(pProvider);
	OutputDebugString(__FUNCTION__ " \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;

	return GL_NOTIFICATION_CONTINUE;
}

GLOBAL_NOTIFICATION_STATUS
MyGlobalModule::OnGlobalFileChange(
	IN IGlobalFileChangeProvider * pProvider
)
{
	UNREFERENCED_PARAMETER(pProvider);
	OutputDebugString(__FUNCTION__ " \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;

	return GL_NOTIFICATION_CONTINUE;
}


GLOBAL_NOTIFICATION_STATUS MyGlobalModule::OnGlobalPreBeginRequest(IN IPreBeginRequestProvider * pProvider)
{
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");

	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	HRESULT hr = S_OK;
	IHttpContext* pHttpContext = pProvider->GetHttpContext();
	IHttpResponse* pHttpResponse = pHttpContext->GetResponse();



	IHttpRequest * pHttpRequest = pHttpContext->GetRequest();
	HTTP_REQUEST * httpRequest = pHttpRequest->GetRawHttpRequest();	
	PCSTR url = httpRequest->pRawUrl;
	HTTP_URL_CONTEXT  urlContext = httpRequest->UrlContext;	

	std::string name, value;
	for (int i = 0; i < httpRequest->Headers.UnknownHeaderCount; i++)
	{
		name.assign(httpRequest->Headers.pUnknownHeaders[i].pName, httpRequest->Headers.pUnknownHeaders[i].NameLength);
		value.assign(httpRequest->Headers.pUnknownHeaders[i].pRawValue, httpRequest->Headers.pUnknownHeaders[i].RawValueLength);
	}

	for (int i = 0; i < HttpHeaderRequestMaximum; i++)
	{
		value.assign(httpRequest->Headers.KnownHeaders[i].pRawValue, httpRequest->Headers.KnownHeaders[i].RawValueLength);
	}
	

	/* 33333333333333
	HTTP_DATA_CHUNK *pSourceDataChunk = NULL;
	HTTP_BYTE_RANGE *pFileByteRange = NULL;
	LARGE_INTEGER  lFileSize;
	ULONGLONG ulTotalLength = 0;

	for (int c = 0; c < httpRequest->EntityChunkCount; c++)
	{
		pSourceDataChunk = &httpRequest->pEntityChunks[c];

		switch (pSourceDataChunk->DataChunkType)
		{
		case HttpDataChunkFromMemory:
			ulTotalLength += pSourceDataChunk->FromMemory.BufferLength;
			break;
		case HttpDataChunkFromFileHandle:
			pFileByteRange = &pSourceDataChunk->FromFileHandle.ByteRange;
			//
			// File chunks may contain by ranges with unspecified length 
			// (HTTP_BYTE_RANGE_TO_EOF).  In order to send parts of such a chunk, 
			// its necessary to know when the chunk is finished, and
			// we need to move to the next chunk.
			//              
			if (pFileByteRange->Length.QuadPart == HTTP_BYTE_RANGE_TO_EOF)
			{
				if (GetFileType(pSourceDataChunk->FromFileHandle.FileHandle) ==
					FILE_TYPE_DISK)
				{
					if (!GetFileSizeEx(pSourceDataChunk->FromFileHandle.FileHandle,
						&lFileSize))
					{
						DWORD dwError = GetLastError();
						hr = HRESULT_FROM_WIN32(dwError);
						//goto Finished;
					}

					// put the resolved file length in the chunk, replacing 
					// HTTP_BYTE_RANGE_TO_EOF
					pFileByteRange->Length.QuadPart =
						lFileSize.QuadPart - pFileByteRange->StartingOffset.QuadPart;
				}
				else
				{
					hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
					//goto Finished;
				}
			}

			ulTotalLength += pFileByteRange->Length.QuadPart;
			break;
		default:
			// TBD: consider implementing HttpDataChunkFromFragmentCache, 
			// and HttpDataChunkFromFragmentCacheEx
			hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			//goto Finished;
		}
	}

	char* m_pResponseBuffer = (char *)pHttpContext->AllocateRequestMemory(ulTotalLength);
	DWORD m_pResponseLength = 0;

	ulTotalLength = 0;

	for (int c = 0; c < httpRequest->EntityChunkCount; c++)
	{
		pSourceDataChunk = &httpRequest->pEntityChunks[c];

		switch (pSourceDataChunk->DataChunkType)
		{
		case HttpDataChunkFromMemory:
			memcpy(m_pResponseBuffer + ulTotalLength, pSourceDataChunk->FromMemory.pBuffer, pSourceDataChunk->FromMemory.BufferLength);
			ulTotalLength += pSourceDataChunk->FromMemory.BufferLength;
			break;
		case HttpDataChunkFromFileHandle:
			pFileByteRange = &pSourceDataChunk->FromFileHandle.ByteRange;

			if (ReadFileChunk(pSourceDataChunk, m_pResponseBuffer + ulTotalLength) != S_OK)
			{
				DWORD dwErr = GetLastError();

				hr = HRESULT_FROM_WIN32(dwErr);
				// goto Finished;
			}

			ulTotalLength += pFileByteRange->Length.QuadPart;
			break;
		default:
			// TBD: consider implementing HttpDataChunkFromFragmentCache, 
			// and HttpDataChunkFromFragmentCacheEx
			hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			//goto Finished;
		}
	}	
	m_pResponseLength = ulTotalLength;

	std::string body;
	body.assign(const_cast<const char*>(m_pResponseBuffer), m_pResponseLength);

	std::ofstream ofile("C:\\applogs\\proxy-webhook\\teste.jpg", std::ios::binary);
	ofile.write(m_pResponseBuffer, m_pResponseLength);
	ofile.close();

	const float f = 3.14f;
	std::ofstream ofile2("C:\\applogs\\proxy-webhook\\foobar.bin", std::ios::binary);
	ofile2.write((char*)&f, sizeof(float));

	body = m_pResponseBuffer;

	*/

	//////////////////////////////

	//if (pHttpResponse != NULL)
	//{
	//	HTTP_DATA_CHUNK dataChunk1;
	//	HTTP_DATA_CHUNK dataChunk2;

	//	pHttpResponse->Clear();

	//	int BUFFERLENGTH = 256;

	//	//char szBuffer[BUFFERLENGTH];
	//	//char szBuffer2[BUFFERLENGTH];   

	//	char* szBuffer = (char *)pHttpContext->AllocateRequestMemory(BUFFERLENGTH);
	//	char* szBuffer2 = (char *)pHttpContext->AllocateRequestMemory(BUFFERLENGTH);

	//	dataChunk1.DataChunkType = HttpDataChunkFromMemory;
	//	strcpy_s(szBuffer, 255, "Hello world!!!\r\n");

	//	dataChunk1.FromMemory.pBuffer = (PVOID)szBuffer;
	//	dataChunk1.FromMemory.BufferLength = (ULONG)strlen(szBuffer);
	//	hr = pHttpResponse->WriteEntityChunkByReference(&dataChunk1, -1);

	//	if (FAILED(hr))
	//	{
	//		pProvider->SetErrorStatus(hr);
	//		return GL_NOTIFICATION_HANDLED;
	//	}

	//	dataChunk2.DataChunkType = HttpDataChunkFromMemory;
	//	wchar_t wstrTest1[] = L"rafael Souza";
	//	int encodedStrLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstrTest1, -1, szBuffer2, BUFFERLENGTH, NULL, NULL);

	//	dataChunk2.FromMemory.pBuffer = (PVOID)szBuffer2;
	//	dataChunk2.FromMemory.BufferLength = encodedStrLen;
	//	hr = pHttpResponse->WriteEntityChunkByReference(&dataChunk2, -1);

	//	if (FAILED(hr))
	//	{
	//		pProvider->SetErrorStatus(hr);
	//		return GL_NOTIFICATION_HANDLED;
	//	}
	//	// Comentado para nao retornar daqui.
	//	//pHttpResponse->SetHeader("Content-Type", "text/json", 9, true);
	//	//return GL_NOTIFICATION_HANDLED;
	//}
	return GL_NOTIFICATION_CONTINUE;
}

GLOBAL_NOTIFICATION_STATUS
MyGlobalModule::OnGlobalApplicationStart(
	IN IHttpApplicationStartProvider * pProvider
)
{
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	UNREFERENCED_PARAMETER(pProvider);
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");

	return GL_NOTIFICATION_CONTINUE;
}

GLOBAL_NOTIFICATION_STATUS
MyGlobalModule::OnGlobalApplicationStop(
	IN IHttpApplicationStopProvider * pProvider
)
{
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	UNREFERENCED_PARAMETER(pProvider);
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");

	return GL_NOTIFICATION_CONTINUE;
}

GLOBAL_NOTIFICATION_STATUS 
MyGlobalModule::OnGlobalApplicationPreload(
	IN IGlobalApplicationPreloadProvider * pProvider
)
{
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");
	/*
	This function is called every time the application’s worker process loads. In here, for example we can check if this start is due to a process recycle or not.
	The worker process can either be starting for the first time or the result of a recycle, either way this function will be called.
	*/
	//Lets check if this was the result of process recycle

	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	IHttpContext3 * pHttpContext3 = NULL;
	HRESULT hr = S_OK;
	IGlobalApplicationPreloadProvider2 * g_pPreloadProvider = NULL;


	IHttpContext *pHttpContext = NULL;
	hr = pProvider->CreateContext(&pHttpContext);

	if (FAILED(hr))
	{
		goto Finished;
	}
	// now we will get the new IGlobalApplicationPreloadProvider2
	// we will need the global Server Info we saved in the RegisterModule function

	hr = HttpGetExtendedInterface( g_pHttpServer,
			pProvider,
			&g_pPreloadProvider);

	if (FAILED(hr))
	{
		goto Finished;
	}


	//Now we can use the IGlobalApplicationPreloadProvider to check if the process was the result of a recycle
	if (g_pPreloadProvider->IsProcessRecycled())
	{
		//TODO: something specific to a recycled process
		OutputDebugString(__FUNCTION__ " app pool recycled process \n");
	}
	else
	{
		//TODO: something generic to an app pool process starting
		OutputDebugString(__FUNCTION__ " app pool process starting \n");
	}

Finished:

	if (pHttpContext != NULL)
	{
		pHttpContext->ReleaseClonedContext();
		pHttpContext = NULL;
	}

	return GL_NOTIFICATION_CONTINUE;
}

