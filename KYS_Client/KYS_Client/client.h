#pragma once

#include <Windows.h>
#include <list>
#include "protocol.h"
#include "RingBuffer.h"
#include "console_buffer.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Ŭ���̾�Ʈ�� �⺻ Ŭ����
class Client 
{

public:
    virtual ~Client() {}

    virtual void Render(ConsoleBuffer* buffer) = 0;
    virtual void Update() = 0;
    virtual void PacketProcess(char* packet) = 0;

};