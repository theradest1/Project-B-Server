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
#include <sstream>
#include <ctime>


//using namespace std;

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

std::vector<std::string> playerTransformInfo{};
std::vector<std::string> playerInfo{};
std::vector<int> currentPlayerIDs{};
std::vector<int> playerDisconnectTimers{};
std::vector<std::string> eventsToSend{};
std::vector<int> uMessageIDs{};

SOCKET server_socket;
WSADATA wsa;
sockaddr_in server;

std::map<std::string, std::function<void(std::string, sockaddr_in)>> functionMap; //a map of all functions available to the clients

//UTILL -------------------------------
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

void sendMessage(std::string message, sockaddr_in client)
{
	if (sendto(server_socket, message.c_str(), strlen(message.c_str()), 0, (sockaddr*)&client, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code: %d", WSAGetLastError());
	}
}

std::string getCurrentDateTimeAsString() {
	// Get the current time
	std::time_t now = std::time(nullptr);

	// Convert the current time to a string
	std::string dateTime = std::ctime(&now);

	// Remove the newline character at the end
	dateTime.erase(dateTime.length() - 1);

	return dateTime;
}

int getPlayerIndex(int playerID) {
	return currentPlayerIDs.indexOf(parseInt(info.split("~")[1]));
}

//Client options -------------------------------
void updateTransform(std::string message, sockaddr_in client) //to do
{
}
void updateEvent(std::string message, sockaddr_in client) //to do
{
}
void newEvent(std::string message, sockaddr_in client) //to do
{
}
void newClient(std::string message, sockaddr_in client)
{
	sendMessage(std::to_string(currentID) + "~" + std::to_string(transformTPS) + "~" + std::to_string(eventTPS) + "~" + std::to_string(maxMessageID), client);
	std::vector<std::string> splitInfo = splitString(message, '~');
	addEventToAll("newClient~" + std::to_string(currentID) + '~' + splitInfo[1]);

	//debug it
	std::cout << "---------------------\nDate/Time: " << getCurrentDateTimeAsString() << "\nNew client: ID = " << currentID << ", Username: " << splitInfo[1] << std::endl;

	std::string allPlayerJoinInfo = "";
	for (int playerIndex : currentPlayerIDs) {
		allPlayerJoinInfo += "newClient~" + currentPlayerIDs[playerIndex] + '~' + playerInfo[playerIndex] + " | ";
	}
	eventsToSend.push_back(allPlayerJoinInfo);
	playerInfo.push_back(splitInfo[1]);
	playerTransformInfo.push_back("(0, 0, 0)~(0, 0, 0, 1)");
	playerDisconnectTimers.push_back(0);
	currentPlayerIDs.push_back(currentID);
	uMessageIDs.push_back(0);

	currentID++;
}
void leave(std::string message, sockaddr_in client) //to do
{
	getPlayerIndex(splitString(message, '~'));
	disconnectClient(currentPlayerIDs.indexOf(parseInt(info.split("~")[1])));
	std::cout << "Player with ID " << playerID << " has left the game" << std::endl;
}

void ping(std::string message, sockaddr_in client)
{
	sendMessage(std::to_string(serverVersion), client);
}


//Server Functions -------------------------------
void checkDisconnectTimers() //to do
{
}
void disconnectClient() //to do
{
}
void addEventToAll(std::string message) //to do
{
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

void processMessage(std::string message, sockaddr_in client)
{
	std::cout << "Message: " << message << ", IP: " << inet_ntoa(client.sin_addr) << ", Port: " << ntohs(client.sin_port) << std::endl;
	std::string functionName = splitString(message, '~')[0];
	if (functionMap.find(functionName) != functionMap.end()) {
		functionMap[functionName](message, client);
	}
	else {
		std::cout << "Command with name " << functionName << " does not exist." << std::endl;
	}
}

void bindFunctions()
{
	functionMap["tu"] = [](std::string message, sockaddr_in client) { updateTransform(message, client); };
	functionMap["eu"] = [](std::string message, sockaddr_in client) { updateEvent(message, client); };
	functionMap["e"] = [](std::string message, sockaddr_in client) { newEvent(message, client); };
	functionMap["newClient"] = [](std::string message, sockaddr_in client) { newClient(message, client); };
	functionMap["leave"] = [](std::string message, sockaddr_in client) { leave(message, client); };
	functionMap["ping"] = [](std::string message, sockaddr_in client) { ping(message, client); };
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

		std::cin.getline(message, BUFFER_LEN);
	}

	printf("Closing Server...\n");
	closesocket(server_socket);
	printf("Done.\nCleaning Up...\n");
	WSACleanup();
	printf("Done, goodbye (:");
	return 0;
}