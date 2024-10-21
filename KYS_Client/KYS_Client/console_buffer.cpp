#include "console_buffer.h"
#include <algorithm>

ConsoleBuffer::ConsoleBuffer(int w, int h) : width(w), height(h) 
{
    frontBuffer.resize(width * height);
    backBuffer.resize(width * height);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // �ܼ� â ũ�� ����
    SMALL_RECT windowSize = { 0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    // �ܼ� ���� ũ�� ����
    COORD bufferSize = { static_cast<SHORT>(width), static_cast<SHORT>(height) };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    // Ŀ�� �����
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.bVisible = FALSE;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    // ��ũ�ѹ� ����
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    csbi.dwSize.X = width;
    csbi.dwSize.Y = height;
    SetConsoleScreenBufferSize(hConsole, csbi.dwSize);

    // �ܼ� ��� ���� (���� ���� ��� ��Ȱ��ȭ)
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hConsole, mode);
}

ConsoleBuffer::~ConsoleBuffer() {
    CloseHandle(hConsole);
}

void ConsoleBuffer::Clear() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x < 130) {
                SetChar(x, y, L'.', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else {
                SetChar(x, y, L' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
        }
    }
}

void ConsoleBuffer::SetChar(int x, int y, wchar_t ch, WORD attributes) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int index = y * width + x;
        backBuffer[index].Char.UnicodeChar = ch;
        backBuffer[index].Attributes = attributes;
    }
}

void ConsoleBuffer::WriteString(int x, int y, const std::wstring& str, WORD attributes) {
    for (size_t i = 0; i < str.length() && x + i < width; ++i) {
        SetChar(x + i, y, str[i], attributes);
    }
}

void ConsoleBuffer::DrawBorder() {
    for (int x = 0; x < width; ++x) {
        SetChar(x, 0, L'��');
        SetChar(x, height - 1, L'��');
    }
    for (int y = 1; y < height - 1; ++y) {
        SetChar(0, y, L'��');
        SetChar(width - 1, y, L'��');
        SetChar(100, y, L'��');  // ���� ������ ä�� ���� ���м�
    }
    SetChar(0, 0, L'��');
    SetChar(width - 1, 0, L'��');
    SetChar(0, height - 1, L'��');
    SetChar(width - 1, height - 1, L'��');
    SetChar(100, 0, L'��');
    SetChar(100, height - 1, L'��');
}

void ConsoleBuffer::Render() {
    WriteConsoleOutput(hConsole, backBuffer.data(), bufferSize, { 0, 0 }, &writeRegion);
    std::swap(frontBuffer, backBuffer);
}

