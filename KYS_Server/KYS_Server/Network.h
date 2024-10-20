#pragma once

#include <vector>
#include <queue>
#include <list>
#include <WinSock2.h>
#include <MSWSock.h>
#include <Windows.h>
#include <process.h>
#include <random>
#include "ringbuffer.h"
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

using namespace std;

#define MAX_BUFFER 1024
#define MAX_WORKER_THREAD 4
#define SERVER_PORT 8000
#define FRAME_RATE 60
#define MAX_ACCEPT_CALLS 10

// �۾� Ÿ�� ����
enum class JobType 
{
    USER_PROCESS,
    MONSTER_PROCESS,
    CHAT_PROCESS
};

// ���� ���� ����ü
struct User
{
    SOCKET socket;
    DWORD session_id;
    RingBuffer* p_recv_buffer;
    RingBuffer* p_send_buffer;
    OVERLAPPED recv_overlapped;
    OVERLAPPED send_overlapped;
    bool is_sending;

    bool is_move;
    short x;
    short y;
    BYTE hp;
    BYTE direction;
    int time;
};

// �۾� ����ü ����
struct Job 
{
    JobType type;
    DWORD session_id;
    User* p_user;  // User ��ü�� ���� ������
    BYTE packet_type;
    char data[dfPACKET_MAX_SIZE];
};



struct Monster
{
    DWORD id;
    short x;
    short y;
    BYTE hp;
    BYTE direction;
};

class IocpServer 
{
private:
    HANDLE h_iocp_;                           // IOCP �ڵ�
    SOCKET listen_socket_;                    // ������ ����
    vector<HANDLE> worker_threads_;           // �۾��� ������ �ڵ�
    queue<Job> user_job_queue_;               // ���� ó�� �۾� ť
    queue<Job> monster_job_queue_;            // ���� ó�� �۾� ť
    queue<Job> chat_job_queue_;               // ä�� ó�� �۾� ť
    CRITICAL_SECTION user_queue_cs_;          // ���� ť ����ȭ�� ���� ũ��Ƽ�� ����
    CRITICAL_SECTION monster_queue_cs_;       // ���� ť ����ȭ�� ���� ũ��Ƽ�� ����
    CRITICAL_SECTION chat_queue_cs_;          // ä�� ť ����ȭ�� ���� ũ��Ƽ�� ����
    list<User*> user_list_;                    // ���� ����Ʈ
    list<Monster*> monster_list_;              // ���� ����Ʈ
    list<int> user_delete_list_;              // ���� ���� ����Ʈ (���� ID)
    list<int> monster_delete_list_;           // ���� ���� ����Ʈ (���� ID)
    CRITICAL_SECTION user_list_cs_;           // ���� ����Ʈ ����ȭ�� ���� ũ��Ƽ�� ����
    CRITICAL_SECTION monster_list_cs_;        // ���� ����Ʈ ����ȭ�� ���� ũ��Ƽ�� ����
    CRITICAL_SECTION user_delete_list_cs_;    // ���� ���� ����Ʈ ����ȭ�� ���� ũ��Ƽ�� ����
    CRITICAL_SECTION monster_delete_list_cs_; // ���� ���� ����Ʈ ����ȭ�� ���� ũ��Ƽ�� ����
    HANDLE h_accept_thread_;                  // ���� ó�� ������ �ڵ�
    HANDLE h_user_thread_;                    // ���� ó�� ������ �ڵ�
    HANDLE h_monster_thread_;                 // ���� ó�� ������ �ڵ�
    HANDLE h_chat_thread_;                    // ä�� ó�� ������ �ڵ�
    HANDLE h_frame_thread_;                   // ������ ���� ������ �ڵ�
    HANDLE frame_start_event_;                // ������ ���� �̺�Ʈ
    HANDLE frame_end_event_;                  // ������ ���� �̺�Ʈ
    DWORD max_players_;                       // �ִ� �÷��̾� ��
    int ring_buffer_size_;                    // �� ���� ũ��
    int pending_accepts_;
    CRITICAL_SECTION send_cs_;                // �۽� ����ȭ�� ���� ũ��Ƽ�� ����
    CRITICAL_SECTION recv_cs_;                // ���� ����ȭ�� ���� ũ��Ƽ�� ����

    struct AcceptOverlapped
    {
        OVERLAPPED overlapped;
        SOCKET accept_socket;
    };

public:
    IocpServer();
    ~IocpServer();
    bool Initialize();
    void StartProcess();
    void EnqueueJob(const Job job);

    DWORD active_sessions_;                   // Ȱ�� ���� ��
    

private:
    static unsigned __stdcall WorkerThread(void* arg);
    void ProcessReceivedData(User* user, DWORD bytes_transferred);
    void PostAccept(); 
    void PostRecv(User* user);
    void PostSend(User* user);
    bool SendPacket(DWORD session_id, char* packet, int packet_size);
    void BroadcastPacket(char* packet, int packet_size);
    void ExceptOneBroadcastPacket(char* packet, int packet_size, DWORD except_id);

    static unsigned __stdcall AcceptThread(void* arg);
    void ProcessAccept(LPOVERLAPPED p_overlapped);
    DWORD GenerateSessionId();
    void AddUser(User* user);
    void DeleteUser(DWORD session_id);
    void RemoveUser();

    static unsigned __stdcall UserProcessThread(void* arg);
    User* GetUserBySessionId(DWORD session_id);
    void DequeueJobUser();
    void ProcessMoveStart(User* user, CS_MoveStart* packet);
    void ProcessMoveStop(User* user, CS_MoveStop* packet); 
    void ProcessUserMove();
    void ProcessAttack1(User* user, CS_Attack1* packet); 
    void CheckUserTimeout();
    void ProcessHeartbeat(User* user, CS_HEARTBEAT* packet);
    void SendSyncPacket(User* user);
    void SendCreateMyCharacter(User* user);
    void SendCreateOtherCharacter(User* new_user);
    void SendExistingUsersInfo(User* new_user);

    

    static unsigned __stdcall MonsterProcessThread(void* arg);
    //void DequeueJobMonster();
    void AddMonster();
    DWORD GenerateMonsterSessionId();
    void ProcessMonsterAI();
    Monster* GetMonsterById(DWORD monster_id);
    void UpdateMonsterPosition(Monster* monster);
    void UpdateMonsterDirection(Monster* monster);
    void BroadcastMonsterMove(Monster* monster);
    void RemoveMonster();

    static unsigned __stdcall ChatProcessThread(void* arg);
    void DequeueJobChat();

    static unsigned __stdcall FrameThread(void* arg);
    
    void ProcessDeleteLists();
    int GetRandomNumber(int min, int max);
};