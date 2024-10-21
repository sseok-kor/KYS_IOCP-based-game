#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "client_network.h"
#include <process.h>
#include <WS2tcpip.h>

ClientNetwork::ClientNetwork() : m_Socket(INVALID_SOCKET), m_hIOCP(NULL), m_RecvBuffer(8192), m_SendBuffer(8192) {}

ClientNetwork::~ClientNetwork() {
    if (m_Socket != INVALID_SOCKET) {
        closesocket(m_Socket);
    }
    if (m_hIOCP != NULL) {
        CloseHandle(m_hIOCP);
    }
    WSACleanup();
}

bool ClientNetwork::Initialize() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
        return false;
    }

    m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (m_Socket == INVALID_SOCKET) 
    {
        return false;
    }

    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (m_hIOCP == NULL) 
    {
        return false;
    }

    if (CreateIoCompletionPort((HANDLE)m_Socket, m_hIOCP, 0, 0) == NULL) 
    {
        return false;
    }

    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);
    if (hThread == NULL) {
        return false;
    }
    CloseHandle(hThread);

    return true;
}

bool ClientNetwork::Connect(const char* ip, unsigned short port) {
    sockaddr_in serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    if (connect(m_Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        printf("5\n");
        return false;
    }

    return true;
}

bool ClientNetwork::Send(const char* data, int length) {
    if (length > 0) {
        if (m_SendBuffer.GetFreeSize() < length) {
            return false;
        }
        m_SendBuffer.Enqueue(data, length);
    }

    if (m_SendBuffer.GetUseSize() == 0) {
        return true;  // 보낼 데이터가 없음
    }

    WSABUF wsaBuf;
    wsaBuf.buf = m_SendBuffer.GetFrontBufferPtr();
    wsaBuf.len = m_SendBuffer.GetUseSize();

    DWORD sendBytes;
    int result = WSASend(m_Socket, &wsaBuf, 1, &sendBytes, 0, &m_SendOverlapped, NULL);
    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        return false;
    }
    return true;
}

bool ClientNetwork::Recv() {
    WSABUF wsaBuf;
    wsaBuf.buf = m_RecvBuffer.GetFrontBufferPtr();
    wsaBuf.len = m_RecvBuffer.DirectEnqueueSize();

    DWORD recvBytes;
    DWORD flags = 0;
    OVERLAPPED* overlapped = new OVERLAPPED();
    ZeroMemory(overlapped, sizeof(OVERLAPPED));

    if (WSARecv(m_Socket, &wsaBuf, 1, &recvBytes, &flags, overlapped, NULL) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            delete overlapped;
            return false;
        }
    }

    return true;
}

void ClientNetwork::ProcessPacket(Client* client) {
    while (m_RecvBuffer.GetUseSize() >= sizeof(PacketHeader)) {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(m_RecvBuffer.GetRearBufferPtr());
        if (m_RecvBuffer.GetUseSize() >= header->bySize) {
            char* packet = new char[header->bySize];
            m_RecvBuffer.Dequeue(packet, header->bySize);
            client->PacketProcess(packet);
            delete[] packet;
        }
        else {
            break;
        }
    }
}

HANDLE ClientNetwork::GetIOCPHandle() const 
{
    return m_hIOCP;
}

unsigned __stdcall ClientNetwork::WorkerThread(void* arg) 
{
    ClientNetwork* network = static_cast<ClientNetwork*>(arg);
    while (true) {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        OVERLAPPED* overlapped;

        BOOL result = GetQueuedCompletionStatus(network->m_hIOCP, &bytesTransferred, &completionKey, &overlapped, INFINITE);

        if (!result) {
            // 에러 처리
            continue;
        }

        if (bytesTransferred == 0) {
            // 연결 종료 처리
            break;
        }

        if (overlapped == &network->m_RecvOverlapped) {
            // 수신 처리
            network->m_RecvBuffer.MoveRear(bytesTransferred);
            network->ProcessPacket(network->m_pClient);
            network->Recv();  // 다음 수신을 위해 다시 호출
        }
        else if (overlapped == &network->m_SendOverlapped) {
            // 송신 완료 처리
            network->m_SendBuffer.MoveFront(bytesTransferred);
            if (network->m_SendBuffer.GetUseSize() > 0) {
                network->Send(nullptr, 0);  // 남은 데이터가 있으면 다시 전송
            }
        }
    }
    return 0;
}