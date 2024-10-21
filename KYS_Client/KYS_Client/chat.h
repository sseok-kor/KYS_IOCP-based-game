#pragma once
#include "client.h"

class Chat : public Client
{
private:
    struct ChatMessage 
    {
        int id;
        wstring message;
    };

    list<ChatMessage> m_ChatLog;

public:
    virtual ~Chat() {};
    virtual void Render(ConsoleBuffer* buffer) override;
    virtual void Update() override;
    virtual void PacketProcess(char* packet) override;

    void AddChatMessage(int id, const WCHAR* msg);
};
