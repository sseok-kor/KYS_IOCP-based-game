#pragma once

#include "client_network.h"
#include "User.h"
#include "Monster.h"
#include "Chat.h"

class MainClient 
{
private:
    ClientNetwork m_Network;
    User m_User;
    Monster m_Monster;
    Chat m_Chat;
    ConsoleBuffer* m_ConsoleBuffer;

    static unsigned int WINAPI NetworkThread(void* arg);

public:
    bool Initialize();
    void GameLoop();
    void ProcessUserInput();
    void SendMovePacket(BYTE direction);
    void SendChatMessage();
    ClientNetwork& GetNetwork() { return m_Network; };
};