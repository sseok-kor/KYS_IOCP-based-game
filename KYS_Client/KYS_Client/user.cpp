#include "user.h"

 User :: ~User()
{
     for (auto& user : m_UserList) 
     {
         delete user;
     }
}

 void User::Render(ConsoleBuffer* buffer) {
     for (const auto& user : m_UserList) {
         if (user->x >= 0 && user->x < 130 && user->y >= 0 && user->y < 70) {
             buffer->SetChar(user->x, user->y, L'U', FOREGROUND_RED | FOREGROUND_INTENSITY);
         }
     }
 }

 void User::Update()
 {
     //
 }

 void User::PacketProcess(char* packet)
 {
     PacketHeader* header = reinterpret_cast<PacketHeader*>(packet);
     switch (header->byType) 
     {
     case dfPACKET_SC_CREATE_MY_CHARACTER:
     {
         SC_CreateMyCharacter* my_char = reinterpret_cast<SC_CreateMyCharacter*>(packet);
         AddUser(my_char->ID, my_char->X, my_char->Y, my_char->Direction, my_char->HP);
         break;
     }
     case dfPACKET_SC_CREATE_OTHER_CHARACTER:
     {
         SC_CreateOtherCharacter* other_char = reinterpret_cast<SC_CreateOtherCharacter*>(packet);
         AddUser(other_char->ID, other_char->X, other_char->Y, other_char->Direction, other_char->HP);
         break;
     }
     case dfPACKET_SC_DELETE_CHARACTER:
     {
         SC_DeleteCharacter* del_char = reinterpret_cast<SC_DeleteCharacter*>(packet);
         RemoveUser(del_char->ID);
         break;
     }
     case dfPACKET_SC_MOVE_START:
     {
         SC_MoveStart* move_start = reinterpret_cast<SC_MoveStart*>(packet);
         UpdateUserPosition(move_start->ID, move_start->X, move_start->Y, move_start->Direction);
         break;
     }
     case dfPACKET_SC_MOVE_STOP:
     {
         SC_MoveStop* move_stop = reinterpret_cast<SC_MoveStop*>(packet);
         UpdateUserPosition(move_stop->ID, move_stop->X, move_stop->Y, move_stop->Direction);
         break;
     }
     case dfPACKET_SC_ATTACK1:
     {
         SC_Attack1* attack = reinterpret_cast<SC_Attack1*>(packet);
         break;
     }
     case dfPACKET_SC_CHARACTER_DAMAGE:
     {
         SC_CharacterDamage* damage = reinterpret_cast<SC_CharacterDamage*>(packet);
         UpdateUserHP(damage->damageID, damage->damageHP);
         break;
     }
     case dfPACKET_SC_SYNC:
     {
         SC_SYNC* sync = reinterpret_cast<SC_SYNC*>(packet);
         UpdateUserPosition(sync->ID, sync->X,sync->Y,dfPACKET_MOVE_DIR_DD);
         break;
     }
     case dfPACKET_SC_HEARTBEAT:
     {
         //하트비트
         break;
     }
     default:
         break;
     }
 }

 void User::AddUser(int id, short x, short y, BYTE direction, BYTE hp) 
 {
     st_user* new_user = new st_user{ id, x, y, direction, hp };
     m_UserList.push_back(new_user);
 }

 void User::RemoveUser(int id) 
 {
     for (auto it = m_UserList.begin(); it != m_UserList.end(); ++it) 
     {
         if ((*it)->id == id) 
         {
             delete* it;
             m_UserList.erase(it);
             break;
         }
     }
 }

 void User::UpdateUserPosition(int id, short x, short y, BYTE direction) 
 {
     for (auto& user : m_UserList) 
     {
         if (user->id == id) 
         {
             user->x = x;
             user->y = y;
             user->direction = direction;
         }
     }
 }

 void User::UpdateUserHP(int id, BYTE hp) 
 {
     for (auto& user : m_UserList) 
     {
         if (user->id == id) 
         {
             user->hp = hp;
             break;
         }
     }
 }