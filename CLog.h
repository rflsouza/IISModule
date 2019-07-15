#pragma once
#ifndef _XWC_CLog_H_
#define _XWC_CLog_H_

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::ostringstream
#include <fstream>		// std::ofstream
#include <mutex>
#include <queue>
#include <memory>
#include <atomic>

#include <windows.h>
#include <tchar.h>

#if defined(UNICODE) || defined(_UNICODE)
	typedef std::wstring String;
	#define to_String std::to_wstring
	#define Serror _wcserror 
	#define Cerr std::wcerr
	#define Cout std::wcout
	#define OStringstream std::wostringstream
	#define Ofstream std::wofstream
#else
	typedef std::string String;	
	#define Serror strerror 
	#define Cerr std::cerr
	#define Cout std::cout
	#define OStringstream std::ostringstream
	#define to_String std::to_string
	#define Ofstream std::ofstream
#endif


class CLog
{
private:
	std::unique_ptr<Ofstream> ofs;
	std::mutex mutex_;

	String m_pathFile;
	std::queue<String> listFiles;

	std::atomic<int> flushCount;

	TCHAR strDate[64];
	SYSTEMTIME SysTime;

	void createFile();
	void checkFile();

public:
	/* Create Log
	pathFile = path of file, sample: C:\\applogs\\proxy-webhook\\
	prefixFile = prefix of File name, sample: proxy
	*/
	CLog(String pathFile, String prefixFile);
	~CLog();
	void write(const TCHAR*, bool forceWrite = false);
	void write(const String&, bool forceWrite = false);
	void write(OStringstream *, bool forceWrite = false);
};

#endif // _XWC_CLog_H_