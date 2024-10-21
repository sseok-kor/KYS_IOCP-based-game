#include "RingBuffer.h"


using namespace std;


RingBuffer::RingBuffer() : ringBufferSize(4096)
{
	InitializeCriticalSection(&cs);
	begin = (char*)malloc(ringBufferSize);
	end = begin + ringBufferSize;
	readPos = begin;
	writePos = begin;
}

RingBuffer::RingBuffer(int buffersize) : ringBufferSize(buffersize)
{
	InitializeCriticalSection(&cs);
	begin = (char*)malloc(ringBufferSize);
	end = begin + ringBufferSize;
	readPos = begin;
	writePos = begin;
}

RingBuffer::~RingBuffer()
{
	DeleteCriticalSection(&cs);
	free(begin);
}

bool RingBuffer::ReSize(int size)
{
	EnterCriticalSection(&cs);
	if (size <= 0) // 입력받은 크기가 유효하지 않은 경우
	{
		return false;
	}

	int usedSize = GetUseSize();
	if (size < usedSize) // 입력받은 크기가 현재 사용중인 크기보다 작은경우
	{
		return false;
	}

	char* newBegin = (char*)malloc(size);
	if (newBegin == nullptr)
	{
		return false;
	}

	if (writePos >= readPos) // 데이터가 분할되지 않은 경우
	{
		memcpy_s(newBegin, size, readPos, writePos - readPos);
	}
	else // 데이터가 분할 된 경우
	{
		memcpy_s(newBegin, size, readPos, end - readPos);
		memcpy_s(newBegin + (end - readPos), size - (end - readPos), begin, writePos - begin);
	}



	begin = newBegin;
	end = begin + size;
	readPos = begin;
	writePos = begin + usedSize;
	ringBufferSize = size;

	free(newBegin);
	LeaveCriticalSection(&cs);
	return true;
}

int RingBuffer::GetBufferSize()
{
	return end - begin - 1;
}

int RingBuffer::GetUseSize()
{
	if (writePos >= readPos)
	{
		return writePos - readPos;
	}
	return (end - readPos - 1) + (writePos - begin);
}

int RingBuffer::GetFreeSize()
{
	if (writePos >= readPos)
	{
		return (end - writePos) + (readPos - begin - 1);
	}

	return (readPos - writePos - 1);
}

int RingBuffer::Enqueue(const char* buffer, int size)
{
	EnterCriticalSection(&cs);
	if (size <= 0 || GetFreeSize() < size)
	{
		return 0;
	}

	if (DirectEnqueueSize() >= size)
	{
		memcpy_s(writePos, size, buffer, size);
		MoveFront(size);
		return size;
	}

	const char* temp = buffer;

	int directenqueueSize = DirectEnqueueSize();
	memcpy_s(writePos, directenqueueSize, temp, directenqueueSize);
	temp += directenqueueSize;
	MoveFront(directenqueueSize);

	int remainSize = size - directenqueueSize;
	memcpy_s(writePos, remainSize, temp, remainSize);
	MoveFront(remainSize);

	LeaveCriticalSection(&cs);
	return size;


}

int RingBuffer::Dequeue(char* pDest, int size)
{
	EnterCriticalSection(&cs);
	if (size <= 0 || GetUseSize() < size)
	{
		return 0;
	}

	if (DirectDequeueSize() >= size)
	{
		memcpy_s(pDest, size, readPos, size);
		MoveRear(size);
		return size;
	}

	char* pDestTmp = pDest;
	int directdequeueSize = DirectDequeueSize();
	memcpy_s(pDestTmp, directdequeueSize, readPos, directdequeueSize);
	MoveRear(directdequeueSize);

	int remainSize = size - directdequeueSize;
	pDestTmp += directdequeueSize;
	memcpy_s(pDestTmp, remainSize, readPos, remainSize);
	MoveRear(remainSize);

	LeaveCriticalSection(&cs);
	return size;


}

int RingBuffer::Peek(char* pDest, int size)
{
	EnterCriticalSection(&cs);
	if (size <= 0 || GetUseSize() < size)
	{
		return 0;
	}

	if (DirectDequeueSize() >= size)
	{
		memcpy_s(pDest, size, readPos, size);
		return size;
	}

	char* pDestTmp = pDest;
	char* tempRear = readPos;
	int directdequeueSize = DirectDequeueSize();
	memcpy_s(pDestTmp, directdequeueSize, readPos, directdequeueSize);
	MoveRear(directdequeueSize);

	int remainSize = size - directdequeueSize;
	pDestTmp += directdequeueSize;
	memcpy_s(pDestTmp, remainSize, readPos, remainSize);
	readPos = tempRear;
	LeaveCriticalSection(&cs);
	return size;
}

void RingBuffer::ClearBuffer()
{
	EnterCriticalSection(&cs);
	readPos = begin;
	writePos = begin;
	memset(begin, 0, ringBufferSize);
	LeaveCriticalSection(&cs);
}

int RingBuffer::DirectEnqueueSize()
{
	if (writePos >= readPos)
	{
		return end - writePos - 1;
	}

	return readPos - writePos - 1;
}

int RingBuffer::DirectDequeueSize()
{
	if (writePos >= readPos)
	{
		return writePos - readPos;
	}

	return end - readPos - 1;
}

void RingBuffer::MoveRear(int iSize)
{
	EnterCriticalSection(&cs);
	readPos += iSize;
	if (readPos >= end - 1)
	{
		int overFlow = readPos - end;
		readPos = begin + overFlow;
	}
	LeaveCriticalSection(&cs);
}

void RingBuffer::MoveFront(int iSize)
{
	EnterCriticalSection(&cs);
	writePos += iSize;
	if (writePos >= end - 1)
	{
		int overFlow = writePos - end;
		writePos = begin + overFlow;
	}
	LeaveCriticalSection(&cs);
}

char* RingBuffer::GetFrontBufferPtr()
{
	EnterCriticalSection(&cs);
	return writePos;
	LeaveCriticalSection(&cs);
}

char* RingBuffer::GetRearBufferPtr()
{
	EnterCriticalSection(&cs);
	return readPos;
	LeaveCriticalSection(&cs);
}