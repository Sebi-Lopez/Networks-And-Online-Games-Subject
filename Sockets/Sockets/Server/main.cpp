#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <WinSock2.h>

void printWSErrorAndExit(const char* msg)
{
	wchar_t* s = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s,
		0, NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	system("pause");
	exit(-1);
}

void InitWinSock() 
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printWSErrorAndExit(std::string("Initializing Win Socket").c_str());
	else
		printf("Initialized Win Socket successfully \n");
}
int main()
{
	printf("------------- SERVER ------------- \n");

	int checkError = 0;

	// Initialize Socket Lib
	InitWinSock();

	// Create a new socket
	SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server_socket == INVALID_SOCKET)
		printWSErrorAndExit(std::string("Server Socket Creation").c_str());
	else
		printf("Server Socket Created Successfully \n");

	// ---------  Set the ip address info ---------
	sockaddr_in address_in;
	address_in.sin_family = AF_INET;	// IpV4
	address_in.sin_port = htons(50);	// Port
	address_in.sin_addr.S_un.S_addr = INADDR_ANY;	// Server can get info from any ip


	// Bind the actual ip info to the server socket
	checkError = bind(server_socket, (const struct sockaddr*)&address_in, sizeof(address_in));	// Casting is due to C build API
	if (checkError != NO_ERROR)
		printWSErrorAndExit(std::string("Binding Address to socket").c_str());
	else
		printf("Binding Address to socket successfull \n");

	for (int i = 0; i < 5; ++i)
	{
		char recieved_msg[15] = { NULL };
		int* addrSize = new int;
		*addrSize = sizeof(address_in);		// I think it demands a pointer with the size, not an output variable -_- 
		recvfrom(server_socket, recieved_msg, 15, 0, (sockaddr*)&address_in, addrSize);
		delete addrSize;

		printf("MESSAGE RECIEVED: %s \n", recieved_msg);

		sendto(
			server_socket, // Which socket sends, and by doing so binds. 
			"pong", // Message to send
			sizeof(5), // Size of message in bytes
			0, // Flags, by now 0
			(const struct sockaddr*)&address_in, // Casting due to C API implementation
			sizeof(address_in)
		);

		
		Sleep(2500);
	}
	

	// Delete and stop socket
	checkError = closesocket(server_socket);
	if (checkError != NO_ERROR)
		printWSErrorAndExit(std::string("Closing Server Socket").c_str());
	else
		printf("Closed Server Socket Successfully \n");

	// Cleanup Socket Lib
	checkError = WSACleanup();
	if (checkError != NO_ERROR)
		printWSErrorAndExit(std::string("Win Socket Clean Up").c_str());
	else
		printf("Win Socket Cleaned Up Successfully \n");

	system("pause");

	return 0;
}