#pragma once
#include <Windows.h>
#include <vector>
#include <string>

class ConsoleBuffer {
private:
    std::vector<CHAR_INFO> frontBuffer;
    std::vector<CHAR_INFO> backBuffer;
    int width;
    int height;
    HANDLE hConsole;
    COORD bufferSize;
    SMALL_RECT writeRegion;

public:
    ConsoleBuffer(int w, int h);
    ~ConsoleBuffer();
    void Clear();
    void SetChar(int x, int y, wchar_t ch, WORD attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    void WriteString(int x, int y, const std::wstring& str, WORD attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    void DrawBorder();
    void Render();
};