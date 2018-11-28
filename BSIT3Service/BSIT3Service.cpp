#include <Windows.h>
#include <tchar.h>

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;



VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

void Start_Service();
void Install_Service();
void Delete_Service();
void Stop_Service();

#define SERVICE_NAME  _T("123456789")

int _tmain(int argc, TCHAR *argv[])
{
	LPTSTR servicePath = LPTSTR(argv[0]);

	if (argc - 1 == 0) 
	{
		SERVICE_TABLE_ENTRY ServiceTable[1];
		ServiceTable[0].lpServiceName = (LPWSTR)SERVICE_NAME;
		ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

		if (!StartServiceCtrlDispatcher(ServiceTable)) 
		{
			OutputDebugString(_T("Error: StartServiceCtrlDispatcher"));
		}
	}
	else if (wcscmp(argv[argc - 1], _T("start")) == 0) 
	{
		Start_Service();
	}
	else if (wcscmp(argv[argc - 1], _T("install")) == 0)
	{
		Install_Service();
	}
	else if (wcscmp(argv[argc - 1], _T("delete")) == 0) 
	{
		Delete_Service();
	}
	else if (wcscmp(argv[argc - 1], _T("stop")) == 0)
	{
		Stop_Service();
	}

	//OutputDebugString(_T("My Sample Service: Main: Entry"));

	//SERVICE_TABLE_ENTRY ServiceTable[] =
	//{
	//	{(LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
	//	{NULL, NULL}
	//};

	//if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	//{
	//	OutputDebugString(_T("My Sample Service: Main: StartServiceCtrlDispatcher returned error"));
	//	return GetLastError();
	//}

	//OutputDebugString(_T("My Sample Service: Main: Exit"));
	return 0;
}


VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	g_ServiceStatus.dwServiceType = SERVICE_WIN32;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
	g_ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
	g_ServiceStatus.dwCheckPoint = 0;
	g_ServiceStatus.dwWaitHint = 0;
	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
	if (!g_StatusHandle)		return;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
	g_ServiceStopEvent = CreateEvent(0, FALSE, FALSE, 0);
	g_ServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	// Start a thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	// Wait until our worker thread exits signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
	CloseHandle(g_ServiceStopEvent);
	g_ServiceStopEvent = 0;
	g_ServiceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Entry"));

	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		 * Perform tasks neccesary to stop the service here
		 */

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}

	OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Exit"));
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Entry"));

	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		/*
		 * Perform main service function here
		 */
		OutputDebugString(_T("dioioio"));
		 //  Simulate some work by sleeping
		Sleep(3000);
	}

	OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Exit"));

	return ERROR_SUCCESS;
}

void Start_Service()
{
	SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);
	if (serviceControlManager)
	{
		SC_HANDLE service = OpenService(serviceControlManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
		if (service)
		{
			StartService(service, NULL, NULL);
			CloseServiceHandle(service);
		}
		OutputDebugString(_T("Open success"));
	}
	CloseServiceHandle(serviceControlManager);
}

void Install_Service()
{
	SC_HANDLE serviceControlManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (serviceControlManager)
	{
		TCHAR path[_MAX_PATH + 1];
		if (GetModuleFileName(0, path, sizeof(path) / sizeof(path[0])) > 0)
		{
			SC_HANDLE service = CreateService(serviceControlManager, SERVICE_NAME, SERVICE_NAME, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
				SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, path,
				0, 0, 0, 0, 0);

			CloseServiceHandle(service);
			OutputDebugString(_T("Install success"));
		}
		else {
			OutputDebugString(_T("GetModuleFileName error : %d", GetLastError()));

		}

		CloseServiceHandle(serviceControlManager);
	}
	else {
		OutputDebugString(_T("OpenSCManager error : %d", GetLastError()));
	}
}

void Delete_Service()
{
	SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);

	if (serviceControlManager)
	{
		SC_HANDLE service = OpenService(serviceControlManager,
			SERVICE_NAME, SERVICE_QUERY_STATUS | DELETE);
		if (service)
		{
			SERVICE_STATUS serviceStatus;
			if (QueryServiceStatus(service, &serviceStatus))
			{
				if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
					DeleteService(service);
				else OutputDebugString(_T("\nService should be stopped."));
			}

			CloseServiceHandle(service);
		}

		CloseServiceHandle(serviceControlManager);
		OutputDebugString(_T("\nDelete success"));
	}
}

void Stop_Service()
{
	SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);
	if (serviceControlManager)
	{
		SC_HANDLE service = OpenService(serviceControlManager, SERVICE_NAME, SERVICE_QUERY_STATUS | SERVICE_STOP);
		if (service)
		{
			SERVICE_STATUS serviceStatus;
			if (QueryServiceStatus(service, &serviceStatus))
			{
				if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
					ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus);
				else OutputDebugString(_T("\nService should be running.\n"));
			}
		}
		OutputDebugString(_T("Stop success"));
	}
	CloseServiceHandle(serviceControlManager);
}

