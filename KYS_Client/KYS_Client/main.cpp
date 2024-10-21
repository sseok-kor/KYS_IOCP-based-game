#include "main_client.h"

int main() 
{
    MainClient client;

    

    if (!client.Initialize()) 
    {
        wprintf(L"Failed to initialize client.\n");
        return 1;
    }

    client.GetNetwork().Initialize();

    if (!client.GetNetwork().Connect("127.0.0.1", 8000)) 
    {
        wprintf(L"Failed to connect to server.\n");
        return 1;
    }

    client.GameLoop();

    return 0;
}