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
#include "mywebsocket.h"

//  Global server instance
extern IHttpServer * g_pHttpServer;
//  Global module context id
extern PVOID g_pModuleContext;

extern IISHelpers g_IISHelper;


extern CLog *p_log;
extern IISCounter g_IISCounter;
extern std::string gAppPath;
extern std::map<HTTP_CONNECTION_ID, MyWebSocket*> g_ListWebSocket;

#endif //__PRECOMP_H__

