#include "precomp.h"
#include "mywebsocket.h"

CMyHttpModule::CMyHttpModule()
{
	OutputDebugString(__FUNCTION__ " \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);
}

CMyHttpModule::~CMyHttpModule()
{
	OutputDebugString(__FUNCTION__ " \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);
}


REQUEST_NOTIFICATION_STATUS ExecuteRequest(CLog* p_log, IHttpContext * pHttpContext)
{
	std::ostringstream strLog;
	strLog << "thread start " << std::this_thread::get_id();

	p_log->write(&strLog);
	std::string data;
	IISHelpers::GetEntity(pHttpContext, data);

	PCSTR rawBuffer = NULL;
	DWORD rawLength = 0;
	IISHelpers::GetVariable(pHttpContext, "ALL_RAW", &rawBuffer, &rawLength, true);

	for (int i = 10; i > 0; --i) {
		strLog << "waiting: " << i << std::endl;
		p_log->write(&strLog);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	IHttpResponse *pHttpResponse = pHttpContext->GetResponse();
	pHttpResponse->SetHeader("Content-Type", "text/plain", strlen("text/plain"), true);

	HTTP_DATA_CHUNK dataChunk1;

	dataChunk1.DataChunkType = HttpDataChunkFromMemory;
	dataChunk1.FromMemory.pBuffer = (PVOID)rawBuffer;
	dataChunk1.FromMemory.BufferLength = (ULONG)rawLength;

	HRESULT hr = pHttpResponse->WriteEntityChunkByReference(&dataChunk1, -1);

	if (FAILED(hr))
	{
		p_log->write("thread end with Error. Return RQ_NOTIFICATION_CONTINUE");
		return RQ_NOTIFICATION_CONTINUE;
	}
	

	strLog << "thread end " << std::this_thread::get_id() << " Return RQ_NOTIFICATION_FINISH_REQUEST";
	pHttpContext->IndicateCompletion(RQ_NOTIFICATION_FINISH_REQUEST);
	return RQ_NOTIFICATION_FINISH_REQUEST;

}



VOID WINAPI WEBSOCKET_COMPLETION(
	HRESULT     hrError,
	VOID *      pvCompletionContext,
	DWORD       cbIO,
	BOOL        fUTF8Encoded,
	BOOL        fFinalFragment,
	BOOL        fClose
) 
{
	if (FAILED(hrError))
	{

	}

	void * readBuffer = pvCompletionContext;
	DWORD readLength = 1024;
	OutputDebugString( (char *)readBuffer);
	

};

REQUEST_NOTIFICATION_STATUS
CMyHttpModule::OnBeginRequest(
	IN IHttpContext * pHttpContext, 
	IN OUT IHttpEventProvider * pProvider
)
try 
{
	//UNREFERENCED_PARAMETER(pHttpContext);
	//UNREFERENCED_PARAMETER(pProvider);
	//DebugBreak();
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	g_requestsCount++;

	HRESULT hr = S_OK;
	IHttpRequest* pHttpRequest = pHttpContext->GetRequest();
	IHttpResponse* pHttpResponse = pHttpContext->GetResponse();

	std::map<std::string, int> urlmap;
	urlmap["/websocket"] = EHTTPSamples::WEB_SOCKET;
	urlmap["/get_test"] = EHTTPSamples::WEB_REQUEST_GET;
	urlmap["/post_test"] = EHTTPSamples::WEB_REQUEST_POST_DATA;
	urlmap["/post_file"] = EHTTPSamples::WEB_REQUEST_POST_FILE;
	urlmap["/post_async"] = EHTTPSamples::WEB_REQUEST_ASYNC_THREAD;

	HTTP_REQUEST * httpRequest = pHttpRequest->GetRawHttpRequest();		
	std::string v_forwardURL = httpRequest->pRawUrl;	
	//DWORD v_pcchValueLength;
	//IISHelpers::GetVariable(pHttpContext, "HTTP_URL", &v_forwardURL, &v_pcchValueLength, true);
		
	strLog << __FUNCTION__ << "Request: "<< v_forwardURL << std::endl;
	p_log->write(&strLog);

	// sample:  /iis-module/socket
	size_t found = v_forwardURL.find_last_of('/');
	if (found > 0) {
		v_forwardURL = v_forwardURL.substr(found);
	}

	strLog << __FUNCTION__ << "Command: " << v_forwardURL << std::endl;
	p_log->write(&strLog);

	if (urlmap.find(v_forwardURL) != urlmap.end())
	{
		switch (urlmap[v_forwardURL])
		{
			case WEB_SOCKET:
			{
				// https://docs.microsoft.com/en-us/iis/configuration/system.webserver/websocket
				//https://www.joshwieder.net/2012/09/websockets-and-iis8-enable-websocket.html
				//https://www.codeproject.com/Articles/716148/Using-Sec-Websocket-Protocol

				// DebugBreak();

#pragma region WEBSOCKET Handshake
				PCSTR rawBuffer = NULL;
				DWORD rawLength = 0;
				IISHelpers::GetVariable(pHttpContext, "HEADER_Sec-WebSocket-Key", &rawBuffer, &rawLength, true);
				//auto hhc = pHttpRequest->GetHeader(HTTP_HEADER_ID::HttpHeaderUpgrade);

				char output[29] = {};
				WebSocketHandshake::generate(rawBuffer, output);

				pHttpResponse->Clear();
				pHttpResponse->SetHeader("Upgrade", "websocket", strlen("websocket"), true);
				pHttpResponse->SetHeader("Connection", "Upgrade", strlen("Upgrade"), true);
				pHttpResponse->SetHeader("Sec-WebSocket-Accept", output, strlen(output), true);
				//pHttpResponse->SetHeader("Sec-WebSocket-Protocol", "xwc", strlen("xwc"), true);

				pHttpResponse->SetStatus(101, "Web Socket Protocol Handshake", 0, hr);
				if (FAILED(hr))
				{
					strLog << __FUNCTION__ << "Error to SetStatus 101 in Upgrade" << hr;
					p_log->write(&strLog);
					goto Finished;
				}

				// Buffer to store the byte count.
				DWORD cbSent = 0;
				// Buffer to store if asyncronous completion is pending.
				BOOL fCompletionExpected = false;
				hr = pHttpResponse->Flush(false, true, &cbSent, &fCompletionExpected);
#pragma endregion

				IHttpContext3* pHttpContext3;
				hr = HttpGetExtendedInterface(g_pHttpServer, pHttpContext, &pHttpContext3);
				if (FAILED(hr))
				{
					strLog << __FUNCTION__ << "Error to HttpGetExtendedInterface" << hr;
					p_log->write(&strLog);
					goto Finished;
				}

				//
				// Get Pointer to IWebSocketContext
				//
				IWebSocketContext * pWebSocketContext;
				pWebSocketContext = (IWebSocketContext *)pHttpContext3->GetNamedContextContainer()->GetNamedContext(IIS_WEBSOCKET);
				if (pWebSocketContext == NULL)
				{
					hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
					strLog << __FUNCTION__ << "Error to GetContext websockets" << hr;
					p_log->write(&strLog);

					goto Finished;
				}

				USHORT statusSocket = 0;
				LPCWSTR statusBuffer = NULL;
				USHORT statusLength = 0;
				
				strLog << __FUNCTION__ << "WEBSOCKET ESTABLISHED " << ++g_websocketsCount;
				p_log->write(&strLog);

				MyWebSocket websocket(p_log, g_pHttpServer, pHttpContext, pWebSocketContext);
				websocket.Reading();
								
				strLog << __FUNCTION__ << "WEBSOCKET RELEASED " << --g_websocketsCount;
				p_log->write(&strLog);

				//ULONG count = 1;
				//while (true)
				//{
				//	// https://www.iana.org/assignments/websocket/websocket.xml#close-code-number-rules
				//	//https://tools.ietf.org/html/rfc6455
				//	hr = pWebSocketContext->GetCloseStatus(&statusSocket, &statusBuffer, &statusLength);
				//	if (hr == NULL || statusSocket == -1) 
				//	{
				//		strLog << __FUNCTION__ << "websockets closed, Status:" << statusBuffer << " ret:" << hr << hr;
				//		p_log->write(&strLog);

				//		break;
				//	}

				//	void * readBuffer = pHttpContext->AllocateRequestMemory(1024);
				//	DWORD readLength = 1024;					
				//	BOOL bUTF8Encoded = FALSE;
				//	BOOL bFinalFragment = TRUE;
				//	BOOL bConnectionClose = FALSE;
				//	BOOL success = FALSE;
				//	hr = pWebSocketContext->ReadFragment(readBuffer, &readLength, TRUE, &bUTF8Encoded, &bFinalFragment, &bConnectionClose, &WEBSOCKET_COMPLETION, readBuffer, &success);
				//	if (FAILED(hr)) {

				//	}
				//	if (success == TRUE)
				//	{
				//		strLog << __FUNCTION__ << "ready socket: lenght:" << readLength << " text:" << (char *)readBuffer;
				//		p_log->write(&strLog);
				//	}

				//	void * writeBuffer = pHttpContext->AllocateRequestMemory(1024);
				//	std::string text("number " + std::to_string(count++));
				//	strcpy((char*)writeBuffer, text.c_str());
				//	DWORD writeLength = strlen((char*)writeBuffer);

				//	hr = pWebSocketContext->WriteFragment(writeBuffer, &writeLength, TRUE, FALSE, TRUE, &WEBSOCKET_COMPLETION, NULL, NULL);
				//	if (FAILED(hr)) {

				//	}

				//	::Sleep(2000);
				//}

				break;
			}
			case WEB_REQUEST_GET:
			{
				


				break;
			}
			case WEB_REQUEST_POST_FILE:
			{
				std::string data;
				IISHelpers::GetEntity(pHttpContext, data);

				PCSTR rawBuffer = NULL;
				DWORD rawLength = 0;
				IISHelpers::GetVariable(pHttpContext, "ALL_RAW", &rawBuffer, &rawLength, true);

				std::ofstream ofile("C:\\applogs\\proxy-webhook\\teste.jpg", std::ios::binary);
				ofile.write(data.c_str(), data.length());
				ofile.close();

				IHttpResponse *pHttpResponse = pHttpContext->GetResponse();

				HTTP_DATA_CHUNK dataChunk1;

				dataChunk1.DataChunkType = HttpDataChunkFromMemory;
				dataChunk1.FromMemory.pBuffer = (PVOID)rawBuffer;
				dataChunk1.FromMemory.BufferLength = (ULONG)rawLength;

				// Comentado para nao retornar daqui.
				pHttpResponse->SetHeader("Content-Type", "text/html", strlen("text/html"), true);
				hr = pHttpResponse->WriteEntityChunkByReference(&dataChunk1, -1);

				if (FAILED(hr))
				{
					pProvider->SetErrorStatus(hr);
					return RQ_NOTIFICATION_CONTINUE;
				}
				std::string *teste = new std::string();
				teste->assign("Rafael Cesar da silva Souza\n\rSouza\r\n");
				IISHelpers::WriteResponse(pHttpContext, *teste);
				teste->assign("        ");
				delete teste;
				teste = nullptr;

				std::string teste2 = "Rafael Cesar da silva Souza\n\rSouza\r\n";
				IISHelpers::WriteResponse(pHttpContext, teste2);
				teste2.assign("");


				return RQ_NOTIFICATION_FINISH_REQUEST;

				break;
			}
			case WEB_REQUEST_ASYNC_THREAD:
			{

				//Test Async Request with thread

				////std::thread t(ExecuteRequest, p_log, pHttpContext);
				//// std::thread{ ExecuteRequest,  p_log, pHttpContext }.detach();
				std::thread banana(ExecuteRequest, p_log, pHttpContext);
				banana.detach();
				p_log->write("OnBeginRequest ->thread. Return RQ_NOTIFICATION_PENDING");
				return RQ_NOTIFICATION_PENDING;

				break;
			}
		}
	}
	else
	{
		goto Finished;
	}
		

	//This will hold the HttpContext for the child request
	IHttpApplication2 * pApplication = NULL;
	hr = S_OK;

	//Now we will get the request's "WARMUP_REQUEST" server variable
	DWORD valueLength = 200;
	PCWSTR warmupRequest[200];
	*warmupRequest = NULL;

	hr = pHttpContext->GetServerVariable("WARMUP_REQUEST",
		warmupRequest,
		&valueLength);

	// "WARMUP_REQUEST" server variable should be set for warming up requests.

	// For non-warming up requests, Error_Invalid_Index error should be returned.
	if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_INDEX))
	{
		//this is not a warmup request
		//just a normal request
		OutputDebugString(__FUNCTION__ " " " this is normal request\n");
	}
	else if (FAILED(hr))
	{
		goto Finished;
	}
	else
	{
		//this is a warmup request
		OutputDebugString(__FUNCTION__ " " " this is a warmup request\n");
	}

	//Now we will use the pApplication to get an IHttpApplication2 object
	hr = HttpGetExtendedInterface(g_pHttpServer,
		pHttpContext->GetApplication(),
		&pApplication);

	if (FAILED(hr))
	{
		goto Finished;
	}

	/*
	It is easy to change the application state based on the requested URL or any other local server variable.
	This can really be helpful if you know that a certain page in the application is slow and want to get back
	to the user while the application works in the background
	*/

	// Calling BeginApplicationWarmup() should put the current application in a Warmup state
	pApplication->BeginApplicationWarmup();
	if (!pApplication->QueryIsWarmingUp())
	{
		hr = S_FALSE;
		goto Finished;
	}

	// Similarly calling EndApplicationWarmup() should end that warmup state the application was in
	pApplication->EndApplicationWarmup();
	if (pApplication->QueryIsWarmingUp())
	{
		hr = S_FALSE;
		goto Finished;
	}

Finished:

	//if we faced an error, we will set the return status to 500 to let the user know that there was a server error
	if (FAILED(hr))
	{
		pHttpContext->GetResponse()->SetStatus(500,
			"Server Error",
			0,
			hr);
		return RQ_NOTIFICATION_FINISH_REQUEST;
	}

	return RQ_NOTIFICATION_CONTINUE;
}
catch (std::exception &e) {
	HRESULT hr = S_OK;
	e.what();

	pHttpContext->GetResponse()->SetStatus(500, "Server Error", 0, hr);

	return RQ_NOTIFICATION_CONTINUE;
};

//  Implementation of the OnAcquireRequestState method
REQUEST_NOTIFICATION_STATUS
CMyHttpModule::OnAcquireRequestState(
    IN IHttpContext *                       pHttpContext,
    IN OUT IHttpEventProvider *             pProvider
)
{
    HRESULT                         hr = S_OK;

	// TODO: implement the AcquireRequestState module functionality
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

Finished:

    if ( FAILED( hr )  )
    {
        return RQ_NOTIFICATION_FINISH_REQUEST;
    }
    else
    {
        return RQ_NOTIFICATION_CONTINUE;
    }
}

// TODO: implement other desired event handler methods below

REQUEST_NOTIFICATION_STATUS 
CMyHttpModule::OnPreExecuteRequestHandler(
	IN IHttpContext * pHttpContext,
	IN IHttpEventProvider * pProvider
)
{

	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	//DebugBreak();

	return RQ_NOTIFICATION_CONTINUE;
}


REQUEST_NOTIFICATION_STATUS
CMyHttpModule::OnPostExecuteRequestHandler(
	IN IHttpContext *             pHttpContext,
	IN IHttpEventProvider *       pProvider
)
{
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);

	return RQ_NOTIFICATION_CONTINUE;
}