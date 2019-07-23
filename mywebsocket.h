#ifndef __MY_WEBSOCKET_H__
#define __MY_WEBSOCKET_H__

#pragma once
#include "CLog.h"
#include "httpserv.h"
#include "iiswebsocket.h"
#include "libwshandshake.hpp"

using Mutex = std::mutex; //std::mutex recursive_mutex

class MyWebSocket : public IHttpStoredContext
{
protected:
	DWORD BUFFERLENGTH = 1024;


public:
	CLog *p_log;

	struct REMOTE
	{
		HTTP_CONNECTION_ID connectionId;
		std::string addr;
		std::string host;
		std::string port;
		std::string user;
		std::string userAgent;
	} remote;

	Mutex mtx;

	/*
	IIS variable
	*/
	//IHttp- variable
	IHttpServer* m_HttpServer;
	//http context
	IHttpContext* m_HttpContext;
	// socket context
	IWebSocketContext * m_WebSocketContext;

	void * readBuffer;
	DWORD readBufferLength;
	void * writeBuffer;

	std::string data;

	//Constructor
	MyWebSocket() = delete;
	MyWebSocket(CLog *log, IHttpServer *is, IHttpContext *ic, IWebSocketContext *wsc);
	~MyWebSocket();

	//move operator
//	WebSocket& operator=(WebSocket&&);

	//deleter
	void CleanupStoredContext();

	void Reading();
	HRESULT Write(std::string data);

	void continuousReading();
};

namespace functorWebSocket {
	//State Machine Async Function
	void WINAPI ReadAsyncCompletion(HRESULT hrError, VOID * pvCompletionContext, DWORD cbIO, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose);
	void WINAPI WritAsyncCompletion(HRESULT hrError, PVOID pvCompletionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose);

	void WINAPI ContinuousReadAsyncCompletion(HRESULT hrError, VOID * pvCompletionContext, DWORD cbIO, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose);

	//Supplementaly
	void WINAPI fWebSocketNULL(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose);
}

#endif // __MY_WEBSOCKET_H__