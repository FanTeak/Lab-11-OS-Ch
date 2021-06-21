#include "resourses.h"

using namespace std;
static int connectionNum = 0;

unsigned int __stdcall startServer(void*);

int main(void) {
    NewLog(false, vector<string>{ ""});
    NewLog(false, vector<string>{ "================================================================"});
    NewLog(false, vector<string>{ ""});
    NewLog(true, vector<string>{ "SERVER", "Starting..."});
    HANDLE main = (HANDLE)_beginthreadex(0, 0, startServer, NULL, 0, 0);
    if (!main) {
        NewLog(true, vector<string>{ "ERROR", "Unable to create main thread"});
        return 1;
    }
    cout << "Press ESCAPE to terminate program\r\n";
    while (_getch() != 27);
    CloseHandle(main);
    WSACleanup();
    return 0;
}

unsigned int __stdcall startServer(void*) {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        NewLog(true, vector<string>{ "ERROR", "WSAStartup failed", to_string(iResult) });
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        NewLog(true, vector<string>{ "ERROR", "getaddrinfo failed", to_string(iResult) });
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        NewLog(true, vector<string>{ "ERROR", "socket failed", to_string(iResult) });
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        NewLog(true, vector<string>{ "ERROR", "bind failed", to_string(iResult) });
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        NewLog(true, vector<string>{ "ERROR", "listen failed", to_string(iResult) });
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    extern HANDLE mutx;
    mutx = CreateMutex(0, FALSE, 0);
    NewLog(true, vector<string>{ "SERVER", "Ready for work", "Wait for connections..." });
    // Accept a client socket
    while (true) {
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            NewLog(true, vector<string>{ "ERROR", "accept failed", to_string(iResult) });
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        _beginthreadex(0, 0, ClientTread, (void*)(ClientSocket), 0, 0);
        //ClientTread((void*)ClientSocket);
    }

    // No longer need server socket
    CloseHandle(mutx);
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}