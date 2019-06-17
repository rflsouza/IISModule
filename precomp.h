#ifndef __PRECOMP_H__
#define __PRECOMP_H__

// Include fewer Windows headers 
#define WIN32_LEAN_AND_MEAN

//  IIS7 Server API header file
#include "httpserv.h"

//  Project header files
#include "myglobalmodule.h"
#include "mymodule.h"
#include "mymodulefactory.h"
#include "iishelpers.h"
#include "CLog.h"

//  Global server instance
extern IHttpServer * g_pHttpServer;
//  Global module context id
extern PVOID g_pModuleContext;

extern IISHelpers g_IISHelper;


extern CLog *p_log;
extern std::atomic<unsigned long> g_requests;
extern std::string gAppPath;

#endif

