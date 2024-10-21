#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include "client.h"

class ClientNetwork {
private:
    SOCKET m_Socket;
    HANDLE m_hIOCP;
    RingBuffer m_RecvBuffer;
    RingBuffer m_SendBuffer;
    OVERLAPPED m_RecvOverlapped;
    OVERLAPPED m_SendOverlapped;
    Client* m_pClient;

    static unsigned int __stdcall WorkerThread(void* arg);

public:
    ClientNetwork();
    ~ClientNetwork();

    bool Initialize();
    bool Connect(const char* ip, unsigned short port);
    bool Send(const char* data, int length);
    bool Recv();
    void ProcessPacket(Client* client);
    HANDLE GetIOCPHandle() const;
};