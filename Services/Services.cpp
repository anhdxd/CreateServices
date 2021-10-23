#undef UNICODE 
#pragma warning(disable : 4996)
#include <Windows.h>
#include <iostream>


#define SVCNAME "AbSvcAnhdz"
#define LOGFILE "C:\\Users\\Vanh\\Desktop\\TestService\\memstatus.log"


void InstallSvc(); // Cài Services
void MainService(); // Hàm services chính
int WriteToLog(char* str);
void SvcCtrlHandler(DWORD dwCtrl); // Hàm xử lý request do control dispath gửi về từ SCM

HANDLE ghSvcStopEvent;
SERVICE_STATUS g_SvcStatus;
SERVICE_STATUS_HANDLE g_hSvcStatus;

int main()
{
    SERVICE_TABLE_ENTRY ServicesTable[2]; // Bảng này lưu các service định nghĩa trong ct
    ServicesTable[0].lpServiceName = (LPSTR)SVCNAME;
    ServicesTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)MainService; // tro toi ham main cua services

    ServicesTable[1].lpServiceName = 0;
    ServicesTable[1].lpServiceProc = 0; // tro toi ham main cua services

    InstallSvc();

    //StartServiceCtrlDispatcher(ServicesTable); // Buộc phải có tối thiểu 2 table, kết thúc = service null
    if (!StartServiceCtrlDispatcher(ServicesTable))
    {
        int result = WriteToLog((char*)"Deo gi y dcm");
    }
}


void InstallSvc()
{
    SC_HANDLE schSCManager, schService; // Handle cua trinh quan ly services va services
    TCHAR PathExeServices[MAX_PATH];
    if (!GetModuleFileName(0, PathExeServices, MAX_PATH))
    {
        printf("Cannot install service (%d)\n", GetLastError());

        return;
    }
    schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS); // mở trình quản lý
    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        //CloseServiceHandle(schSCManager);
        return;
    }

    schService = CreateService(schSCManager,
        SVCNAME, SVCNAME,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, // SERVICE_DEMAND_START: khi gọi hàm startservice thì chạy svc
        PathExeServices, 0, 0, 0, 0, 0);
    if (schService == NULL)
    {
        printf("CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        //CloseServiceHandle(schService);
        return;
    }
    else printf("Service installed successfully\n");

    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
    return;
}
void MainService() // Hàm này sẽ được chạy trong 1 Thread khác
{
    ghSvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL);   // no name
    // đăng kí hàm CtrlHandler() xử lý  các request do “control dispatcher” gửi về từ SCM. Hàm RegisterServiceCtrlHandler trả về status handle.
    SERVICE_STATUS_HANDLE hSvcStatus = RegisterServiceCtrlHandler(SVCNAME, (LPHANDLER_FUNCTION)SvcCtrlHandler);
    g_SvcStatus.dwServiceType = SERVICE_WIN32;
    g_SvcStatus.dwCurrentState = SERVICE_START_PENDING;
    g_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_SvcStatus.dwWin32ExitCode = 0;
    g_SvcStatus.dwServiceSpecificExitCode = 0;
    g_SvcStatus.dwCheckPoint = 0;
    g_SvcStatus.dwWaitHint = 0;
    // Chuyen thanh Running
    g_SvcStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hSvcStatus, &g_SvcStatus);
    //
    while (g_SvcStatus.dwCurrentState == SERVICE_RUNNING)
    {
        int result = WriteToLog((char*)"Deo gi y dcm");
        if (g_SvcStatus.dwCurrentState == SERVICE_STOPPED)
            return;
        Sleep(1000);
        
    }
    WriteToLog((char*)"BEN NGOAI MAIN SERVICE");

    //DWORD exitcode;
    //GetExitCodeProcess(GetCurrentProcess, &exitcode);
    //ExitProcess(exitcode);

}
void SvcCtrlHandler(DWORD dwCtrl)
{
    // Handle the requested control code. 
    char buffer[20] = { 0 };
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        // TODO: Write to the database

        g_SvcStatus.dwWin32ExitCode = 0;
        g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
        sprintf(buffer, "Service bi Stop %d", g_SvcStatus.dwCurrentState);
        WriteToLog(buffer);
        return;


    case SERVICE_CONTROL_SHUTDOWN:
        //SetEvent(ghSvcStopEvent);
        g_SvcStatus.dwWin32ExitCode = 0;
        g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
        WriteToLog((char*)"Monitoring sSHUTDOWN.");
        return;
    default:
        break;
    }
}

int WriteToLog(char* str)
{
    FILE* fpLog;
    fpLog = fopen(LOGFILE, "a+");
    if (fpLog == NULL)
        return -1;
    if (fprintf(fpLog, "%s\n", str) < strlen(str))
    {
        fclose(fpLog);
        return -1;
    }
    fclose(fpLog);
    return 0;
}