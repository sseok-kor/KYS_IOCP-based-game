#pragma once

#include <stdlib.h>
#include <windows.h>
#include <memory.h>

class RingBuffer
{
public:

	RingBuffer(void); // ������
	RingBuffer(int bufferSize); // �Ű����� ������
	~RingBuffer(void);

	bool ReSize(int size); // ������ ũ�⺯��
	int GetBufferSize();

	int GetUseSize(); // ���� ������� �뷮 ���
	int GetFreeSize(); // ���� ���ۿ� ���� �뷮 ���

	int Enqueue(const char* buffer, int size); //writePos�� ������ ����
	int Dequeue(char* pDest, int size); // readPos���� ������ ������. readPos �̵�
	int Peek(char* pDest, int size); // readPos���� ������ ������. readPos ����


	///	/////////////////////////////////////////////////////////////////////////
	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// (������ ���� ����)
	//
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	//
	// Parameters: ����.
	// Return: (int)��밡�� �뷮.
	////////////////////////////////////////////////////////////////////////
	int DirectEnqueueSize(void);
	int DirectDequeueSize(void);


	/////////////////////////////////////////////////////////////////////////
	// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
	//
	// Parameters: ����.
	// Return: (int)�̵�ũ��
	/////////////////////////////////////////////////////////////////////////
	void MoveRear(int iSize); 
	void MoveFront(int iSize);

	char* GetFrontBufferPtr(void); // ������ front ������ ����
	char* GetRearBufferPtr(void); // ������ rear ������ ����

	void ClearBuffer(); // ������ ��� ������ ����

private:

	char* begin;
	char* end;
	char* readPos;
	char* writePos;
	CRITICAL_SECTION cs;

	int ringBufferSize;

};