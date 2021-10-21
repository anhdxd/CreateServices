#undef UNICODE 

#include <Windows.h>
#include <iostream>

#define SVCNAME "SvcAnhdz"

void InstallSvc();
int main()
{
    InstallSvc();
    
}


void InstallSvc()
{
    SC_HANDLE schSCManager, schService; // Handle cua trinh quan ly services va services
    schSCManager = OpenSCManager(0,0, SC_MANAGER_ALL_ACCESS); // mở trình quản lý
    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    schService = CreateService(schSCManager,
        SVCNAME, SVCNAME,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, // SERVICE_DEMAND_START: khi gọi hàm startservice thì chạy svc
        0, 0, 0, 0, 0, 0);
    if (schService == NULL)
    {
        printf("CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return ;
    }
    else printf("Service installed successfully\n");





    return;
}
