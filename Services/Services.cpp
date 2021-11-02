#undef UNICODE 
#pragma warning(disable : 4996)
#include <Windows.h>
#include <iostream>


#define SVCNAME "AbSvcAnhdz"
#define LOGFILE ".\\memstatus.log"
#define NAMEPIPES "\\\\.\\pipe\\testpipesVA"
#define BUFSIZE 512

void InstallSvc(); // Cài Services
void WINAPI MainService(); // Hàm services chính
int WriteToLog(char* str);
void WINAPI SvcCtrlHandler(DWORD dwCtrl); // Hàm xử lý request do control dispath gửi về từ SCM
int WriteToPipes(HANDLE hPipe, char* WriteBuff);
int SendRecvPipes(HANDLE hPipe);

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
        int result = WriteToLog((char*)"StartServiceCtrlDispatcher = 0");
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
        //WriteToLog((char*)"OpenSCManager Failed");
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
        //WriteToLog((char*)"Install Failed");
        printf("CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        //CloseServiceHandle(schService);
        return;
    }
    else printf("Install Successfuly");

    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
    return;
}
void WINAPI MainService() // Hàm này sẽ được chạy trong 1 Thread khác
{
    //**********************KHỞI TẠO **********************************
    char Err[512] = { 0 };
    BOOL bClientConnect = FALSE;
    ghSvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL);   // no name
    // đăng kí hàm CtrlHandler() xử lý  các request do “control dispatcher” gửi về từ SCM. Hàm RegisterServiceCtrlHandler trả về status handle.
    g_hSvcStatus = RegisterServiceCtrlHandler(SVCNAME, (LPHANDLER_FUNCTION)SvcCtrlHandler);
    g_SvcStatus.dwServiceType = SERVICE_WIN32;
    g_SvcStatus.dwCurrentState = SERVICE_START_PENDING;
    g_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_SvcStatus.dwWin32ExitCode = 0;
    g_SvcStatus.dwServiceSpecificExitCode = 0;
    g_SvcStatus.dwCheckPoint = 0;
    g_SvcStatus.dwWaitHint = 0;
    // Chuyen thanh Running
    g_SvcStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_hSvcStatus, &g_SvcStatus);

    //**************************** CODE *************************************
    // Create Pipes
    // Tạo quyền truy cập cho bên ngoài không cần admin
    PSECURITY_DESCRIPTOR SecurityDes = NULL;
    BYTE  sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SecurityDes = (PSECURITY_DESCRIPTOR)sd;
    InitializeSecurityDescriptor(SecurityDes, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(SecurityDes, TRUE, (PACL)NULL, FALSE);
    SECURITY_ATTRIBUTES SecurityAttribute = { sizeof(SecurityAttribute), SecurityDes, FALSE };
    //GetSecurityDescriptorSacl(&SecurityDes, (LPBOOL)TRUE,);
    while (g_SvcStatus.dwCurrentState == SERVICE_RUNNING)
    {
        HANDLE hPipe = CreateNamedPipe(
            NAMEPIPES,             // pipe name 
            PIPE_ACCESS_DUPLEX,       // read/write access 
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,       // message type pipe  |   // message-read mode // blocking mode 
            PIPE_UNLIMITED_INSTANCES, // max. instances  
            BUFSIZE,                  // output buffer size 
            BUFSIZE,                  // input buffer size 
            0,                        // client time-out 
            &SecurityAttribute);                    // default security attribute 
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            sprintf(Err, "hPipe Error: %d", GetLastError());
            WriteToLog(Err);
            return;
        }
        // Đợi Client kết nối, nếu ko thành công trả về error
        bClientConnect = ConnectNamedPipe(hPipe, 0) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (bClientConnect)
        {
            SendRecvPipes(hPipe);
        }
    }
    WriteToLog((char*)"BEN NGOAI MAIN SERVICE");
    return;
    //DWORD exitcode;
    //GetExitCodeProcess(GetCurrentProcess, &exitcode);
    //ExitProcess(exitcode);

}
void WINAPI SvcCtrlHandler(DWORD dwCtrl)
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
int SendRecvPipes(HANDLE hPipe)
{
    char TempBuff[BUFSIZE] = { 0 };
    char Err[512] = { 0 };
    char WriteBuff[100] = { 0 };
    HANDLE hFile;
    BOOL fSuccess = FALSE;
    DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0, cbToWrite = 0;
    // Cấp phát Heap, mặc dù méo biết dùng
    HANDLE hHeap = GetProcessHeap();
    char* pchReceive = (char*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(char));
    WriteToLog((char*)"Client connected!");
    // Check message từ client ở đây
    while (1)
    {
        // Đọc pipes nó sẽ đợi cho có thông tin mới nó mới đọc
        fSuccess = ReadFile(
            hPipe,        // handle to pipe 
            pchReceive,    // buffer to receive data 
            BUFSIZE * sizeof(char), // size of buffer 
            &cbBytesRead, // number of bytes read 
            NULL);
        // Check Nếu đọc ko thành công hoặc đọc được 0 Bytes
        if (!fSuccess || cbBytesRead == 0)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                WriteToLog((char*)"Client Disconnected");
            else
            {
                ZeroMemory(Err, sizeof(Err));
                sprintf(Err, "ReadFile failed Error: %d", GetLastError());
                WriteToLog(Err);
                //printf("ReadFile failed Error: %d", GetLastError());
            }
            break; // Quay lại trạng thái chờ client kết nối tới
        }
        ZeroMemory(TempBuff, sizeof("Delete:"));
        lstrcpyn(TempBuff, pchReceive, lstrlen("Delete:") + 1);
        if (lstrcmp(TempBuff, "AddFil:") == 0)
        {
            //printf(TempBuff);
            ZeroMemory(TempBuff, sizeof(TempBuff));
            lstrcpyn(TempBuff, &pchReceive[7], lstrlen(pchReceive) + 1);
            if ((hFile = CreateFile(TempBuff, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0)) != INVALID_HANDLE_VALUE)
                WriteToPipes(hPipe, (char*)"Create File Success !");
            else
            {
                sprintf(WriteBuff, "Create File Error: %d", GetLastError());
                WriteToPipes(hPipe, WriteBuff);
            }
            CloseHandle(hFile);

        }
        if (lstrcmp(TempBuff, "Delete:") == 0)
        {
            //printf(TempBuff);
            ZeroMemory(TempBuff, sizeof(TempBuff));
            lstrcpyn(TempBuff, &pchReceive[7], lstrlen(pchReceive) + 1);
            if ((hFile = CreateFile(TempBuff, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
            {
                if (DeleteFile(TempBuff))
                {
                    WriteToPipes(hPipe, (char*)"Delete File Success !");
                }
                else
                {
                    sprintf(WriteBuff, "Delete File Error: %d", GetLastError());
                    WriteToPipes(hPipe, WriteBuff);
                }
            }
            else
            {
                sprintf(WriteBuff, "Delete File Error: %d", GetLastError());
                WriteToPipes(hPipe, WriteBuff);
            }
            CloseHandle(hFile);
        }
    }
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    HeapFree(hHeap, 0, pchReceive);
    return 0;
}
int WriteToPipes(HANDLE hPipe, char* WriteBuff)
{
    HANDLE hFile;
    BOOL fSuccess = FALSE;
    DWORD cbToWrite = 0, cbWritten = 0;
    char Err[512] = { 0 };
    //ZeroMemory(WriteBuff, sizeof(WriteBuff));
    cbToWrite = lstrlen(WriteBuff);
    fSuccess = WriteFile(
        hPipe,                  // pipe handle 
        WriteBuff,             // message 
        cbToWrite,              // message length 
        &cbWritten,             // bytes written 
        NULL);                  // not overlapped 

    if (!fSuccess)
    {
        sprintf(Err, "WriteFile to pipe failed. GLE=%d\n", GetLastError());
        WriteToLog(Err);
        return -1;
    }
    return 0;
}
