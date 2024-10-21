#pragma once

#include <Windows.h>
#include <list>
#include "protocol.h"
#include "RingBuffer.h"
#include "console_buffer.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 클라이언트의 기본 클래스
class Client 
{

public:
    virtual ~Client() {}

    virtual void Render(ConsoleBuffer* buffer) = 0;
    virtual void Update() = 0;
    virtual void PacketProcess(char* packet) = 0;

};