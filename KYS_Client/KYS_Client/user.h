#pragma once

#include"client.h"

class User : public Client
{
private:
    struct st_user 
    {
        int id;
        short x;
        short y;
        BYTE direction;
        BYTE hp;
    };
    list<st_user*> m_UserList;

public:
    virtual ~User();
    virtual void Render(ConsoleBuffer* buffer) override;
    virtual void Update() override;
    virtual void PacketProcess(char* packet) override;
    void AddUser(int id, short x, short y, BYTE direction, BYTE hp);
    void RemoveUser(int id);
    void UpdateUserPosition(int id, short x, short y, BYTE direction);
    void UpdateUserHP(int id, BYTE hp);
};