#include <iostream>
#include <cstring>
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <list>
#include <vector>
#include <functional>
#include <map>
#include <string>

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996) 

#define SERVER_PORT 6969
#define BUFFER_LEN 1024

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
sockaddr_in server;

std::map<std::string, std::function<void(string, sockaddr_in)>> functionMap; //a map of all functions available to the clients

void bindFunctions() 
{
	functionMap["tu"] = [](string message, sockaddr_in client) { updateTransform(message, client); };
}

void updateTransform(string message, sockaddr_in client)
{
	//debug
	printf("Message: %s, IP: %s, Port: %d", message, client);
}

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
	printf("Bind done.\nServer Setup Done\n\nServer Port: %d\nServer Version: %d\n", SERVER_PORT, serverVersion);
}

void processMessage(string message, sockaddr_in client)
{
	string functionName = splitString(message, '~')[0];
	if (functionMap.find(functionName) != functionMap.end()) {
		functionMap[functionName](message, client);
	}
	else {
		printf("Command with name %s does not exist.", functionName);
	}
}

void sendMessage(string message, sockaddr_in client)
{
	const char* data = "test message";
	if (sendto(server_socket, data, strlen(data), 0, (sockaddr*)&client, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code: %d", WSAGetLastError());
	}
}


int main()
{
	bindFunctions();
	initializeServer();

	while (true)
	{
		fflush(stdout);
		char message[BUFFER_LEN] = {};

		// try to receive some data, this is a blocking call
		int message_len;
		int slen = sizeof(sockaddr_in);
		sockaddr_in client;
		if (message_len = recvfrom(server_socket, message, BUFFER_LEN, 0, (sockaddr*)&client, &slen) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code: %d", WSAGetLastError());
			exit(0);
		}

		// print details of the client/peer and the data received and then process it
		printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		printf("Data: %s\n", message);
		processMessage(message, client);

		cin.getline(message, BUFFER_LEN);
	}

	printf("Closing Server...\n");
	closesocket(server_socket);
	printf("Done.\nCleaning Up...\n");
	WSACleanup();
	printf("Done, goodbye (:");
	return 0;
}




//UTILL -------------------
std::vector<std::string> splitString(const std::string& input, char delimiter) {
	std::vector<std::string> result;
	size_t start = 0;
	size_t end = input.find(delimiter);

	while (end != std::string::npos) {
		result.push_back(input.substr(start, end - start));
		start = end + 1;
		end = input.find(delimiter, start);
	}

	result.push_back(input.substr(start));

	return result;
}
