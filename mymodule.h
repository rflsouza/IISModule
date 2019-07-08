#ifndef __MY_MODULE_H__
#define __MY_MODULE_H__

//  The module implementation.
//  This class is responsible for implementing the 
//  module functionality for each of the server events
//  that it registers for.
class CMyHttpModule : public CHttpModule
{
public:
	CMyHttpModule();
	~CMyHttpModule();

	// RQ_BEGIN_REQUEST
	REQUEST_NOTIFICATION_STATUS
	OnBeginRequest(
		IN IHttpContext *         pHttpContext,
		IN OUT IHttpEventProvider *   pProvider
	);


	// RQ_ACQUIRE_REQUEST_STATE - Implementation of the AcquireRequestState event handler method.
    REQUEST_NOTIFICATION_STATUS
    OnAcquireRequestState(
        IN IHttpContext *                       pHttpContext,
        IN OUT IHttpEventProvider *             pProvider
    );	
	
	// RQ_EXECUTE_REQUEST_HANDLER
	REQUEST_NOTIFICATION_STATUS
	OnPreExecuteRequestHandler(
		IN IHttpContext *             pHttpContext,
		IN IHttpEventProvider *       pProvider
	);


	// RQ_EXECUTE_REQUEST_HANDLER
	REQUEST_NOTIFICATION_STATUS
	OnPostExecuteRequestHandler(
		IN IHttpContext *             pHttpContext,
		IN IHttpEventProvider *       pProvider
	);


};

class WebSocket : public IHttpStoredContext 
{

public:
	
	/*
	IIS variable
	*/
	//IHttp- variable
	IHttpServer* m_HttpServer;
	//http context
	IHttpContext* m_HttpContext;

	void * readBuffer;
	DWORD readLength = 1024;

	//Constructor
	WebSocket() = delete;
	WebSocket(IHttpServer *is, IHttpContext *ic) :m_HttpServer(is), m_HttpContext(ic)
	{
		readBuffer = ic->AllocateRequestMemory(readLength);
	};

	//move operator
//	WebSocket& operator=(WebSocket&&);

	//deleter
	void CleanupStoredContext() 
	{
		m_HttpServer = nullptr;
		m_HttpContext = nullptr;
		delete this;
	};


		//State Machine Async Function
		void WINAPI ReadAsyncCompletion(HRESULT hrError, VOID * pvCompletionContext, DWORD cbIO, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
		{
			if (FAILED(hrError))
			{

			}
		};

		void WINAPI WritAsyncCompletion(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose) 
		{
		};

		//Supplementaly
		void WINAPI fWebSocketNULL(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
		{
		};
	
};

#endif //__MY_MODULE_H__
