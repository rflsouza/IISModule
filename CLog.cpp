#include "CLog.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

CLog::CLog(String pathFile, String prefixFile) : flushCount(0)
{
	m_pathFile = pathFile;
	m_pathFile += _T("Logs\\");

	if (!fs::is_directory(m_pathFile) || !fs::exists(m_pathFile)) {
		fs::create_directory(m_pathFile);
	}
	
	m_pathFile += prefixFile;

	createFile();

	if (ofs->is_open()) {
		*ofs << _T("Log inicializado ProcessId:") << GetCurrentProcessId() << _T(" ThreadId:") << GetCurrentThreadId() << _T(" \n\n");
	}	
}

CLog::~CLog()
{
	*ofs << _T("Log Finalizado\n\n");
	ofs->close();
}

void CLog::createFile()
{	
	GetLocalTime(&SysTime);		

#if defined(UNICODE) || defined(_UNICODE)
	swprintf_s(&strDate[0], 63, _T("%.4d%.2d%.2d%.2d%.2d%.2d%.3d"), SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds);
#else
	sprintf_s(&strDate[0], 63, _T("%.4d%.2d%.2d%.2d%.2d%.2d%.3d"), SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds);
#endif

	String file = m_pathFile + _T("-") + String(&strDate[0]) + _T(".log");

	try { 
		ofs = std::make_unique<Ofstream>(file, Ofstream::out);


		if (!ofs->is_open())
		{
			String erro = Serror(errno);
			Cerr << _T("Critical, unable to create log file ") << erro;
			createFile();
		}
		else
		{
			listFiles.push(file);
		}
	}
	catch (std::exception ex)
	{
		Cerr << _T("Critical, unable to create log file") << ex.what();
	}
}

void CLog::checkFile()
{
	long size = ofs->tellp();
	//*ofs << "size log:" << size << std::endl;
	// 50MB = 50*1024*1024
	if (size > (50*1024*1024)) {

		ofs->flush();
		ofs->close();

		if (listFiles.size() >= 50) 
		{
			String file = listFiles.front();
			Cout << _T("Remove file:") << file << std::endl;

			if (fs::remove(file.c_str()) != 0) {
				Cout << _T("Erro to remove file:") << file << std::endl;
			}				
			else {
				Cout << _T("Remove file:") << file << _T(" successfully") << std::endl;
				listFiles.pop();
			}							
		}

		createFile();
	}
}

void CLog::write(const TCHAR *text, bool forceWrite)
{
	GetLocalTime(&SysTime);

	std::lock_guard<std::mutex> lk(mutex_);


#if defined(UNICODE) || defined(_UNICODE)
	swprintf_s(&strDate[0], 63, _T("%.2d/%.2d/%.2d %.2d:%.2d:%.2d.%.3d [%.6d] "), SysTime.wDay, SysTime.wMonth, SysTime.wYear, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds, std::this_thread::get_id());
#else
	sprintf_s(&strDate[0], 63, _T("%.2d/%.2d/%.2d %.2d:%.2d:%.2d.%.3d [%.6d] "), SysTime.wDay, SysTime.wMonth, SysTime.wYear, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds, std::this_thread::get_id());
#endif	

	checkFile();

	*ofs << &strDate[0] << text << _T("\n");

#ifdef DEBUG
	forceWrite = true;
#endif // DEBUG

	if (flushCount > 5000 || forceWrite) {
		ofs->flush();
		flushCount = 0;
	}
	else {		
		flushCount++;
	}
}

void CLog::write(const String &text, bool forceWrite)
{
	write(text.c_str(), forceWrite);
}

void CLog::write(OStringstream *text, bool forceWrite)
{	
	String msg = text->str();
	text->str(_T("")); text->clear();	

	write(msg.c_str(), forceWrite);
}
