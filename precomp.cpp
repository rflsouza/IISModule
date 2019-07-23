#include "precomp.h"


//  Global server instance
IHttpServer * g_pHttpServer = NULL;
//  Global module context id
PVOID g_pModuleContext = NULL;
// Object Helpers IIS
IISHelpers g_IISHelper;

CLog *p_log;
IISCounter g_IISCounter;
std::string gAppPath;
std::map<HTTP_CONNECTION_ID, MyWebSocket*> g_ListWebSocket;