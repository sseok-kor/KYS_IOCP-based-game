#include "main_client.h"
#include <conio.h>
#include <process.h>
#include "protocol.h"

unsigned int WINAPI MainClient::NetworkThread(void* arg) 
{
    MainClient* client = reinterpret_cast<MainClient*>(arg);
    while (true) {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        OVERLAPPED* overlapped;

        if (GetQueuedCompletionStatus(client->m_Network.GetIOCPHandle(), &bytesTransferred, &completionKey, &overlapped, INFINITE)) 
        {
            if (bytesTransferred == 0) 
            {
                // 연결 종료 처리
                break;
            }

            // 패킷 처리
            client->m_Network.ProcessPacket(&client->m_User);
            client->m_Network.ProcessPacket(&client->m_Monster);
            client->m_Network.ProcessPacket(&client->m_Chat);

            // 다음 수신 대기
            client->m_Network.Recv();
        }
    }
    return 0;
}

bool MainClient::Initialize() 
{
    if (!m_Network.Initialize()) 
    {
        return false;
    }

    m_ConsoleBuffer = new ConsoleBuffer(171, 70); 

    unsigned int threadID;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, &threadID);
    if (hThread == NULL) 
    {
        return false;
    }
    CloseHandle(hThread);

    return true;
}

void MainClient::GameLoop() 
{
    while (true) 
    {
        // 업데이트
        m_User.Update();
        m_Monster.Update();
        m_Chat.Update();

        // 렌더링
        m_ConsoleBuffer->Clear();
        m_ConsoleBuffer->DrawBorder();

        m_User.Render(m_ConsoleBuffer);
        m_Monster.Render(m_ConsoleBuffer);
        m_Chat.Render(m_ConsoleBuffer);

        m_ConsoleBuffer->Render();

        // 사용자 입력 처리
        ProcessUserInput();

        Sleep(16); // 약 60 FPS
    }
}

void MainClient::ProcessUserInput() 
{
    if (_kbhit()) 
    {
        int key = _getch();
        switch (key) {
        case 'w':
        case 'W':
            SendMovePacket(dfPACKET_MOVE_DIR_UU);
            break;
        case 's':
        case 'S':
            SendMovePacket(dfPACKET_MOVE_DIR_DD);
            break;
        case 'a':
        case 'A':
            SendMovePacket(dfPACKET_MOVE_DIR_LL);
            break;
        case 'd':
        case 'D':
            SendMovePacket(dfPACKET_MOVE_DIR_RR);
            break;
        case 'c':
        case 'C':
            SendChatMessage();
            break;
        case 'q':
        case 'Q':
            // 게임 종료
            exit(0);
            break;
        }
    }
}

void MainClient::SendMovePacket(BYTE direction) 
{
    CS_MoveStart move_packet;
    move_packet.header.byCode = dfPACKET_CODE;
    move_packet.header.bySize = sizeof(CS_MoveStart);
    move_packet.header.byType = dfPACKET_CS_MOVE_START;
    move_packet.Direction = direction;
    move_packet.X = 0; // 서버에서 계산
    move_packet.Y = 0; // 서버에서 계산
    m_Network.Send(reinterpret_cast<char*>(&move_packet), sizeof(CS_MoveStart));
}

void MainClient::SendChatMessage() 
{
    WCHAR chatMsg[32];
    wprintf(L"Enter chat message: ");
    wscanf_s(L"%31s", chatMsg, 32);

    CS_CHAT chat_packet;
    chat_packet.header.byCode = dfPACKET_CODE;
    chat_packet.header.bySize = sizeof(CS_CHAT);
    chat_packet.header.byType = dfPACKET_CS_CHAT;
    chat_packet.ID = 0; // 서버에서 설정
    wcscpy_s(chat_packet.message, chatMsg);
    m_Network.Send(reinterpret_cast<char*>(&chat_packet), sizeof(CS_CHAT));
}