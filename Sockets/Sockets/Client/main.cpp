#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>

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
	printf("------------- CLIENT ------------- \n");

	int checkError = 0;

	// Initialize Socket Lib
	InitWinSock();

	// Create a new socket
	SOCKET client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_socket == INVALID_SOCKET)
		printWSErrorAndExit(std::string("Client Socket Creation").c_str());

	// ---------  Set the ip address info ---------
	sockaddr_in address_out;
	address_out.sin_family = AF_INET;	// IpV4
	address_out.sin_port = htons(50);	// Port
	const char* remoteAddrStr = "192.168.0.21"; // My IP Address
	inet_pton(AF_INET, remoteAddrStr, &address_out.sin_addr);

	// Send a message 

	for (int i = 0; i < 5; ++i)
	{
		char recieved_msg[15] = { NULL };
		int* addrSize = new int; // I think it demands a pointer with the size, not an output variable -_-
		*addrSize = sizeof(address_out);
		sendto(
			client_socket, // Which socket sends, and by doing so binds. 
			"ping", // Message to send
			sizeof(5), // Size of message in bytes
			0, // Flags, by now 0
			(const struct sockaddr*)&address_out, // Casting due to C API implementation
			sizeof(address_out)
		);

		recvfrom(client_socket, recieved_msg, 15, 0, (sockaddr*)&address_out, addrSize);
		delete addrSize;
		printf("MESSAGE RECIEVED: %s \n", recieved_msg);
		Sleep(2500);
	}
	// Delete and stop socket
	checkError = closesocket(client_socket);
	if (checkError != NO_ERROR)
		printWSErrorAndExit(std::string("Closing Client Socket").c_str());
	else
		printf("Closed Client Socket Successfully \n");

	// Cleanup Socket Lib
	checkError = WSACleanup();
	if (checkError != NO_ERROR)
		printWSErrorAndExit(std::string("Win Socket Clean Up").c_str());
	else
		printf("Win Socket Cleaned Up Successfully \n");

	system("pause");

	return 0;
}