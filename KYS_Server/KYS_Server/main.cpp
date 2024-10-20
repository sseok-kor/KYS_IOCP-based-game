#include "Network.h"

int main()
{
	IocpServer server;
	server.Initialize();
	server.StartProcess();

	DWORD currentClientCount = 0;

	while (1)
	{
		if (currentClientCount != 10)
		{
			currentClientCount++;
			Sleep(1000);
		}
		else
		{
			printf("Current Client : %d\n", server.active_sessions_);
			currentClientCount = 0;
		}
		 
	}

	return 0;
}