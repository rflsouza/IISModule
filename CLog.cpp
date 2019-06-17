#include "CLog.h"

CLog::CLog(std::string pathFile) : flushCount(0)
{
	m_pathFile = pathFile;
	
	createFile();

	if (ofs->is_open()) {
		*ofs << "Log inicializado ProcessId:" << GetCurrentProcessId() << " ThreadId:" << GetCurrentThreadId() << " \n\n";
	}	
}

CLog::~CLog()
{
	*ofs << "Log Finalizado\n\n";
	ofs->close();
}

void CLog::createFile()
{	
	GetLocalTime(&SysTime);	
	sprintf_s(&strDate[0], 63, "%.4d%.2d%.2d%.2d%.2d%.2d%.3d", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds);
	std::string file = m_pathFile + "-" + std::string(&strDate[0]) + ".log";

	try { 
		ofs = std::make_unique<std::ofstream>(file, std::ofstream::out);


		if (!ofs->is_open())
		{
			std::string erro = strerror(errno);
			std::cerr << "Critical, unable to create log file " << erro;
			createFile();
		}
		else
		{
			listFiles.push(file);
		}
	}
	catch (std::exception ex)
	{
		std::cerr << "Critical, unable to create log file" << ex.what();
	}
}

void CLog::checkFile()
{
	long size = ofs->tellp();
	//*ofs << "size log:" << size << std::endl;
	if (size > (20*1024*1024)) {

		ofs->flush();
		ofs->close();

		if (listFiles.size() >= 5) 
		{
			std::string file = listFiles.front();
			std::cout << "Remove file:" << file << std::endl;

			if (remove(file.c_str()) != 0) {
				std::cout << "Erro to remove file:" << file << std::endl;				
			}				
			else {
				std::cout << "Remove file:" << file << " successfully" << std::endl;
				listFiles.pop();
			}							
		}

		createFile();
	}
}

void CLog::write(const char *text, bool forceWrite)
{
	GetLocalTime(&SysTime);

	std::lock_guard<std::mutex> lk(mutex_);

	sprintf_s(&strDate[0], 63, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d.%.3d [%.6d] ", SysTime.wDay, SysTime.wMonth, SysTime.wYear, SysTime.wHour, SysTime.wMinute, SysTime.wSecond, SysTime.wMilliseconds, std::this_thread::get_id());

	checkFile();

	*ofs << &strDate[0] << text << "\n";

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

void CLog::write(const std::string &text, bool forceWrite)
{
	write(text.c_str(), forceWrite);
}

void CLog::write(std::ostringstream *text, bool forceWrite)
{	
	std::string msg = text->str();
	text->str(""); text->clear();	

	write(msg.c_str(), forceWrite);
}
