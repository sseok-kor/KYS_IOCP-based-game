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

// 작업 타입 정의
enum class JobType 
{
    USER_PROCESS,
    MONSTER_PROCESS,
    CHAT_PROCESS
};

// 유저 정보 구조체
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

// 작업 구조체 정의
struct Job 
{
    JobType type;
    DWORD session_id;
    User* p_user;  // User 객체에 대한 포인터
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
    HANDLE h_iocp_;                           // IOCP 핸들
    SOCKET listen_socket_;                    // 리스닝 소켓
    vector<HANDLE> worker_threads_;           // 작업자 스레드 핸들
    queue<Job> user_job_queue_;               // 유저 처리 작업 큐
    queue<Job> monster_job_queue_;            // 몬스터 처리 작업 큐
    queue<Job> chat_job_queue_;               // 채팅 처리 작업 큐
    CRITICAL_SECTION user_queue_cs_;          // 유저 큐 동기화를 위한 크리티컬 섹션
    CRITICAL_SECTION monster_queue_cs_;       // 몬스터 큐 동기화를 위한 크리티컬 섹션
    CRITICAL_SECTION chat_queue_cs_;          // 채팅 큐 동기화를 위한 크리티컬 섹션
    list<User*> user_list_;                    // 유저 리스트
    list<Monster*> monster_list_;              // 몬스터 리스트
    list<int> user_delete_list_;              // 유저 삭제 리스트 (유저 ID)
    list<int> monster_delete_list_;           // 몬스터 삭제 리스트 (몬스터 ID)
    CRITICAL_SECTION user_list_cs_;           // 유저 리스트 동기화를 위한 크리티컬 섹션
    CRITICAL_SECTION monster_list_cs_;        // 몬스터 리스트 동기화를 위한 크리티컬 섹션
    CRITICAL_SECTION user_delete_list_cs_;    // 유저 삭제 리스트 동기화를 위한 크리티컬 섹션
    CRITICAL_SECTION monster_delete_list_cs_; // 몬스터 삭제 리스트 동기화를 위한 크리티컬 섹션
    HANDLE h_accept_thread_;                  // 접속 처리 스레드 핸들
    HANDLE h_user_thread_;                    // 유저 처리 스레드 핸들
    HANDLE h_monster_thread_;                 // 몬스터 처리 스레드 핸들
    HANDLE h_chat_thread_;                    // 채팅 처리 스레드 핸들
    HANDLE h_frame_thread_;                   // 프레임 관리 스레드 핸들
    HANDLE frame_start_event_;                // 프레임 시작 이벤트
    HANDLE frame_end_event_;                  // 프레임 종료 이벤트
    DWORD max_players_;                       // 최대 플레이어 수
    int ring_buffer_size_;                    // 링 버퍼 크기
    int pending_accepts_;
    CRITICAL_SECTION send_cs_;                // 송신 동기화를 위한 크리티컬 섹션
    CRITICAL_SECTION recv_cs_;                // 수신 동기화를 위한 크리티컬 섹션

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

    DWORD active_sessions_;                   // 활성 세션 수
    

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