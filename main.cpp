#include "precomp.h"


/*
https://github.com/NeusoftSecurity/SEnginx

https://docs.microsoft.com/en-us/iis/develop/runtime-extensibility/develop-a-native-cc-module-for-iis
https://docs.microsoft.com/en-us/iis/develop/runtime-extensibility/sample-image-watermark-module
https://docs.microsoft.com/en-us/iis/web-development-reference/native-code-development-overview/designing-native-code-http-modules
https://docs.microsoft.com/en-us/iis/web-development-reference/native-code-development-overview/comparing-native-code-and-managed-code-notifications
*/

/*
	Entry-Point Function
	https://docs.microsoft.com/en-us/windows/desktop/dlls/dynamic-link-library-entry-point-function
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{

#ifdef DEBUG // Para debug no VS Startup.
	//::Sleep(10000);
#endif 

	std::ostringstream strLog;

	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
			
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.			
		OutputDebugString(__FUNCTION__ " DLL_PROCESS_ATTACH \n");
		
		gAppPath = "C:\\applogs\\proxy-webhook\\";

		p_log = new CLog(gAppPath + "iis-module");
		p_log->write("DLL_PROCESS_ATTACH");			
		strLog << "Path: " << gAppPath;
		p_log->write(&strLog, true);

		g_IISHelper.RegisterIIHelpersModule();

		break;		
	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		OutputDebugString(__FUNCTION__ " DLL_THREAD_ATTACH \n");
		p_log->write("DLL_THREAD_ATTACH");
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		OutputDebugString(__FUNCTION__ " DLL_THREAD_DETACH \n");
		p_log->write("DLL_THREAD_DETACH", true);
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		OutputDebugString(__FUNCTION__ " DLL_PROCESS_DETACH \n");
		p_log->write("DLL_PROCESS_DETACH", true);
		delete p_log;
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

// https://docs.microsoft.com/en-us/iis/develop/runtime-extensibility/develop-a-native-cc-module-for-iis
// https://www.iis.net/downloads/community/2007/01/iis7-native-api-cplusplus-starter-kit
// Para registrar o modulo precisa ser no IIS %windir%\system32\inetsrv\config\applicationhost.config
// <add name="IIS7NativeModule" image="C:\projetos\workcenter\xwc-iis-module\x64\Debug\IIS7NativeModule.dll" />
//     https://docs.microsoft.com/en-us/iis/get-started/introduction-to-iis/iis-modules-overview
//  The RegisterModule entrypoint implementation.
//  This method is called by the server when the module DLL is 
//  loaded in order to create the module factory,
//  and register for server events.
HRESULT
__stdcall
RegisterModule(
    DWORD                           dwServerVersion,
    IHttpModuleRegistrationInfo *   pModuleInfo,
    IHttpServer *                   pHttpServer
)
{
    HRESULT hr = S_OK;
	
	std::ostringstream strLog;
	strLog << __FUNCTION__ << " Start";
	p_log->write(&strLog);

    if ( pModuleInfo == NULL || pHttpServer == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

	strLog << __FUNCTION__ << " Save the IHttpServer and the module context id for future use";
	p_log->write(&strLog);
	// Save the IHttpServer and the module context id for future use
	g_pModuleContext = pModuleInfo->GetId();
	g_pHttpServer = pHttpServer;

#pragma region REGISTER GLOBAL MODULES
	strLog << __FUNCTION__ << " Create GlobalModule";
	p_log->write(&strLog);

	MyGlobalModule * pGlobalModule = new MyGlobalModule;

	if (NULL == pGlobalModule)
	{
		strLog << __FUNCTION__ << " Error to Create GlobalModule";
		p_log->write(&strLog);

		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	DWORD optionsGlobalNotifications = GL_CONFIGURATION_CHANGE | GL_FILE_CHANGE | GL_PRE_BEGIN_REQUEST | GL_APPLICATION_START | GL_APPLICATION_STOP | GL_APPLICATION_PRELOAD;

	strLog << __FUNCTION__ << " SetGlobalNotifications";
	p_log->write(&strLog);
	hr = pModuleInfo->SetGlobalNotifications(pGlobalModule, optionsGlobalNotifications);

	if (FAILED(hr))
	{
		strLog << __FUNCTION__ << " SetGlobalNotifications FAILED:" << hr;
		p_log->write(&strLog);

		return hr;
	}

#pragma endregion


#pragma region REGISTER MODULES
	strLog << __FUNCTION__ << " Create ModuleFactory";
	p_log->write(&strLog);

	CMyHttpModuleFactory  *  pFactory = NULL;

    // create the module factory
    pFactory = new CMyHttpModuleFactory();
    if ( pFactory == NULL )
    {
		strLog << __FUNCTION__ << " Error Create ModuleFactory";
		p_log->write(&strLog);

        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }

    // register for server events
    // TODO: register for more server events here
	/*
	#define RQ_BEGIN_REQUEST               0x00000001 // request is beginning 
	#define RQ_AUTHENTICATE_REQUEST        0x00000002 // request is being authenticated             
	#define RQ_AUTHORIZE_REQUEST           0x00000004 // request is being authorized 
	#define RQ_RESOLVE_REQUEST_CACHE       0x00000008 // satisfy request from cache 
	#define RQ_MAP_REQUEST_HANDLER         0x00000010 // map handler for request 
	#define RQ_ACQUIRE_REQUEST_STATE       0x00000020 // acquire request state 
	#define RQ_PRE_EXECUTE_REQUEST_HANDLER 0x00000040 // pre-execute handler 
	#define RQ_EXECUTE_REQUEST_HANDLER     0x00000080 // execute handler 
	#define RQ_RELEASE_REQUEST_STATE       0x00000100 // release request state 
	#define RQ_UPDATE_REQUEST_CACHE        0x00000200 // update cache 
	#define RQ_LOG_REQUEST                 0x00000400 // log request 
	#define RQ_END_REQUEST                 0x00000800 // end request
	*/
	
	DWORD optionsRequestNotifications = RQ_BEGIN_REQUEST | RQ_ACQUIRE_REQUEST_STATE | RQ_PRE_EXECUTE_REQUEST_HANDLER;
	DWORD optionsPostRequestNotifications = RQ_EXECUTE_REQUEST_HANDLER;
	
	strLog << __FUNCTION__ << " SetRequestNotifications";
	p_log->write(&strLog);

	hr = pModuleInfo->SetRequestNotifications(pFactory /* module factory */,
		optionsRequestNotifications /* server event mask */,
		optionsPostRequestNotifications /* server post event mask */);

	if (FAILED(hr)) 
	{
		strLog << __FUNCTION__ << " SetRequestNotifications FAILED:" << hr;
		p_log->write(&strLog);

		goto Finished;
	}

    pFactory = NULL;

#pragma endregion

Finished:
    
    if ( pFactory != NULL )
    {
        delete pFactory;
        pFactory = NULL;
    }   

    return hr;
}
