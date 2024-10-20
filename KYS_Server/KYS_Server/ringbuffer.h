#pragma once

#include <stdlib.h>
#include <windows.h>
#include <memory.h>

class RingBuffer
{
public:

	RingBuffer(void); // 생성자
	RingBuffer(int bufferSize); // 매개변수 생성자
	~RingBuffer(void);

	bool ReSize(int size); // 링버퍼 크기변경
	int GetBufferSize();

	int GetUseSize(); // 현재 사용중인 용량 얻기
	int GetFreeSize(); // 현재 버퍼에 남은 용량 얻기

	int Enqueue(const char* buffer, int size); //writePos에 데이터 넣음
	int Dequeue(char* pDest, int size); // readPos에서 데이터 가져옴. readPos 이동
	int Peek(char* pDest, int size); // readPos에서 데이터 가져옴. readPos 고정


	///	/////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	////////////////////////////////////////////////////////////////////////
	int DirectEnqueueSize(void);
	int DirectDequeueSize(void);


	/////////////////////////////////////////////////////////////////////////
	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: (int)이동크기
	/////////////////////////////////////////////////////////////////////////
	void MoveRear(int iSize); 
	void MoveFront(int iSize);

	char* GetFrontBufferPtr(void); // 버퍼의 front 포인터 얻음
	char* GetRearBufferPtr(void); // 버퍼의 rear 포인터 얻음

	void ClearBuffer(); // 버퍼의 모든 데이터 삭제

private:

	char* begin;
	char* end;
	char* readPos;
	char* writePos;
	CRITICAL_SECTION cs;

	int ringBufferSize;

};