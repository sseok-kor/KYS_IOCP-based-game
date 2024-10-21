#include "chat.h"


void Chat::Render(ConsoleBuffer* buffer) {
    buffer->WriteString(131, 0, L"Chat", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    int y = 1;
    for (const auto& msg : m_ChatLog) {
        std::wstring displayMsg = L"[" + std::to_wstring(msg.id) + L"] " + msg.message;
        if (displayMsg.length() > 18) {
            displayMsg = displayMsg.substr(0, 15) + L"...";
        }
        buffer->WriteString(131, y++, displayMsg);
        if (y >= 70) break;
    }
}

void Chat::Update() 
{
    // 채팅 업데이트 로직
    while (m_ChatLog.size() > 100) 
    {
        m_ChatLog.pop_front();
    }
}

void Chat::PacketProcess(char* packet)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packet);
    switch (header->byType)  
    {
    case dfPACKET_SC_CHAT: 
    {
        SC_CHAT* chat_packet = reinterpret_cast<SC_CHAT*>(packet); 
        ChatMessage newMsg; 
        newMsg.id = chat_packet->ID; 
        newMsg.message = chat_packet->message; 
        m_ChatLog.push_back(newMsg); 
        break;
    }
    default:
        break;
    }
}

void Chat::AddChatMessage(int id, const WCHAR* msg) 
{
    ChatMessage newMsg;
    newMsg.id = id;
    newMsg.message = msg;
    m_ChatLog.push_back(newMsg);
}