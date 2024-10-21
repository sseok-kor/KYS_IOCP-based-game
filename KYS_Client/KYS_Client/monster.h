#pragma once
#include "client.h"

class Monster : public Client
{
private:
    struct st_monster
    {
        int id;
        short x;
        short y;
        BYTE direction;
        BYTE hp;
    };

    list<st_monster*> m_MonsterList;

public:
    virtual ~Monster();
    virtual void Render(ConsoleBuffer* buffer) override;
    virtual void Update() override;
    virtual void PacketProcess(char* packet) override;

    void AddMonster(int id, short x, short y, BYTE direction, BYTE hp);
    void RemoveMonster(int id);
    void UpdateMonsterPosition(int id, short x, short y, BYTE direction);
    void UpdateMonsterHP(int id, BYTE hp);
};