#pragma once
#define _WINSOCKAPI_
#include <httpserv.h>
#include <string>

class MyGlobalModule : public CGlobalModule
{
private:
	DWORD m_dwPageSize;

public:
	MyGlobalModule();
	~MyGlobalModule();

	// GL_CONFIGURATION_CHANGE - configuration changed
	GLOBAL_NOTIFICATION_STATUS
		OnGlobalConfigurationChange(
			IN IGlobalConfigurationChangeProvider * pProvider
	);

	// GL_FILE_CHANGE - file changed
	GLOBAL_NOTIFICATION_STATUS
		OnGlobalFileChange(
			IN IGlobalFileChangeProvider * pProvider
		);

	//GL_PRE_BEGIN_REQUEST - before request pipeline has started 
	GLOBAL_NOTIFICATION_STATUS
		OnGlobalPreBeginRequest(
			IN IPreBeginRequestProvider * pProvider
		);

	// GL_APPLICATION_START - application start
	GLOBAL_NOTIFICATION_STATUS
		OnGlobalApplicationStart(
			IN IHttpApplicationStartProvider * pProvider
		);

	// GL_APPLICATION_STOP - application end
	GLOBAL_NOTIFICATION_STATUS
		OnGlobalApplicationStop(
			IN IHttpApplicationStopProvider * pProvider
		);

	// GL_APPLICATION_PRELOAD - application preload notification
	GLOBAL_NOTIFICATION_STATUS
		OnGlobalApplicationPreload(
			IN IGlobalApplicationPreloadProvider * pProvider
		);

	VOID Terminate()
	{
		delete this;
	};

	HRESULT ReadFileChunk(HTTP_DATA_CHUNK *chunk, char *buf)
	{
		OVERLAPPED ovl;
		DWORD dwDataStartOffset;
		ULONGLONG bytesTotal = 0;
		BYTE *	pIoBuffer = NULL;
		HANDLE	hIoEvent = INVALID_HANDLE_VALUE;
		HRESULT hr = S_OK;

		pIoBuffer = (BYTE *)VirtualAlloc(NULL,
			1,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE);
		if (pIoBuffer == NULL)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto Done;
		}

		hIoEvent = CreateEvent(NULL,  // security attr
			FALSE, // manual reset
			FALSE, // initial state
			NULL); // name
		if (hIoEvent == NULL)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto Done;
		}

		while (bytesTotal < chunk->FromFileHandle.ByteRange.Length.QuadPart)
		{
			DWORD bytesRead = 0;
			int was_eof = 0;
			ULONGLONG offset = chunk->FromFileHandle.ByteRange.StartingOffset.QuadPart + bytesTotal;

			ZeroMemory(&ovl, sizeof ovl);
			ovl.hEvent = hIoEvent;
			ovl.Offset = (DWORD)offset;
			dwDataStartOffset = ovl.Offset & (m_dwPageSize - 1);
			ovl.Offset &= ~(m_dwPageSize - 1);
			ovl.OffsetHigh = offset >> 32;

			if (!ReadFile(chunk->FromFileHandle.FileHandle,
				pIoBuffer,
				m_dwPageSize,
				&bytesRead,
				&ovl))
			{
				DWORD dwErr = GetLastError();

				switch (dwErr)
				{
				case ERROR_IO_PENDING:
					//
					// GetOverlappedResult can return without waiting for the
					// event thus leaving it signalled and causing problems
					// with future use of that event handle, so just wait ourselves
					//
					WaitForSingleObject(ovl.hEvent, INFINITE); // == WAIT_OBJECT_0);

					if (!GetOverlappedResult(
						chunk->FromFileHandle.FileHandle,
						&ovl,
						&bytesRead,
						TRUE))
					{
						dwErr = GetLastError();

						switch (dwErr)
						{
						case ERROR_HANDLE_EOF:
							was_eof = 1;
							break;

						default:
							hr = HRESULT_FROM_WIN32(dwErr);
							goto Done;
						}
					}
					break;

				case ERROR_HANDLE_EOF:
					was_eof = 1;
					break;

				default:
					hr = HRESULT_FROM_WIN32(dwErr);
					goto Done;
				}
			}

			bytesRead -= dwDataStartOffset;

			if (bytesRead > chunk->FromFileHandle.ByteRange.Length.QuadPart)
			{
				bytesRead = (DWORD)chunk->FromFileHandle.ByteRange.Length.QuadPart;
			}
			if ((bytesTotal + bytesRead) > chunk->FromFileHandle.ByteRange.Length.QuadPart)
			{
				bytesRead = chunk->FromFileHandle.ByteRange.Length.QuadPart - bytesTotal;
			}

			memcpy(buf, pIoBuffer + dwDataStartOffset, bytesRead);

			buf += bytesRead;
			bytesTotal += bytesRead;

			if (was_eof != 0)
				chunk->FromFileHandle.ByteRange.Length.QuadPart = bytesTotal;
		}

	Done:
		if (NULL != pIoBuffer)
		{
			VirtualFree(pIoBuffer, 0, MEM_RELEASE);
		}

		if (INVALID_HANDLE_VALUE != hIoEvent)
		{
			CloseHandle(hIoEvent);
		}

		return hr;
	}

	
};

