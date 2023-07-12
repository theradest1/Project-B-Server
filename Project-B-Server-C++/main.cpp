#include <iostream>
#include <cstring>
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <list>
#include <vector>

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996) 

#define SERVER_PORT 6969
#define BUFFER_LEN 1024
const list<string> validCommands{"tu", "eu", "e", "newClient", "leave", "ping"};

int currentID = 0;
int currentUMessageID = 0;

const int eventTPS = 40;
const int transformTPS = 14;
const int maxMessageID = 0;
const int checksBeforeDisconnect = 3;
const int serverVersion = -1;

list<string> playerTransformInfo{};
list<string> playerInfo{};
list<int> currentPlauerIDs{};
list<int> playerDisconnectTimers{};
list<string> eventsToSend{};
list<int> uMessageID{};

SOCKET server_socket;
WSADATA wsa;
sockaddr_in server, client;

void initializeServer() 
{
	system("title UDP Server");

	// initialise winsock
	printf("Starting Server.\nInitialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code: %d", WSAGetLastError());
		exit(0);
	}
	printf("Initialised.\nCreating Socket...\n");

	// create a socket
	if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket: %d", WSAGetLastError());
	}
	printf("Socket created.\nSetting Bind Info...\n");

	// prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(SERVER_PORT);
	printf("Info Setting Done.\Binding...\n");

	// bind
	if (bind(server_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code: %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	puts("Bind done.\n");
}


int main()
{
	initializeServer();

	while (true)
	{
		printf("Waiting for data...");
		fflush(stdout);
		char message[BUFFER_LEN] = {};

		// try to receive some data, this is a blocking call
		int message_len;
		int slen = sizeof(sockaddr_in);
		if (message_len = recvfrom(server_socket, message, BUFFER_LEN, 0, (sockaddr*)&client, &slen) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code: %d", WSAGetLastError());
			exit(0);
		}

		// print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		printf("Data: %s\n", message);

		cin.getline(message, BUFFER_LEN);

		// reply the client with 2the same data
		if (sendto(server_socket, message, strlen(message), 0, (sockaddr*)&client, sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code: %d", WSAGetLastError());
			return 3;
		}
	}

	closesocket(server_socket);
	WSACleanup();
	return 0;
}
