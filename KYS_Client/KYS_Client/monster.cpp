#include "monster.h"


Monster::~Monster() 
{
    for (auto& monster : m_MonsterList) 
    {
        delete monster;
    }
}


void Monster::Render(ConsoleBuffer* buffer) {
    for (const auto& monster : m_MonsterList) {
        if (monster->x >= 0 && monster->x < 130 && monster->y >= 0 && monster->y < 70) {
            buffer->SetChar(monster->x, monster->y, L'M', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        }
    }
}

void Monster::Update() 
{
    // 몬스터 업데이트 로직
}

void Monster::PacketProcess(char* packet) 
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packet);
    switch (header->byType) 
    {
    case dfPACKET_SC_CREATE_MONSTER:
    {
        SC_CreateMonster* create_monster = reinterpret_cast<SC_CreateMonster*>(packet);
        AddMonster(create_monster->ID, create_monster->X, create_monster->Y, create_monster->Direction, create_monster->HP);
        break;
    }
    case dfPACKET_SC_DELETE_MONSTER:
    {
        SC_DeleteMonster* delete_monster = reinterpret_cast<SC_DeleteMonster*>(packet);
        RemoveMonster(delete_monster->ID);
        break;
    }
    case dfPACKET_SC_MONSTER_MOVE:
    {
        SC_MonsterMove* monster_move = reinterpret_cast<SC_MonsterMove*>(packet);
        UpdateMonsterPosition(monster_move->ID, monster_move->X, monster_move->Y, monster_move->Direction);
        break;
    }
    case dfPACKET_SC_MONSTER_DAMAGE:
    {
        SC_MonsterDamage* monster_damage = reinterpret_cast<SC_MonsterDamage*>(packet);
        UpdateMonsterHP(monster_damage->damageID, monster_damage->damageHP);
        break;
    }
    default:
        break;
    }
}

void Monster::AddMonster(int id, short x, short y, BYTE direction, BYTE hp) 
{
    st_monster* new_monster = new st_monster{ id, x, y, direction, hp };
    m_MonsterList.push_back(new_monster);
}

void Monster::RemoveMonster(int id) 
{
    for (auto it = m_MonsterList.begin(); it != m_MonsterList.end(); ++it) 
    {
        if ((*it)->id == id) 
        {
            delete* it;
            m_MonsterList.erase(it);
            break;
        }
    }
}

void Monster::UpdateMonsterPosition(int id, short x, short y, BYTE direction) 
{
    for (auto& monster : m_MonsterList) 
    {
        if (monster->id == id) 
        {
            monster->x = x;
            monster->y = y;
            monster->direction = direction;
            break;
        }
    }
}

void Monster::UpdateMonsterHP(int id, BYTE hp) 
{
    for (auto& monster : m_MonsterList) 
    {
        if (monster->id == id) 
        {
            monster->hp = hp;
            break;
        }
    }
}