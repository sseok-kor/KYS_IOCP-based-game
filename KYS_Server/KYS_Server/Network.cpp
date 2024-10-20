#include "Network.h"

using namespace std;

int IocpServer:: GetRandomNumber(int min, int max)
{
    static unsigned int seed = GetTickCount();
    seed = seed * 1103515245 + 12345;
    return min + (seed % (max - min + 1));
}

IocpServer::IocpServer()
    : h_iocp_(NULL), listen_socket_(INVALID_SOCKET), max_players_(1000), active_sessions_(0),
    h_accept_thread_(NULL), h_user_thread_(NULL), h_monster_thread_(NULL), h_chat_thread_(NULL), h_frame_thread_(NULL),
    frame_start_event_(NULL), frame_end_event_(NULL), ring_buffer_size_(4096), pending_accepts_(0)
{
    // 각종 크리티컬 섹션 초기화
    InitializeCriticalSection(&user_list_cs_);
    InitializeCriticalSection(&monster_list_cs_);
    InitializeCriticalSection(&user_delete_list_cs_);
    InitializeCriticalSection(&monster_delete_list_cs_);
    InitializeCriticalSection(&user_queue_cs_);
    InitializeCriticalSection(&monster_queue_cs_);
    InitializeCriticalSection(&chat_queue_cs_);
    InitializeCriticalSection(&send_cs_);
    InitializeCriticalSection(&recv_cs_);
}

IocpServer::~IocpServer()
{
    // 크리티컬 섹션 삭제
    DeleteCriticalSection(&user_list_cs_);
    DeleteCriticalSection(&monster_list_cs_);
    DeleteCriticalSection(&user_delete_list_cs_);
    DeleteCriticalSection(&monster_delete_list_cs_);
    DeleteCriticalSection(&user_queue_cs_);
    DeleteCriticalSection(&monster_queue_cs_);
    DeleteCriticalSection(&chat_queue_cs_);
    DeleteCriticalSection(&send_cs_);
    DeleteCriticalSection(&recv_cs_);

    // 소켓 닫기
    if (listen_socket_ != INVALID_SOCKET)
        closesocket(listen_socket_);

    // IOCP 핸들 닫기
    if (h_iocp_)
        CloseHandle(h_iocp_);

    // 이벤트 핸들 닫기
    if (frame_start_event_)
        CloseHandle(frame_start_event_);
    if (frame_end_event_)
        CloseHandle(frame_end_event_);

    // 윈속 정리
    WSACleanup();
}

bool IocpServer::Initialize()
{
    // 윈속 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed - Error Code : %d\n", WSAGetLastError());
        return false;
    }

    // IOCP 생성
    h_iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (h_iocp_ == NULL)
    {
        printf("CreateIoCompletionPort failed - Error Code : %d\n", GetLastError());
        return false;
    }

    // 리스닝 소켓 생성
    listen_socket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_socket_ == INVALID_SOCKET)
    {
        printf("WSASocket failed - Error Code : %d\n", WSAGetLastError());
        return false;
    }

    // 서버 주소 설정
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVER_PORT);

    // 소켓 바인딩
    if (bind(listen_socket_, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        printf("bind failed - Error Code : %d\n", WSAGetLastError());
        closesocket(listen_socket_);
        return false;
    }

    // AcceptEx 옵션
    BOOL bConditionalAccept = TRUE;
    setsockopt(listen_socket_, SOL_SOCKET, SO_CONDITIONAL_ACCEPT,
        (char*)&bConditionalAccept, sizeof(bConditionalAccept));

    // 리스닝 시작
    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("listen failed - Error Code : %d\n", WSAGetLastError());
        closesocket(listen_socket_);
        return false;
    }

    // 프레임 동기화 이벤트 생성
    frame_start_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    frame_end_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);

    printf("Server initialized successfully. Listening on port %d\n", SERVER_PORT);
    return true;
}

void IocpServer::StartProcess()
{
    // 작업자 스레드 생성
    for (int i = 0; i < MAX_WORKER_THREAD; ++i)
    {
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);
        worker_threads_.push_back(hThread);
    }

    // Accept 스레드 생성
    h_accept_thread_ = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);

    // 게임 로직 스레드 생성
    h_user_thread_ = (HANDLE)_beginthreadex(NULL, 0, UserProcessThread, this, 0, NULL);
    h_monster_thread_ = (HANDLE)_beginthreadex(NULL, 0, MonsterProcessThread, this, 0, NULL);
    h_chat_thread_ = (HANDLE)_beginthreadex(NULL, 0, ChatProcessThread, this, 0, NULL);

    // 프레임 관리 스레드 생성
    h_frame_thread_ = (HANDLE)_beginthreadex(NULL, 0, FrameThread, this, 0, NULL);

    printf("All threads started successfully.\n");
}

void IocpServer::EnqueueJob(const Job job)
{
    // 작업 타입에 따라 적절한 큐에 작업 추가
    switch (job.type)
    {
    case JobType::USER_PROCESS:
        EnterCriticalSection(&user_queue_cs_);
        user_job_queue_.push(job);
        LeaveCriticalSection(&user_queue_cs_);
        break;
    case JobType::MONSTER_PROCESS:
        EnterCriticalSection(&monster_queue_cs_);
        monster_job_queue_.push(job);
        LeaveCriticalSection(&monster_queue_cs_);
        break;
    case JobType::CHAT_PROCESS:
        EnterCriticalSection(&chat_queue_cs_);
        chat_job_queue_.push(job);
        LeaveCriticalSection(&chat_queue_cs_);
        break;
    default:
        printf("Unknown job type\n");
        break;
    }
}


unsigned __stdcall IocpServer::WorkerThread(void* arg)
{
    IocpServer* server = static_cast<IocpServer*>(arg);

    while (true)
    {
        DWORD bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        LPOVERLAPPED p_overlapped = NULL;

        BOOL result = GetQueuedCompletionStatus(server->h_iocp_, &bytes_transferred,
            &completion_key, &p_overlapped, INFINITE);

        if (!result)
        {
            if (p_overlapped == NULL)
            {
                // 실제 오류 상황일 때만 오류 메시지 출력
                DWORD error = GetLastError();
                if (error != WAIT_TIMEOUT)
                {
                    printf("GetQueuedCompletionStatus failed - Error Code : %d\n", error);
                }
                continue;
            }

            // 클라이언트 연결 종료 처리
            DWORD session_id = static_cast<DWORD>(completion_key);
            server->DeleteUser(session_id);
            continue;
        }

        if (bytes_transferred == 0 && completion_key == 0 && p_overlapped == NULL)
        {
            break;
        }

        // Accept 완료 처리
        if (completion_key == 0 && p_overlapped != NULL)
        {
            server->ProcessAccept(p_overlapped);
            continue;
        }

        DWORD session_id = static_cast<DWORD>(completion_key);
        User* user = server->GetUserBySessionId(session_id);
        if (user == nullptr)
        {
            printf("Invalid user session\n");
            continue;
        }

        if (p_overlapped == &user->recv_overlapped)
        {
            server->ProcessReceivedData(user, bytes_transferred);
        }
        else if (p_overlapped == &user->send_overlapped)
        {
            EnterCriticalSection(&server->send_cs_);
            user->p_send_buffer->MoveFront(bytes_transferred);
            if (user->p_send_buffer->GetUseSize() > 0)
            {
                server->PostSend(user);
            }
            else
            {
                user->is_sending = false;
            }
            LeaveCriticalSection(&server->send_cs_);
        }
    }
    return 0;
}

void IocpServer::ProcessReceivedData(User* user, DWORD bytes_transferred)
{
    user->p_recv_buffer->MoveRear(bytes_transferred); 
    char* tempDelete = nullptr;
    while (true)
    {
        
        if (user->p_recv_buffer->GetUseSize() < sizeof(PacketHeader))
            break;


        char* packet_data = new char[bytes_transferred];
        user->p_recv_buffer->Dequeue(packet_data, bytes_transferred);

        if (packet_data[0] != 0x96) // 패킷 코드 확인
        {
            delete[] packet_data;
            break; 
        }
            
        tempDelete = packet_data; 

        Job job;
        job.session_id = user->session_id;
        job.p_user = user;
        job.packet_type = packet_data[2];
        memcpy_s(job.data, dfPACKET_MAX_SIZE, packet_data, bytes_transferred); 


        switch (job.packet_type) // 패킷 타입 확인
        {
        case dfPACKET_CS_MOVE_START:
            job.type = JobType::USER_PROCESS;    
            EnqueueJob(job);
            break;
        case dfPACKET_CS_MOVE_STOP:
            job.type = JobType::USER_PROCESS;
            EnqueueJob(job);
            break;
        case dfPACKET_CS_ATTACK1:
            job.type = JobType::USER_PROCESS;
            EnqueueJob(job);
            break;
        case dfPACKET_CS_HEARTBEAT:
            ProcessHeartbeat(user, (CS_HEARTBEAT*)packet_data);
            break;
        case dfPACKET_CS_CHAT:
            job.type = JobType::CHAT_PROCESS; 
            EnqueueJob(job);
        default:
            printf("Unknown packet type\n");
            break;
        }

        
    }
    delete[] tempDelete;

    // 다음 수신을 위해 WSARecv 재호출
    PostRecv(user);
}

void IocpServer::PostAccept()
{
    if (pending_accepts_ >= MAX_ACCEPT_CALLS)
    {
        return;  // 최대 AcceptEx 호출 수에 도달
    }

    SOCKET accept_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (accept_socket == INVALID_SOCKET)
    {
        printf("WSASocket failed - Error Code : %d\n", WSAGetLastError());
        return;
    }

    AcceptOverlapped* accept_overlapped = new AcceptOverlapped();
    ZeroMemory(&accept_overlapped->overlapped, sizeof(OVERLAPPED));
    accept_overlapped->accept_socket = accept_socket;

    DWORD bytes_received = 0;
    char accept_buffer[(sizeof(SOCKADDR_IN) + 16) * 2];

    if (!AcceptEx(listen_socket_, accept_socket, accept_buffer, 0,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
        &bytes_received, &accept_overlapped->overlapped))
    {
        if (WSAGetLastError() != ERROR_IO_PENDING)
        {
            printf("AcceptEx failed - Error Code : %d\n", WSAGetLastError());
            closesocket(accept_socket);
            delete accept_overlapped;
            return;
        }
    }

    pending_accepts_++;
}

void IocpServer::PostRecv(User* user)
{
    DWORD flags = 0;
    WSABUF wsabuf;
    wsabuf.len = user->p_recv_buffer->GetFreeSize();
    wsabuf.buf = user->p_recv_buffer->GetRearBufferPtr();

    int result = WSARecv(user->socket, &wsabuf, 1, NULL, &flags, &user->recv_overlapped, NULL);
    if (result == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
    {
        printf("WSARecv failed - Error Code : %d\n", WSAGetLastError());
        DeleteUser(user->session_id); 
    }
}

void IocpServer::PostSend(User* user)
{
    if (user->is_sending)
        return;

    EnterCriticalSection(&send_cs_);

    int send_size = user->p_send_buffer->GetUseSize();
    if (send_size == 0)
    {
        LeaveCriticalSection(&send_cs_);
        return;
    }

    user->is_sending = true;
    WSABUF wsabuf;
    wsabuf.len = send_size;
    wsabuf.buf = user->p_send_buffer->GetFrontBufferPtr();

    DWORD bytes_sent = 0;
    int result = WSASend(user->socket, &wsabuf, 1, &bytes_sent, 0, &user->send_overlapped, NULL);

    if (result == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
    {
        printf("WSASend failed - Error Code : %d\n", WSAGetLastError());
        user->is_sending = false;
        DeleteUser(user->session_id); 
    }

    LeaveCriticalSection(&send_cs_);
}

bool IocpServer::SendPacket(DWORD session_id, char* packet, int packet_size)
{
    User* user = GetUserBySessionId(session_id);
    if (user == nullptr)
        return false;

    EnterCriticalSection(&send_cs_);
    int enqueued_size = user->p_send_buffer->Enqueue(packet, packet_size);
    if (enqueued_size != packet_size)
    {
        LeaveCriticalSection(&send_cs_);
        return false;
    }

    if (!user->is_sending)
    {
        PostSend(user);
    }
    LeaveCriticalSection(&send_cs_);
    return true;
}

void IocpServer::BroadcastPacket(char* packet, int packet_size)
{
    EnterCriticalSection(&user_list_cs_);
    for (auto& user : user_list_)
    {
         SendPacket(user->session_id, packet, packet_size);
    }
    LeaveCriticalSection(&user_list_cs_);
}

void IocpServer::ExceptOneBroadcastPacket(char* packet, int packet_size, DWORD except_id)
{
    EnterCriticalSection(&user_list_cs_);
    for (auto& user : user_list_)
    {
        if (user->session_id != except_id)
        {
            SendPacket(user->session_id, packet, packet_size);
        }
    }
    LeaveCriticalSection(&user_list_cs_);
}

unsigned __stdcall IocpServer::AcceptThread(void* arg)
{
    IocpServer* server = static_cast<IocpServer*>(arg);

    while (true)
    {
        server->PostAccept();
    }
    return 0;
}

void IocpServer::ProcessAccept(LPOVERLAPPED p_overlapped)
{
    pending_accepts_--;  // AcceptEx 완료 시 감소

    AcceptOverlapped* accept_overlapped = reinterpret_cast<AcceptOverlapped*>(p_overlapped);
    SOCKET client_socket = accept_overlapped->accept_socket;

    // 소켓 옵션 설정
    setsockopt(client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        (char*)&listen_socket_, sizeof(listen_socket_));

    // 새 User 객체 생성
    User* new_user = new User();
    new_user->socket = client_socket;
    new_user->session_id = GenerateSessionId();
    new_user->p_recv_buffer = new RingBuffer(ring_buffer_size_);
    new_user->p_send_buffer = new RingBuffer(ring_buffer_size_);
    new_user->is_sending = false;
    new_user->is_move = false;
    new_user->x = GetRandomNumber(0, 1000);
    new_user->y = GetRandomNumber(0, 1000);
    new_user->hp = 100;
    new_user->direction = dfPACKET_MOVE_DIR_LL;
    new_user->time = GetTickCount();

    // IOCP에 소켓 등록
    if (CreateIoCompletionPort((HANDLE)client_socket, h_iocp_, (ULONG_PTR)new_user->session_id, 0) == NULL)
    {
        printf("CreateIoCompletionPort failed - Error Code : %d\n", GetLastError());
        delete new_user;
        closesocket(client_socket);
        delete accept_overlapped;
        return;
    }

    // 유저 리스트에 추가
    AddUser(new_user);

    // SC_CreateMyCharacter 패킷 전송
    SendCreateMyCharacter(new_user);

    // 다른 유저들에게 SC_CreateOtherCharacter 패킷 전송
    SendCreateOtherCharacter(new_user);

    // 새 유저에게 기존 유저들의 정보 전송
    SendExistingUsersInfo(new_user);

    // 최초 Recv 요청
    PostRecv(new_user);

    // AcceptOverlapped 객체 삭제
    delete accept_overlapped;

    // 다음 Accept 요청
    PostAccept();
}

DWORD IocpServer::GenerateSessionId()
{
    static DWORD nextUserSessionId = 1;
    return InterlockedIncrement(&nextUserSessionId);
}

void IocpServer::AddUser(User* new_user)
{
    // 유저 리스트에 새 유저 추가
    EnterCriticalSection(&user_list_cs_);
    user_list_.push_back(new_user);
    active_sessions_++;
    LeaveCriticalSection(&user_list_cs_);

    //// SC_CreateMyCharacter 패킷 전송
    //SC_CreateMyCharacter create_my_char_packet;
    //create_my_char_packet.header.byCode = dfPACKET_CODE;
    //create_my_char_packet.header.bySize = sizeof(SC_CreateMyCharacter) - sizeof(PacketHeader);
    //create_my_char_packet.header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
    //create_my_char_packet.ID = new_user->session_id;
    //create_my_char_packet.Direction = new_user->direction;
    //create_my_char_packet.X = new_user->x;
    //create_my_char_packet.Y = new_user->y;
    //create_my_char_packet.HP = new_user->hp;
    //SendPacket(new_user->session_id, (char*)&create_my_char_packet, sizeof(SC_CreateMyCharacter));

    //// 다른 유저들에게 SC_CreateOtherCharacter 패킷 전송
    //SC_CreateOtherCharacter create_other_char_packet;
    //create_other_char_packet.header.byCode = dfPACKET_CODE;
    //create_other_char_packet.header.bySize = sizeof(SC_CreateOtherCharacter) - sizeof(PacketHeader);
    //create_other_char_packet.header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
    //create_other_char_packet.ID = new_user->session_id;
    //create_other_char_packet.Direction = new_user->direction;
    //create_other_char_packet.X = new_user->x;
    //create_other_char_packet.Y = new_user->y;
    //create_other_char_packet.HP = new_user->hp;

    //EnterCriticalSection(&user_list_cs_);
    //for (auto other_user : user_list_)
    //{
    //    if (other_user->session_id != new_user->session_id)
    //    {
    //        SendPacket(other_user->session_id, (char*)&create_other_char_packet, sizeof(SC_CreateOtherCharacter));

    //        // 새 유저에게 기존 유저 정보 전송
    //        SC_CreateOtherCharacter existing_user_packet;
    //        existing_user_packet.header.byCode = dfPACKET_CODE;
    //        existing_user_packet.header.bySize = sizeof(SC_CreateOtherCharacter) - sizeof(PacketHeader);
    //        existing_user_packet.header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
    //        existing_user_packet.ID = other_user->session_id;
    //        existing_user_packet.Direction = other_user->direction;
    //        existing_user_packet.X = other_user->x;
    //        existing_user_packet.Y = other_user->y;
    //        existing_user_packet.HP = other_user->hp;
    //        SendPacket(new_user->session_id, (char*)&existing_user_packet, sizeof(SC_CreateOtherCharacter));
    //    }
    //}
    //LeaveCriticalSection(&user_list_cs_);
}

void IocpServer::DeleteUser(DWORD session_id)
{
    User* user = GetUserBySessionId(session_id);
    if (user == nullptr)
    {
        return;
    }

    // SC_DeleteCharacter 패킷 생성
    SC_DeleteCharacter packet;
    packet.header.byCode = dfPACKET_CODE;
    packet.header.bySize = sizeof(SC_DeleteCharacter);
    packet.header.byType = dfPACKET_SC_DELETE_CHARACTER;
    packet.ID = session_id;

    // 모든 유저에게 SC_DeleteCharacter 패킷 전송
    BroadcastPacket((char*)&packet, sizeof(SC_DeleteCharacter));

    // user_delete_list_에 삭제할 유저 등록
    EnterCriticalSection(&user_list_cs_);
    user_delete_list_.push_back(session_id);
    LeaveCriticalSection(&user_list_cs_);

    return;

}

void IocpServer::RemoveUser()
{
    // user_delete_list_를 ID 기준으로 정렬
    EnterCriticalSection(&user_delete_list_cs_);
    user_delete_list_.sort();
    LeaveCriticalSection(&user_delete_list_cs_);

    EnterCriticalSection(&user_list_cs_);

    // user_delete_list_가 비어있지 않은 경우에만 처리
    if (!user_delete_list_.empty())
    {
        auto delete_it = user_delete_list_.begin();
        auto user_it = user_list_.begin();

        while (user_it != user_list_.end() && delete_it != user_delete_list_.end())
        {
            if ((*user_it)->session_id == *delete_it)
            {
                User* userToDelete = *user_it; 

                closesocket((*user_it)->socket);

                delete (*user_it)->p_recv_buffer;
                delete (*user_it)->p_send_buffer;

                delete userToDelete; 

                // 유저 리스트에서 해당 유저 제거
                user_it = user_list_.erase(user_it);
                ++delete_it;
                active_sessions_--;
            }
            else if ((*user_it)->session_id < *delete_it)
            {
                ++user_it;
            }
            else
            {
                ++delete_it;
            }
        }

        // 삭제 작업이 완료되었으므로 삭제 리스트를 비움
        EnterCriticalSection(&user_delete_list_cs_);
        user_delete_list_.clear();
        LeaveCriticalSection(&user_delete_list_cs_);

    }

    LeaveCriticalSection(&user_list_cs_);
}

unsigned __stdcall IocpServer::UserProcessThread(void* arg)
{
    IocpServer* server = static_cast<IocpServer*>(arg);
    while (true)
    {
        // 프레임 시작 신호 대기
        WaitForSingleObject(server->frame_start_event_, INFINITE);

        // 유저 작업 큐 처리
        server->DequeueJobUser();

        // 유저 이동 처리
        server->ProcessUserMove();

        // 유저 타임아웃 체크
        server->CheckUserTimeout();

        server->RemoveUser();

        // 프레임 종료 신호
        SetEvent(server->frame_end_event_);
    }
    return 0;
}

User* IocpServer::GetUserBySessionId(DWORD session_id)
{
    EnterCriticalSection(&user_list_cs_);
    for (auto& user : user_list_)
    {
        if (user->session_id == session_id)
        {
            LeaveCriticalSection(&user_list_cs_);
            return user;
        }
    }
    LeaveCriticalSection(&user_list_cs_);
    return nullptr;
}

void IocpServer::DequeueJobUser()
{
    EnterCriticalSection(&user_queue_cs_); 
    while (!user_job_queue_.empty())
    {
        Job job = user_job_queue_.front();
        user_job_queue_.pop();
        LeaveCriticalSection(&user_queue_cs_);

        User* user = GetUserBySessionId(job.session_id);
        // 패킷 타입에 따른 처리
        switch (job.packet_type) 
        {
        case dfPACKET_CS_MOVE_START: 
            ProcessMoveStart(user, (CS_MoveStart*)job.data);
            break;
        case dfPACKET_CS_MOVE_STOP: 
            ProcessMoveStop(user, (CS_MoveStop*)job.data);
            break;
        case dfPACKET_CS_ATTACK1: 
            ProcessAttack1(user, (CS_Attack1*)job.data);
            break;
        case dfPACKET_CS_HEARTBEAT:
            ProcessHeartbeat(user, (CS_HEARTBEAT*)job.data);
            break;
        }
        EnterCriticalSection(&user_queue_cs_); 
    }

    LeaveCriticalSection(&user_queue_cs_); 
    
}


void IocpServer::ProcessMoveStart(User* user, CS_MoveStart* packet)
{
    user->is_move = true;
    user->direction = packet->Direction;

    if (abs(user->x - packet->X >= 10) || abs(user->y - packet->Y) >= 10 )
    {
        user->x = packet->X;
        user->y = packet->Y;
        SendSyncPacket(user);
    }

    // 유저 위치 업데이트
    user->x = packet->X;
    user->y = packet->Y;

    // SC_MoveStart 패킷 생성 및 브로드캐스트
    SC_MoveStart response;
    response.header.byCode = dfPACKET_CODE;
    response.header.bySize = sizeof(SC_MoveStart);
    response.header.byType = dfPACKET_SC_MOVE_START;
    response.ID = user->session_id;
    response.Direction = user->direction;
    response.X = user->x;
    response.Y = user->y;

    ExceptOneBroadcastPacket((char*)&response, sizeof(SC_MoveStart), user->session_id); 
}

void IocpServer::ProcessMoveStop(User* user, CS_MoveStop* packet)
{
    user->is_move = false;
    user->direction = packet->Direction;

    if (abs(user->x - packet->X >= 10) || abs(user->y - packet->Y) >= 10)
    {
        user->x = packet->X;
        user->y = packet->Y;
        SendSyncPacket(user);
    }

    // 유저 위치 업데이트
    user->x = packet->X;
    user->y = packet->Y;

    // SC_MoveStop 패킷 생성 및 브로드캐스트
    SC_MoveStop response;
    response.header.byCode = dfPACKET_CODE;
    response.header.bySize = sizeof(SC_MoveStop);
    response.header.byType = dfPACKET_SC_MOVE_STOP;
    response.ID = user->session_id;
    response.Direction = user->direction;
    response.X = user->x;
    response.Y = user->y;

    ExceptOneBroadcastPacket((char*)&response, sizeof(SC_MoveStop), user->session_id); 

    // 좌표 동기화 체크
    if (abs(user->x - packet->X) > 10 || abs(user->y - packet->Y) > 10)
    {
        SendSyncPacket(user);
    }
}

void IocpServer::ProcessUserMove()
{
    EnterCriticalSection(&user_list_cs_);
    for (auto& user : user_list_)
    {
        if (user->is_move)
        {
            // 이동 방향에 따라 좌표 업데이트
            switch (user->direction)
            {
            case dfPACKET_MOVE_DIR_LL:
                user->x -= 2;
                break;
            case dfPACKET_MOVE_DIR_RR:
                user->x += 2;
                break;
            case dfPACKET_MOVE_DIR_UU:
                user->y -= 2;
                break;
            case dfPACKET_MOVE_DIR_DD:
                user->y += 2;
                break;
            }

            // 맵 경계 체크 (맵 크기를 1000x1000으로 가정)
            user->x = max(0, min(user->x, 1000));
            user->y = max(0, min(user->y, 1000));
        }
    }
    LeaveCriticalSection(&user_list_cs_);
}

void IocpServer::CheckUserTimeout()
{
    DWORD current_time = GetTickCount();
    EnterCriticalSection(&user_list_cs_);
    for (auto it = user_list_.begin(); it != user_list_.end();)
    {
        User* user = *it;
        if (current_time - user->time > 10000) // 10초 타임아웃
        {
            DWORD session_id = user->session_id;
            it = user_list_.erase(it);
            LeaveCriticalSection(&user_list_cs_);

            // DeleteUser 함수 호출
            DeleteUser(session_id);

            EnterCriticalSection(&user_list_cs_);
        }
        else
        {
            ++it;
        }
    }
    LeaveCriticalSection(&user_list_cs_);
}

void IocpServer::ProcessHeartbeat(User* user, CS_HEARTBEAT* packet)
{
    user->time = packet->Time;

    // SC_HEARTBEAT 패킷 생성 및 전송
    SC_HEARTBEAT response;
    response.header.byCode = dfPACKET_CODE;
    response.header.bySize = sizeof(SC_HEARTBEAT)-sizeof(PacketHeader);
    response.header.byType = dfPACKET_SC_HEARTBEAT;
    response.Time = packet->Time;

    SendPacket(user->session_id, (char*)&response, sizeof(SC_HEARTBEAT));
}

void IocpServer::SendSyncPacket(User* user)
{
    SC_SYNC packet;
    packet.header.byCode = dfPACKET_CODE;
    packet.header.bySize = sizeof(SC_SYNC) - sizeof(PacketHeader);
    packet.header.byType = dfPACKET_SC_SYNC;
    packet.ID = user->session_id;
    packet.X = user->x;
    packet.Y = user->y;

    BroadcastPacket((char*)&packet, sizeof(SC_SYNC));
}
void IocpServer::SendCreateMyCharacter(User* user)
{
    // SC_CreateMyCharacter 패킷 생성
    SC_CreateMyCharacter packet;
    packet.header.byCode = dfPACKET_CODE;
    packet.header.bySize = sizeof(SC_CreateMyCharacter) - sizeof(PacketHeader);
    packet.header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
    packet.ID = user->session_id;
    packet.Direction = user->direction;
    packet.X = user->x;
    packet.Y = user->y;
    packet.HP = user->hp;

    // 패킷 전송
    SendPacket(user->session_id, (char*)&packet, sizeof(SC_CreateMyCharacter));
}

void IocpServer::SendCreateOtherCharacter(User* new_user)
{
    // SC_CreateOtherCharacter 패킷 생성
    SC_CreateOtherCharacter packet;
    packet.header.byCode = dfPACKET_CODE;
    packet.header.bySize = sizeof(SC_CreateOtherCharacter) - sizeof(PacketHeader);
    packet.header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
    packet.ID = new_user->session_id;
    packet.Direction = new_user->direction;
    packet.X = new_user->x;
    packet.Y = new_user->y;
    packet.HP = new_user->hp;

    // 모든 다른 유저에게 새 유저 정보 전송
    ExceptOneBroadcastPacket((char*)&packet, sizeof(SC_CreateOtherCharacter), new_user->session_id);
}

void IocpServer::SendExistingUsersInfo(User* new_user)
{
    EnterCriticalSection(&user_list_cs_);
    for (auto& user : user_list_)
    {
        if (user->session_id != new_user->session_id)
        {
            // 기존 유저 정보를 담은 SC_CreateOtherCharacter 패킷 생성
            SC_CreateOtherCharacter packet;
            packet.header.byCode = dfPACKET_CODE;
            packet.header.bySize = sizeof(SC_CreateOtherCharacter) - sizeof(PacketHeader);
            packet.header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
            packet.ID = user->session_id;
            packet.Direction = user->direction;
            packet.X = user->x;
            packet.Y = user->y;
            packet.HP = user->hp;

            // 새 유저에게 기존 유저 정보 전송
            SendPacket(new_user->session_id, (char*)&packet, sizeof(SC_CreateOtherCharacter));
        }
    }
    LeaveCriticalSection(&user_list_cs_);
}

void IocpServer::ProcessAttack1(User* user, CS_Attack1* packet)
{
    short attack_range = 3;
    short attack_range_x = 0;
    short attack_range_y = 0;
    switch (user->direction)
    {
    case dfPACKET_MOVE_DIR_LL:
        attack_range_x = user->x - attack_range; 
        attack_range_y = user->y;
        break;
    case dfPACKET_MOVE_DIR_RR:
        attack_range_x = user->x + attack_range;
        attack_range_y = user->y;
        break;
    case dfPACKET_MOVE_DIR_UU:
        attack_range_x = user->x;
        attack_range_y = user->y - attack_range; 
        break;
    case dfPACKET_MOVE_DIR_DD:
        attack_range_x = user->x;
        attack_range_y = user->y + attack_range; 
        break;
    }
  

    EnterCriticalSection(&monster_list_cs_);
    for (auto& monster : monster_list_)
    {
        if ((monster->x == attack_range_x) && (monster->y == attack_range_y))
        {
            monster->hp = (monster->hp > 3) ? monster->hp - 3 : 0;

            // SC_MonsterDamage 패킷 생성 및 브로드캐스트
            SC_MonsterDamage damage_packet;
            damage_packet.header.byCode = dfPACKET_CODE;
            damage_packet.header.bySize = sizeof(SC_MonsterDamage) - sizeof(PacketHeader); 
            damage_packet.header.byType = dfPACKET_SC_MONSTER_DAMAGE; 
            damage_packet.attackID = user->session_id;
            damage_packet.damageID = monster->id;
            damage_packet.damageHP = monster->hp;

            BroadcastPacket((char*)&damage_packet, sizeof(SC_MonsterDamage)); 

            if (monster->hp == 0)
            {
                monster_delete_list_.push_back(monster->id);

                // SC_DeleteMonster 패킷 생성 및 브로드캐스트
                SC_DeleteMonster delete_packet; 
                delete_packet.header.byCode = dfPACKET_CODE;
                delete_packet.header.bySize = sizeof(SC_DeleteMonster) - sizeof(PacketHeader);
                delete_packet.header.byType = dfPACKET_SC_DELETE_MONSTER; 
                delete_packet.ID = monster->id;
                monster_delete_list_.push_back(monster->id);

                BroadcastPacket((char*)&delete_packet, sizeof(SC_DeleteMonster)); 
            }
        }
    }
    LeaveCriticalSection(&monster_list_cs_);
}

unsigned __stdcall IocpServer::MonsterProcessThread(void* arg)
{
    IocpServer* server = static_cast<IocpServer*>(arg);
    while (true)
    {
        // 프레임 시작 신호 대기
        WaitForSingleObject(server->frame_start_event_, INFINITE);

        // server->DequeueJobMonster(); 

        // 몬스터 생성
        server->AddMonster();

        // 몬스터 AI 및 이동 처리
        server->ProcessMonsterAI();

        server->RemoveMonster();

        // 프레임 종료 신호
        SetEvent(server->frame_end_event_);
    }
    return 0;
}

/////// 현재 서버 설계에선 필요 없음
//void IocpServer::DequeueJobMonster()
//{
//    EnterCriticalSection(&monster_queue_cs_);
//    while (!monster_job_queue_.empty())
//    {
//        Job job = monster_job_queue_.front();
//        monster_job_queue_.pop();
//        LeaveCriticalSection(&monster_queue_cs_);
//
//
//        EnterCriticalSection(&monster_queue_cs_);
//    }
//    LeaveCriticalSection(&monster_queue_cs_);
//}

void IocpServer::AddMonster()
{
    Monster* newMonster = new Monster;
    newMonster->id = GenerateMonsterSessionId();
    newMonster->direction = dfPACKET_MOVE_DIR_LL;
    newMonster->hp = 100;
    newMonster->x = GetRandomNumber(0, 1000);
    newMonster->y = GetRandomNumber(0, 1000);

    EnterCriticalSection(&monster_list_cs_);
    monster_list_.push_back(newMonster);
    LeaveCriticalSection(&monster_list_cs_);

    // 몬스터 생성 SC 패킷 생성 및 브로드캐스트
    SC_CreateMonster packet;
    packet.header.byCode = 0x96;
    packet.header.byType = dfPACKET_SC_CREATE_MONSTER;
    packet.header.bySize = sizeof(SC_CreateMonster) - sizeof(PacketHeader);
    packet.ID = newMonster->id;
    packet.Direction = newMonster->direction;
    packet.HP = newMonster->hp;
    packet.X = newMonster->x;
    packet.Y = newMonster->y;

    BroadcastPacket((char*)&packet, sizeof(SC_CreateMonster));

}

DWORD IocpServer::GenerateMonsterSessionId()
{
    static DWORD nextMonsterSessionId = 1;
    return InterlockedIncrement(&nextMonsterSessionId); 
}

void IocpServer::ProcessMonsterAI()
{
    EnterCriticalSection(&monster_list_cs_);
    for (auto& monster : monster_list_)
    {
        int state = GetRandomNumber(0, 2);
        // 몬스터의 상태에 따른 AI 처리
        switch (state)
        {
        case 0:
            // 대기 상태
            break;
        case 1:
            // 이동
            UpdateMonsterPosition(monster);
            break;
        case 2:
            // 방향변화
            UpdateMonsterDirection(monster); 
            break;
        }
    }
    LeaveCriticalSection(&monster_list_cs_);
}

Monster* IocpServer::GetMonsterById(DWORD monster_id)
{
    EnterCriticalSection(&monster_list_cs_);
    for (auto& monster : monster_list_)
    {
        if (monster->id == monster_id)
        {
            LeaveCriticalSection(&monster_list_cs_);
            return monster;
        }
    }
    LeaveCriticalSection(&monster_list_cs_);
    return nullptr;
}

void IocpServer::UpdateMonsterPosition(Monster* monster)
{
    EnterCriticalSection(&monster_list_cs_);
    for (auto& monster : monster_list_)
    {
        // 이동 방향에 따라 좌표 업데이트
        switch (monster->direction)
        {
        case dfPACKET_MOVE_DIR_LL:
            monster->x -= 2;
            break;
        case dfPACKET_MOVE_DIR_RR:
            monster->x += 2;
            break;
        case dfPACKET_MOVE_DIR_UU:
            monster->y -= 2;
            break;
        case dfPACKET_MOVE_DIR_DD:
            monster->y += 2;
            break;
        

        // 맵 경계 체크 (맵 크기를 1000x1000으로 가정)
        monster->x = max(0, min(monster->x, 1000)); 
        monster->y = max(0, min(monster->y, 1000));
        }
    }
    LeaveCriticalSection(&monster_list_cs_);
    BroadcastMonsterMove(monster);
}

void IocpServer::UpdateMonsterDirection(Monster* monster)
{
    EnterCriticalSection(&monster_list_cs_);
    int dir = GetRandomNumber(0, 3);

    switch (dir)
    {
    case 0:
        monster->direction = dfPACKET_MOVE_DIR_LL;
        break;
    case 1:
        monster->direction = dfPACKET_MOVE_DIR_RR; 
        break;
    case 2:
        monster->direction = dfPACKET_MOVE_DIR_UU;
        break;
    case3:
        monster->direction = dfPACKET_MOVE_DIR_DD;
        break;
    }

    LeaveCriticalSection(&monster_list_cs_);
    BroadcastMonsterMove(monster); 
}

void IocpServer::BroadcastMonsterMove(Monster* monster)
{
    // SC_MonsterMove 패킷 생성
    SC_MonsterMove packet;
    packet.header.byCode = dfPACKET_CODE;
    packet.header.bySize = sizeof(SC_MonsterMove) - sizeof(PacketHeader);
    packet.header.byType = dfPACKET_SC_MONSTER_MOVE;
    packet.ID = monster->id;
    packet.Direction = monster->direction;
    packet.X = monster->x;
    packet.Y = monster->y;

    // 모든 유저에게 패킷 전송
    BroadcastPacket((char*)&packet, sizeof(SC_MonsterMove));
}

void IocpServer::RemoveMonster()
{
    // monster_delete_list_를 ID 기준으로 정렬
    EnterCriticalSection(&monster_delete_list_cs_);
    monster_delete_list_.sort();
    LeaveCriticalSection(&monster_delete_list_cs_);

    EnterCriticalSection(&monster_list_cs_);

    // monster_delete_list_가 비어있지 않은 경우에만 처리
    if (!monster_delete_list_.empty())
    {
        auto delete_it = monster_delete_list_.begin();
        auto monster_it = monster_list_.begin();

        while (monster_it != monster_list_.end() && delete_it != monster_delete_list_.end())
        {
            if ((*monster_it)->id == *delete_it)
            {
                Monster* monsterToDelete = *monster_it;

                delete monsterToDelete;

                // 몬스터 리스트에서 해당 몬스터 제거
                monster_it = monster_list_.erase(monster_it);
                ++delete_it;
            }
            else if ((*monster_it)->id < *delete_it)
            {
                ++monster_it;
            }
            else
            {
                ++delete_it;
            }
        }

        // 삭제 작업이 완료되었으므로 삭제 리스트를 비움
        EnterCriticalSection(&monster_delete_list_cs_); 
        monster_delete_list_.clear();
        LeaveCriticalSection(&monster_delete_list_cs_); 
    }

    LeaveCriticalSection(&monster_list_cs_);
}

unsigned __stdcall IocpServer::ChatProcessThread(void* arg)
{
    IocpServer* server = static_cast<IocpServer*>(arg);
    while (true)
    {
        // 프레임 시작 신호 대기
        WaitForSingleObject(server->frame_start_event_, INFINITE);

        server->DequeueJobChat();

        // 프레임 종료 신호
        SetEvent(server->frame_end_event_);
    }
    return 0;
}

void IocpServer::DequeueJobChat()
{
    EnterCriticalSection(&chat_queue_cs_);
    while (!chat_job_queue_.empty())
    {
        Job job = chat_job_queue_.front();
        chat_job_queue_.pop();
        LeaveCriticalSection(&chat_queue_cs_);

        // 채팅 관련 작업 처리
        User* user = GetUserBySessionId(job.session_id);
        if (user)
        {
            SC_CHAT packet; 
            packet.header.byCode = 0x96;
            packet.header.bySize = sizeof(packet) - sizeof(PacketHeader);
            packet.header.byType = dfPACKET_SC_CHAT;
            packet.ID = job.session_id;
            memcpy_s(packet.message, sizeof(packet.message), job.data + sizeof(PacketHeader), sizeof(job.data) - sizeof(PacketHeader));

            // 모든 유저에게 패킷 전송
            BroadcastPacket((char*)&packet, sizeof(SC_CHAT)); 

        }

        EnterCriticalSection(&chat_queue_cs_); 
    }
    LeaveCriticalSection(&chat_queue_cs_); 
}


unsigned __stdcall IocpServer::FrameThread(void* arg)
{
    IocpServer* server = static_cast<IocpServer*>(arg);
    DWORD frame_time = 1000 / FRAME_RATE; // 60fps 기준 프레임 시간
    while (true)
    {
        DWORD start_time = GetTickCount();

        // 프레임 시작 신호
        SetEvent(server->frame_start_event_);

        // 모든 처리 스레드가 완료될 때까지 대기
        WaitForSingleObject(server->frame_end_event_, INFINITE);
        WaitForSingleObject(server->frame_end_event_, INFINITE);
        WaitForSingleObject(server->frame_end_event_, INFINITE);

        // 프레임 종료 후 다음 프레임까지 대기
        DWORD elapsed_time = GetTickCount() - start_time;
        if (elapsed_time < frame_time)
        {
            Sleep(frame_time - elapsed_time);
        }

        // 이벤트 리셋
        ResetEvent(server->frame_start_event_);
        ResetEvent(server->frame_end_event_);

        // 유저 타임아웃 체크
        server->CheckUserTimeout();
    }
    return 0;
}






