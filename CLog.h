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

class CLog
{
private:
	std::unique_ptr<std::ofstream> ofs;
	std::mutex mutex_;

	std::string m_pathFile;
	std::queue<std::string> listFiles;

	std::atomic<int> flushCount;

	char strDate[64];
	SYSTEMTIME SysTime;

	void createFile();
	void checkFile();

public:
	CLog(std::string pathFile);
	~CLog();
	void write(const char*, bool forceWrite = false);
	void write(const std::string&, bool forceWrite = false);
	void write(std::ostringstream *, bool forceWrite = false);
};

#endif // _XWC_CLog_H_