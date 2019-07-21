#include "precomp.h"
#include "mywebsocket.h"


void MyWebSocket::Reading()
{
	std::ostringstream strLog;
	DWORD tick;
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
	IISHelpers::GetVariable(m_HttpContext, "HTTP_User-Agent", &strTemp, &strTempLength, false);
	remote.userAgent = strTemp ? strTemp : "";

	IHttpRequest* pHttpRequest = m_HttpContext->GetRequest();
	HTTP_REQUEST * httpRequest = pHttpRequest->GetRawHttpRequest();
	remote.connectionId = httpRequest->ConnectionId;
	std::string ip = IISHelpers::GetIpAddr( httpRequest->Address.pLocalAddress );

	strLog << __FUNCTION__ << " ESTABLISHED connectionId: " << remote.connectionId << " REMOTE_ADDR: " << remote.addr << " REMOTE_HOST: " << remote.host << " REMOTE_PORT: " << remote.port << " REMOTE_USER: " << remote.user << " User-Agent: " << remote.userAgent;
	p_log->write(&strLog);

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
			strLog << __FUNCTION__ << "websocket[" << remote.port << "]  closed, connectionId: " << remote.connectionId << " statusSocket:" << statusSocket << " statusBuffer:" << statusBuffer << " statusLength:" << statusLength << " hr:" << hr;
			p_log->write(&strLog);

			remote.connectionId = 0;
			m_WebSocketContext->CancelOutstandingIO();

			break;
		}

		readBufferLength = BUFFERLENGTH;
		ZeroMemory(readBuffer, readBufferLength);
		BOOL bUTF8Encoded = FALSE;
		// true if this is the final data fragment; otherwise false. need read more data!!!!
		BOOL bFinalFragment = TRUE;
		BOOL bConnectionClose = FALSE;
		// true if an asynchronous completion is pending for this call; otherwise, false. Running in this call!!!
		BOOL bCompletionExpected = FALSE;

		std::unique_lock<Mutex> lck(mtx);
		hr = m_WebSocketContext->ReadFragment(readBuffer, &readBufferLength, TRUE, &bUTF8Encoded, &bFinalFragment, &bConnectionClose, functorWebSocket::ReadAsyncCompletion, this, &bCompletionExpected);
		lck.unlock();

		if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			//normal event!
			//strLog << __FUNCTION__ << "normal event! -> ERROR_IO_PENDING READ websocket[" << remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			//p_log->write(&strLog);
			::Sleep(300);
		}
		else if (FAILED(hr))
		{
			strLog << __FUNCTION__ << " ERROR READ websocket[" << remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			p_log->write(&strLog);

			strLog << __FUNCTION__ << " FINALIZANDO websocket[" << remote.port << "]  statusSocket:" << statusSocket << " statusBuffer:" << statusBuffer << " statusLength:" << statusLength << " hr:" << hr;
			p_log->write(&strLog);
			
			remote.connectionId = 0;
			m_WebSocketContext->CancelOutstandingIO();
			break;
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
				<< " strLength:" << strnlen((char *)readBuffer, readBufferLength)
				<< " text:" << (char *)readBuffer;
			p_log->write(&strLog);

			if (bFinalFragment == false) {
				//need read more data!
				//data.resize(readBufferLength);				
				//data.insert(data.end(),(const char *)readBuffer, (size_t)readBufferLength);
				data.append((char *)readBuffer);
			}
			else
			{
				//data.resize(readBufferLength);
				//data.insert(data.end(), (const char *)readBuffer, (const size_t)readBufferLength);
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
					strLog << __FUNCTION__ << " READ SYNC websocket[" << remote.port << "]"
					<< " Error to WRITE ECHO:" << echo << " ErrorHandler:" << ErrorHandler(__FUNCTION__);
					p_log->write(&strLog);
				}
				// FIM ECHO

				data.clear();
				strLog << __FUNCTION__ << " READ IN SYNC websocket[" << remote.port << "]"
					<< " after clear" 
					<< " data.length:" << data.length()
					<< " data.capacity:" << data.capacity()
					<< " data:" << data.data();
				p_log->write(&strLog);
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
				<< " strLength:" << strnlen((char *)readBuffer, readBufferLength)
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
	
	std::unique_lock<Mutex> lck(mtx);

	if (this == nullptr)
		return -1;
	else if (remote.connectionId == 0) {
		strLog << __FUNCTION__ << " Disconected websocket[" << remote.port << "]";
		p_log->write(&strLog);
		return -2;
	}

	ZeroMemory(writeBuffer, BUFFERLENGTH);
	strcpy((char*)writeBuffer, data.c_str());
	DWORD writeLength = strlen((char*)writeBuffer);

	hr = m_WebSocketContext->WriteFragment(writeBuffer, &writeLength, TRUE, FALSE, TRUE, functorWebSocket::WritAsyncCompletion, this, NULL);
	if (FAILED(hr)) {
		strLog << __FUNCTION__ << " websocket[" << remote.port << "] Error WriteFragment hr:" << hr;
		p_log->write(&strLog);

		if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			
			//normal event!
			strLog << __FUNCTION__ << "websocket[" << remote.port << "] Waiting 300ms and Write Again -> ERROR_IO_PENDING Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			p_log->write(&strLog);

			::Sleep(300);

			hr = m_WebSocketContext->WriteFragment(writeBuffer, &writeLength, TRUE, FALSE, TRUE, functorWebSocket::WritAsyncCompletion, this, NULL);
			if (FAILED(hr)) {
				strLog << __FUNCTION__ << " Error WriteFragment hr:" << hr << "CRITICAL MSG NOT SEND websocket[" << remote.port << "] data:" << writeBuffer;
				p_log->write(&strLog);
			}
			else
			{
				strLog << __FUNCTION__ << " websocket[" << remote.port << "] RESEND WriteFragment sucesso.";
				p_log->write(&strLog);
			}
		}
	}

	return hr;
}

void MyWebSocket::continuousReading()
{
	std::ostringstream strLog;
	DWORD tick;
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
	IISHelpers::GetVariable(m_HttpContext, "HTTP_User-Agent", &strTemp, &strTempLength, false);
	remote.userAgent = strTemp ? strTemp : "";

	IHttpRequest* pHttpRequest = m_HttpContext->GetRequest();
	HTTP_REQUEST * httpRequest = pHttpRequest->GetRawHttpRequest();
	remote.connectionId = httpRequest->ConnectionId;
	std::string ip = IISHelpers::GetIpAddr(httpRequest->Address.pLocalAddress);

	

	strLog << __FUNCTION__ << " ESTABLISHED connectionId: " << remote.connectionId << " REMOTE_ADDR: " << remote.addr << " REMOTE_HOST: " << remote.host << " REMOTE_PORT: " << remote.port << " REMOTE_USER: " << remote.user << " User-Agent: " << remote.userAgent;
	p_log->write(&strLog);

	ULONG count = 0;

	/*strLog << __FUNCTION__ << " ... while "  << count++;
	p_log->write(&strLog);*/

	// https://www.iana.org/assignments/websocket/websocket.xml#close-code-number-rules
	//https://tools.ietf.org/html/rfc6455
	hr = m_WebSocketContext->GetCloseStatus(&statusSocket, &statusBuffer, &statusLength);
	if (hr == NULL || statusSocket == -1)
	{
		strLog << __FUNCTION__ << "websocket[" << remote.port << "]  closed, connectionId: " << remote.connectionId << " statusSocket:" << statusSocket << " statusBuffer:" << statusBuffer << " statusLength:" << statusLength << " hr:" << hr;
		p_log->write(&strLog);

		remote.connectionId = 0;
		m_WebSocketContext->CancelOutstandingIO();
	}

	readBufferLength = BUFFERLENGTH;
	ZeroMemory(readBuffer, readBufferLength);
	BOOL bUTF8Encoded = FALSE;
	// true if this is the final data fragment; otherwise false. need read more data!!!!
	BOOL bFinalFragment = TRUE;
	BOOL bConnectionClose = FALSE;
	// true if an asynchronous completion is pending for this call; otherwise, false. Running in this call!!!
	BOOL bCompletionExpected = FALSE;

	std::unique_lock<Mutex> lck(mtx);
	hr = m_WebSocketContext->ReadFragment(readBuffer, &readBufferLength, TRUE, &bUTF8Encoded, &bFinalFragment, &bConnectionClose, functorWebSocket::ContinuousReadAsyncCompletion, this, &bCompletionExpected);
	lck.unlock();

	if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
		//normal event!
		strLog << __FUNCTION__ << "normal event! -> ERROR_IO_PENDING READ websocket[" << remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
		p_log->write(&strLog);
		::Sleep(300);
	}
	else if (FAILED(hr))
	{
		strLog << __FUNCTION__ << " ERROR READ websocket[" << remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
		p_log->write(&strLog);

		strLog << __FUNCTION__ << " FINALIZANDO websocket[" << remote.port << "]  statusSocket:" << statusSocket << " statusBuffer:" << statusBuffer << " statusLength:" << statusLength << " hr:" << hr;
		p_log->write(&strLog);

		remote.connectionId = 0;
		m_WebSocketContext->CancelOutstandingIO();
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
			<< " strLength:" << strnlen((char *)readBuffer, readBufferLength)
			<< " text:" << (char *)readBuffer;
		p_log->write(&strLog);

		if (bFinalFragment == false) {
			//need read more data!
			//data.resize(readBufferLength);				
			//data.insert(data.end(),(const char *)readBuffer, (size_t)readBufferLength);
			data.append((char *)readBuffer);
		}
		else
		{
			//data.resize(readBufferLength);
			//data.insert(data.end(), (const char *)readBuffer, (const size_t)readBufferLength);
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
				strLog << __FUNCTION__ << " READ SYNC websocket[" << remote.port << "]"
					<< " Error to WRITE ECHO:" << echo << " ErrorHandler:" << ErrorHandler(__FUNCTION__);
				p_log->write(&strLog);
			}
			// FIM ECHO

			data.clear();
			strLog << __FUNCTION__ << " READ IN SYNC websocket[" << remote.port << "]"
				<< " after clear"
				<< " data.length:" << data.length()
				<< " data.capacity:" << data.capacity()
				<< " data:" << data.data();
			p_log->write(&strLog);
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
			<< " strLength:" << strnlen((char *)readBuffer, readBufferLength)
			<< " text:" << (char *)readBuffer
			<< "--------------------------------------------------------";
		p_log->write(&strLog);
	}	
	
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
	//std::shared_ptr<MyWebSocket> pws(reinterpret_cast<MyWebSocket*>(pvCompletionContext));	

	std::unique_lock<Mutex> lck(pws->mtx);

	if (pws == nullptr) 
		return;
	else if (pws->remote.connectionId == 0) {
		strLog << __FUNCTION__ << " Disconected websocket[" << pws->remote.port << "]";
		p_log->write(&strLog);
		return;
	}

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
			strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
				<< " Error to WRITE ECHO:" << echo << " ErrorHandler:" << ErrorHandler(__FUNCTION__);
			p_log->write(&strLog);
		}
		// FIM ECHO

		pws->data.clear();
		strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
			<< " after clear"
			<< " data.length:" << pws->data.length()
			<< " data.capacity:" << pws->data.capacity()
			<< " data:" << pws->data.data();
		p_log->write(&strLog);
	}
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
	//std::shared_ptr<MyWebSocket> pws(reinterpret_cast<MyWebSocket*>(pvCompletionContext));

	if (pws == nullptr)
		return;
	else if (pws->remote.connectionId == 0) {
		strLog << __FUNCTION__ << " Disconected websocket[" << pws->remote.port << "]";
		p_log->write(&strLog);
		return;
	}

	DWORD writeLength = 1024;
	OutputDebugString((char *)pws->writeBuffer);
	
	g_IISCounter.websocketsWrite++;
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



void WINAPI functorWebSocket::ContinuousReadAsyncCompletion(HRESULT hrError, VOID * pvCompletionContext, DWORD cbIO, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
{
	std::ostringstream strLog;
	HRESULT hr;

	if (FAILED(hrError))
	{
		strLog << __FUNCTION__ << " Error ReadAsyncCompletion hr:" << hrError;
		p_log->write(&strLog);
	}
	MyWebSocket* pws = (MyWebSocket*)pvCompletionContext;
	//std::shared_ptr<MyWebSocket> pws(reinterpret_cast<MyWebSocket*>(pvCompletionContext));	

	if (pws == nullptr)
		return;

	std::unique_lock<Mutex> lck(pws->mtx);

	if (pws->remote.connectionId == 0) {
		strLog << __FUNCTION__ << " Disconected websocket[" << pws->remote.port << "]";
		p_log->write(&strLog);
		return;
	}

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

	if (fClose || FAILED(hrError))
	{
		USHORT statusSocket = 0;
		LPCWSTR statusBuffer = NULL;
		USHORT statusLength = 0;
		hr = pws->m_WebSocketContext->GetCloseStatus(&statusSocket, &statusBuffer, &statusLength);
		if (hr == NULL || statusSocket == -1)
		{
			strLog << __FUNCTION__ << "websocket[" << pws->remote.port << "]  closed, connectionId: " << pws->remote.connectionId << " statusSocket:" << statusSocket << " statusBuffer:" << statusBuffer << " statusLength:" << statusLength << " hr:" << hr;
			p_log->write(&strLog);

			pws->CleanupStoredContext();	
			return;
		}
	}

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
		hr = pws->Write(echo);
		if (FAILED(hr)) {
			strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
				<< " Error to WRITE ECHO:" << echo << " ErrorHandler:" << ErrorHandler(__FUNCTION__);
			p_log->write(&strLog);
		}
		// FIM ECHO

		g_IISCounter.websocketsRead++;
		pws->data.clear();
		strLog << __FUNCTION__ << " READ ASYNC websocket[" << pws->remote.port << "]"
			<< " after clear"
			<< " data.length:" << pws->data.length()
			<< " data.capacity:" << pws->data.capacity()
			<< " data:" << pws->data.data();
		p_log->write(&strLog);
	}

	{
		pws->readBufferLength = 1024;
		ZeroMemory(pws->readBuffer, pws->readBufferLength);
		BOOL bUTF8Encoded = FALSE;
		// true if this is the final data fragment; otherwise false. need read more data!!!!
		BOOL bFinalFragment = TRUE;
		BOOL bConnectionClose = FALSE;
		// true if an asynchronous completion is pending for this call; otherwise, false. Running in this call!!!
		BOOL bCompletionExpected = FALSE;

		lck.lock();
		HRESULT hr = pws->m_WebSocketContext->ReadFragment(pws->readBuffer, &pws->readBufferLength, TRUE, &bUTF8Encoded, &bFinalFragment, &bConnectionClose, functorWebSocket::ContinuousReadAsyncCompletion, pws, &bCompletionExpected);
		lck.unlock();

		if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			//normal event!
			strLog << __FUNCTION__ << "normal event! -> ERROR_IO_PENDING READ websocket[" << pws->remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			p_log->write(&strLog);
			::Sleep(300);
		}
		else if (FAILED(hr))
		{
			strLog << __FUNCTION__ << " ERROR READ websocket[" << pws->remote.port << "] Error hr:" << hr << " hr_win32:" << HRESULT_FROM_WIN32(hr);
			p_log->write(&strLog);


			pws->remote.connectionId = 0;
			pws->m_WebSocketContext->CancelOutstandingIO();
		}
	}

};


//Supplementaly
void WINAPI functorWebSocket::fWebSocketNULL(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose)
{
};