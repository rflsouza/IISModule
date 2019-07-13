#include "precomp.h"
#include "mywebsocket.h"


void MyWebSocket::Reading()
{
	std::ostringstream strLog;
	HRESULT hr = S_OK;
	USHORT statusSocket = 0;
	LPCWSTR statusBuffer = NULL;
	USHORT statusLength = 0;

	PCSTR strTemp;
	DWORD strTempLength;	
	
	IISHelpers::GetVariable(m_HttpContext, "REMOTE_ADDR", &strTemp, &strTempLength, false);
	remote.addr = strTemp ? strTemp : "";
	IISHelpers::GetVariable(m_HttpContext, "REMOTE_HOST", &strTemp, &strTempLength, false);
	remote.host = strTemp ? strTemp : "";
	IISHelpers::GetVariable(m_HttpContext, "REMOTE_PORT", &strTemp, &strTempLength, false);
	remote.port = strTemp ? strTemp : "";
	IISHelpers::GetVariable(m_HttpContext, "REMOTE_USER", &strTemp, &strTempLength, false);
	remote.user = strTemp ? strTemp : "";
	strLog << __FUNCTION__ << " REMOTE_ADDR: " << remote.addr << " REMOTE_HOST: " << remote.host << " REMOTE_PORT: " << remote.port << " REMOTE_USER: " << remote.user << std::endl;
	p_log->write(&strLog);

	IHttpRequest* pHttpRequest = m_HttpContext->GetRequest();
	HTTP_REQUEST * httpRequest = pHttpRequest->GetRawHttpRequest();
	remote.connectionId = httpRequest->ConnectionId;
	std::string ip = IISHelpers::GetIpAddr( httpRequest->Address.pLocalAddress );

	ULONG count = 0;
	while (true)
	{		
		/*strLog << __FUNCTION__ << " ... while "  << count++;
		p_log->write(&strLog);*/

		// https://www.iana.org/assignments/websocket/websocket.xml#close-code-number-rules
		//https://tools.ietf.org/html/rfc6455
		hr = m_WebSocketContext->GetCloseStatus(&statusSocket, &statusBuffer, &statusLength);
		if (hr == NULL || statusSocket == -1)
		{
			strLog << __FUNCTION__ << "websockets closed, Status:" << statusBuffer << " ret:" << hr << hr;
			p_log->write(&strLog);

			break;
		}

		ZeroMemory(readBuffer, BUFFERLENGTH);
		BOOL bUTF8Encoded = FALSE;
		// true if this is the final data fragment; otherwise false. need read more data!!!!
		BOOL bFinalFragment = TRUE;
		BOOL bConnectionClose = FALSE;
		// true if an asynchronous completion is pending for this call; otherwise, false. Running in this call!!!
		BOOL bCompletionExpected = FALSE;

		std::unique_lock<std::mutex> lck(mtx);		
		hr = m_WebSocketContext->ReadFragment(readBuffer, &BUFFERLENGTH, TRUE, &bUTF8Encoded, &bFinalFragment, &bConnectionClose, functorWebSocket::ReadAsyncCompletion, this, &bCompletionExpected);
		lck.unlock();

		if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			//normal event!
			//strLog << __FUNCTION__ << "normal event! -> ERROR_IO_PENDING READ websocket[" << remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			//p_log->write(&strLog);
			::Sleep(1000);
		}
		else if (FAILED(hr))
		{
			strLog << __FUNCTION__ << " ERROR READ websocket[" << remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			p_log->write(&strLog);
		} 
		else if (bCompletionExpected == FALSE)
		{
			// return Data synchrono. running now in this call.
			strLog << __FUNCTION__ << " READ SYNC websocket[" << remote.port << "]"
				<< " hr:" << hr
				<< " hr_win32:" << HRESULT_FROM_WIN32(hr)
				<< " fUTF8Encoded:" << bUTF8Encoded
				<< " fFinalFragment:" << bFinalFragment
				<< " fClose:" << bConnectionClose
				<< " bCompletionExpected:" << bCompletionExpected
				<< " strLength:" << strnlen((char *)readBuffer, BUFFERLENGTH)
				<< " text:" << (char *)readBuffer;
			p_log->write(&strLog);

			if (bFinalFragment == false) {
				//need read more data!
				//data.resize(BUFFERLENGTH);				
				//data.insert(data.end(),(const char *)readBuffer, (size_t)BUFFERLENGTH);
				data.append((char *)readBuffer);
			}
			else
			{
				//data.resize(BUFFERLENGTH);
				//data.insert(data.end(), (const char *)readBuffer, (const size_t)BUFFERLENGTH);
				data.append((char *)readBuffer);

				// return Data synchrono. running now in this call.
				strLog << __FUNCTION__ << " READ SYNC websocket[" << remote.port << "]"
					<< " data.length:" << data.length()
					<< " data.capacity:" << data.capacity()
					<< " data:" << data.data();
				p_log->write(&strLog);

				// WRITE ECHO
				std::string echo("echo " + std::string(data.data()));
				HRESULT hr = Write(echo);
				if (FAILED(hr)) {
					strLog << "\n" << __FUNCTION__ << " Error to Write:" << echo;
					p_log->write(&strLog);
				}
				// FIM ECHO

				data.clear();
				strLog << __FUNCTION__ << " READ SYNC websocket[" << remote.port << "]"
					<< " after clear" 
					<< " data.length:" << data.length()
					<< " data.capacity:" << data.capacity()
					<< " data:" << data.data();
			}
		}
		else
		{
			// return Data asynchrono. running in ReadAsyncCompletion
			strLog << __FUNCTION__ << " READ ASYNC websocket[" << remote.port << "]"
				<< " hr:" << hr
				<< " hr_win32:" << HRESULT_FROM_WIN32(hr)
				<< " fUTF8Encoded:" << bUTF8Encoded
				<< " fFinalFragment:" << bFinalFragment
				<< " fClose:" << bConnectionClose
				<< " bCompletionExpected:" << bCompletionExpected
				<< " strLength:" << strnlen((char *)readBuffer, BUFFERLENGTH)
				<< " text:" << (char *)readBuffer
				<< "--------------------------------------------------------";
			p_log->write(&strLog);
		}


		//::Sleep(1000);
	}
}

HRESULT MyWebSocket::Write(std::string data)
{
	std::ostringstream strLog;
	HRESULT hr = S_OK;
	
	std::unique_lock<std::mutex> lck(mtx);

	ZeroMemory(writeBuffer, BUFFERLENGTH);
	strcpy((char*)writeBuffer, data.c_str());
	DWORD writeLength = strlen((char*)writeBuffer);

	hr = m_WebSocketContext->WriteFragment(writeBuffer, &writeLength, TRUE, FALSE, TRUE, functorWebSocket::WritAsyncCompletion, this, NULL);
	if (FAILED(hr)) {
		strLog << __FUNCTION__ << " Error WriteFragment hr:" << hr;
		p_log->write(&strLog);
	}

	return hr;
}


void WINAPI functorWebSocket::ReadAsyncCompletion(HRESULT hrError, VOID * pvCompletionContext, DWORD cbIO, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
{
	std::ostringstream strLog;

	if (FAILED(hrError))
	{
		strLog << __FUNCTION__ << "Error ReadAsyncCompletion hr:" << hrError;
		p_log->write(&strLog);
	}

	MyWebSocket* pws = (MyWebSocket*)pvCompletionContext;

	std::unique_lock<std::mutex> lck(pws->mtx);

	DWORD readLength = 1024;
	OutputDebugString((char *)pws->readBuffer);

	strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
		<< " cbIO:" << cbIO
		<< " fUTF8Encoded:" << fUTF8Encoded
		<< " fFinalFragment:" << fFinalFragment
		<< " fClose:" << fClose
		<< " Length:" << readLength
		<< " strLength:" << strnlen((char *)pws->readBuffer, readLength)
		<< " text:" << (char *)pws->readBuffer;
	pws->p_log->write(&strLog);

	if (fFinalFragment == false) {
		//need read more data!
		//pws->data.resize(pws->data.size() + cbIO);
		//pws->data.insert(pws->data.end(), (const char *)pws->readBuffer, (const size_t)cbIO);
		pws->data.append((char *)pws->readBuffer);
	}
	else
	{
		//pws->data.resize(pws->data.size() + cbIO);
		//pws->data.insert(pws->data.end(), (const char *)pws->readBuffer, (const size_t)cbIO);
		pws->data.append((char *)pws->readBuffer);

		// return Data synchrono. running now in this call.
		strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
			<< " data.length:" << pws->data.length()
			<< " data.capacity:" << pws->data.capacity()
			<< " data:" << pws->data.data();
		p_log->write(&strLog);

	
		// WRITE ECHO
		lck.unlock();
		std::string echo("echo " + std::string(pws->data.data()));
		HRESULT hr = pws->Write(echo);
		if (FAILED(hr)) {
			strLog << "\n" << __FUNCTION__ << " Error to Write:" << echo;
			pws->p_log->write(&strLog);
		}
		// FIM ECHO

		pws->data.clear();
		strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
			<< " after clear"
			<< " data.length:" << pws->data.length()
			<< " data.capacity:" << pws->data.capacity()
			<< " data:" << pws->data.data();
	}

	//HRESULT hrac;
	//void * readBufferMore = pws->m_HttpContext->AllocateRequestMemory(readLength);
	//while (!fFinalFragment) 
	//{
	//	BOOL tCompletionExpected = FALSE;
	//	pws->m_WebSocketContext->CancelOutstandingIO();

	//	//read again
	//	cbIO = 10;

	//	hrac = pws->m_WebSocketContext->ReadFragment(readBufferMore, &cbIO, TRUE, &fUTF8Encoded, &fFinalFragment, &fClose, functorWebSocket::fWebSocketNULL, NULL, &tCompletionExpected);

	//	//has read
	//	if (cbIO > 0) {
	//		strLog << __FUNCTION__ << " READY while websocket[" << pws->remote.port << "]"
	//			<< " cbIO:" << cbIO
	//			<< " fUTF8Encoded:" << fUTF8Encoded
	//			<< " fFinalFragment:" << fFinalFragment
	//			<< " fClose:" << fClose
	//			<< " Length:" << readBufferMore
	//			<< " strLength:" << strnlen((char *)readBufferMore, readLength)
	//			<< " text:" << (char *)readBufferMore;
	//		pws->p_log->write(&strLog);
	//	}
	//	if (FAILED(hrac)) {
	//		strLog << "\n" << __FUNCTION__ << " Error to read while:" << hrac;
	//		p_log->write(&strLog);
	//		break;
	//	}
	//}


};

void WINAPI functorWebSocket::WritAsyncCompletion(HRESULT hrError, PVOID pvCompletionContext, DWORD cbIO, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
{
	std::ostringstream strLog;

	if (FAILED(hrError))
	{
		strLog << __FUNCTION__ << "Error WritAsyncCompletion hr:" << hrError;
		p_log->write(&strLog);
	}

	MyWebSocket* pws = (MyWebSocket*)pvCompletionContext;

	DWORD writeLength = 1024;
	OutputDebugString((char *)pws->writeBuffer);
	
	strLog << __FUNCTION__ << " WRITE websocket[" << pws->remote.port << "]"
		<< " cbIO:" << cbIO
		<< " fUTF8Encoded:" << fUTF8Encoded
		<< " fFinalFragment:" << fFinalFragment
		<< " fClose:" << fClose
		<< " Length:" << writeLength
		<< " strLength:" << strnlen((char *)pws->writeBuffer, writeLength)
		<< " text:" << (char *)pws->writeBuffer;
	pws->p_log->write(&strLog);

};

//Supplementaly
void WINAPI functorWebSocket::fWebSocketNULL(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
{
};