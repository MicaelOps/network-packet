#include <iostream>
#include <Windows.h>

int main()
{
    std::cout << "Loading driver \n";

    HANDLE handle = CreateFile(L"\\\\.\\wfppacketransmitterlink", GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        std::cout << "unable to load!!";
        return -1;
    }

    UCHAR* buffer[2010];

    DWORD packetsRead = 0;
    if (ReadFile(handle, buffer, sizeof(buffer), &packetsRead, NULL) == FALSE) {
        std::cout << "Fail to read packet from driver";
        return -1;
    }
    for (int i = 0; i < 10; i++) {
        UCHAR* packet = buffer[i * 201];
        std::cout << "Packet " << i << " data: " << packet << " \n";
    }
    CloseHandle(handle);
}
