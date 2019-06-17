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

#endif
