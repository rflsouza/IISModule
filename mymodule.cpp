#include "precomp.h"

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

REQUEST_NOTIFICATION_STATUS
CMyHttpModule::OnBeginRequest(
	IN IHttpContext * pHttpContext, 
	IN OUT IHttpEventProvider * pProvider
)
{
	//UNREFERENCED_PARAMETER(pHttpContext);
	//UNREFERENCED_PARAMETER(pProvider);
	//DebugBreak();
	OutputDebugString(__FUNCTION__ " " " This module subscribed to event \n");
	std::ostringstream strLog;
	strLog << __FUNCTION__;
	p_log->write(&strLog);
	
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
	pHttpResponse->SetHeader("Content-Type", "text/plain", strlen("text/plain"), true);
	HRESULT hr = pHttpResponse->WriteEntityChunkByReference(&dataChunk1, -1);

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



	//This will hold the HttpContext for the child request
	IHttpContext3 * pHttpContext3 = NULL;
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