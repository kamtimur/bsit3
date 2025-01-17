#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>  
#pragma comment (lib,"libzip-static.lib")
#include "zip.h"
#include <time.h>
#include <string.h>
#include <fstream>
#include <string>

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE watcher_close_event = INVALID_HANDLE_VALUE;
LPCTSTR path;
void backup();
void back_file_up(zip_t *zip, char *file, WIN32_FIND_DATAA *find_data);
time_t filetime_to_timet(FILETIME const& ft);
void WatchDirectory(char *lpDir, void(*callback)(void), HANDLE close_event);
int addLogMessage(const char* str);
int addLogMessage(const char* str, int code);

char backup_file[] = "C:\\Users\\t440s\\Desktop\\arch\\1.zip";
char backup_folder[] = "C:\\Users\\t440s\\Desktop\\sourcefold\\";

const char config[] = "conf.txt";
char mask1[] = "A???.????";
char mask2[] = "*.jpg";
char mask3[] = "*.xml";
char mask4[] = "*.txt";
char*  masks[] = { mask1,mask2,mask3,mask4 };


VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
//DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

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
			addLogMessage("Error: StartServiceCtrlDispatcher");
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

	addLogMessage("!!!!!!!!!!");
	backup();
	//watcher_close_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	WatchDirectory(backup_folder, backup, g_ServiceStopEvent);


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
	addLogMessage("My Sample Service: ServiceCtrlHandler: Entry");

	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		addLogMessage("My Sample Service: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request");

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
			addLogMessage("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error");
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}

	addLogMessage("My Sample Service: ServiceCtrlHandler: Exit");
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
			addLogMessage("start success");
			CloseServiceHandle(service);
		}
		addLogMessage("Open success");
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
			addLogMessage("Install success");
		}
		else {
			addLogMessage("GetModuleFileName error : %d");

		}

		CloseServiceHandle(serviceControlManager);
	}
	else {
		addLogMessage("OpenSCManager error : %d", GetLastError());
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
				else addLogMessage("\nService should be stopped.");
			}

			CloseServiceHandle(service);
		}

		CloseServiceHandle(serviceControlManager);
		addLogMessage("\nDelete success");
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
				{
					ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus);
					SetEvent(g_ServiceStopEvent);
					addLogMessage("g_ServiceStopEvent");
				}
				else addLogMessage("\nService should be running.\n");
			}
		}
		addLogMessage("Stop success");
	}
	CloseServiceHandle(serviceControlManager);
}

void backup()
{
	addLogMessage("backup");
	int ierror;
	zip_t * zip = zip_open(backup_file, ZIP_CREATE, &ierror);
	char file_with_mask[MAX_PATH];
	char file[MAX_PATH];
	WIN32_FIND_DATAA find_data;

	for (char *mask : masks)
	{
		addLogMessage(mask);
		strcpy(file_with_mask, backup_folder);
		strcat(file_with_mask, mask);
		HANDLE h = FindFirstFileA(file_with_mask, &find_data);
		if (h == INVALID_HANDLE_VALUE) continue;
		if (find_data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) continue;
		addLogMessage(find_data.cFileName);
		back_file_up(zip, file, &find_data);
		while (FindNextFileA(h, &find_data))
		{
			addLogMessage(find_data.cFileName);
			back_file_up(zip, file, &find_data);
		}
	}
	int status = zip_close(zip);
}


void back_file_up(zip_t *zip, char *file, WIN32_FIND_DATAA *find_data)
{
	strcpy(file, backup_folder);
	strcat(file, find_data->cFileName);
	HANDLE hFile = CreateFileA(file,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
	if (hFile == INVALID_HANDLE_VALUE) {
		addLogMessage("file open error");
		return;
	}

	bool to_change = false;

	struct zip_stat stat;
	zip_stat_init(&stat);
	zip_stat(zip, find_data->cFileName, 0, &stat);

	// the file will be backed up if it is not present in the zip
	// or if its last write time is later than the time of last check.
	if (zip_name_locate(zip, find_data->cFileName, NULL) == -1 || (stat.mtime < filetime_to_timet(find_data->ftLastWriteTime)))
	{
		zip_error_t err;
		zip_source *zs = zip_source_win32handle_create(hFile, 0, 0, &err);
		if (zs == NULL)
		{
			CloseHandle(hFile);
			zip_close(zip);
			addLogMessage("file source creation error");
			return;
		}

		if (zip_file_add(zip, find_data->cFileName, zs, ZIP_FL_OVERWRITE | ZIP_FL_ENC_GUESS) == -1)
		{
			addLogMessage("file add error");
			return;
		}

		zip_source_close(zs);
	}
}

time_t filetime_to_timet(FILETIME const& ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

void WatchDirectory(char *lpDir, void(*callback)(void), HANDLE close_event)
{
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[2];
	char lpDrive[4];
	char lpFile[_MAX_FNAME];
	char lpExt[_MAX_EXT];

	addLogMessage("Enter watch.");

	_splitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	// Watch the directory for file creation and deletion. 

	dwChangeHandles[0] = FindFirstChangeNotificationA(
		lpDir,                         // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_ATTRIBUTES |
		FILE_NOTIFY_CHANGE_SIZE |
		FILE_NOTIFY_CHANGE_LAST_WRITE |
		FILE_NOTIFY_CHANGE_SECURITY); // watch file name changes 

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE)
	{
		addLogMessage("ERROR: FindFirstChangeNotification function failed.");
		ExitProcess(GetLastError());
	}

	dwChangeHandles[1] = close_event;

	// Make a final validation check on our handles.

	if ((dwChangeHandles[0] == NULL))
	{
		addLogMessage("ERROR: Unexpected NULL from FindFirstChangeNotification.");
		ExitProcess(GetLastError());
	}

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 

	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		// Wait for notification.

		addLogMessage("Waiting for notification...");

		dwWaitStatus = WaitForMultipleObjects(2, dwChangeHandles, FALSE, INFINITE);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:

			// A file was created, renamed, or deleted in the directory.
			// Refresh this directory and restart the notification.
			addLogMessage("changes...");
			callback();
			if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE)
			{
				addLogMessage("ERROR: FindNextChangeNotification function failed.");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 1:

			// close event;
			addLogMessage("close event.");
			ExitProcess(GetLastError());
			return;
			break;

		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			addLogMessage("No changes in the timeout period.");
			break;

		default:
			addLogMessage("ERROR: Unhandled dwWaitStatus.");
			ExitProcess(GetLastError());
			break;
		}
	}
}

int addLogMessage(const char* str)
{
	errno_t err;
	FILE* log;
	if ((err = fopen_s(&log, "log.txt", "a+")) != 0) {
		return -1;
	}
	fprintf(log, "%s\n", str);
	fclose(log);
	return 0;
}
int addLogMessage(const char* str, int code) 
{
	errno_t err;
	FILE* log;
	if ((err = fopen_s(&log, "log.txt", "a+")) != 0) {
		return -1;
	}
	fprintf(log, "[code: %u] %s\n", code, str);
	fclose(log);
	return 0;
}
