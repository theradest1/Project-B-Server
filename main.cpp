//this server is for universal multiplayer, but I am hopefully going to be using it in other things too
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <Ws2tcpip.h>
#include <thread>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

constexpr int TCP_PORT = 4242;
constexpr int UDP_PORT = 6969;

//std::vector<std::string> clientTransforms{};

int currentClientID = 0;

SOCKET serverSocketUDP, serverSocketTCP;
sockaddr_in serverAddressUDP, serverAddressTCP;

std::vector<sockaddr_in> clients{};

const int BUFFER_LEN = 2048; //max message length

bool serverOnline = true; //set to false to close all tcp streams

std::vector<int> clientIDs{};
std::vector<std::string> clientIPs{};
std::vector<sockaddr_in> clientUDPSockets{};

std::vector<std::string> tcpMessagesToSend{};

//utill ----------
int findIndex(std::vector<int> v, int element) {
	auto it = std::find(v.begin(), v.end(), element);
	if (it != v.end()) {
		return it - v.begin();
	}
	else {
		return -1;
	}
}
int getClientIndex(int clientID) 
{
	int clientIndex = findIndex(clientIDs, clientID);
	if (clientIndex == -1) {
		std::cout << "Could not find client index: " << clientID << std::endl;
	}
	return clientIndex;
}
void addClientData(int clientID)
{
	clientIDs.push_back(clientID);
	tcpMessagesToSend.push_back("");
	//udp socket added on first udp message
}
void removeClientData(int clientID)
{
	int index = getClientIndex(clientID);

	try
	{
		clientIDs.erase(clientIDs.begin() + index);
		tcpMessagesToSend.erase(tcpMessagesToSend.begin() + index);
		clientUDPSockets.erase(clientUDPSockets.begin() + index);
	}
	catch (const std::exception&)
	{
		std::cout << "Tried to remove client " << clientID << " data, but it doesnt exist" << std::endl;
	}
}
std::vector<std::string> splitString(const std::string& input, char delimiter) {
	std::vector<std::string> result;
	std::string current;

	for (char c : input) {
		if (c == delimiter) {
			result.push_back(current);
			current.clear();
		}
		else {
			current += c;
		}
	}

	if (!current.empty()) {
		result.push_back(current);
	}

	return result;
}
void addTCPMessageToAll(std::string message) 
{
	//std::cout << "Added event: " << message << std::endl;
	for (int index = 0; index < tcpMessagesToSend.size(); index++) {
		tcpMessagesToSend[index] += message + "|";
	}
}

void addTCPMessage(int clientID, std::string message)
{
	tcpMessagesToSend[getClientIndex(clientID)] += message + "|";
}

std::string subCharArray(char arr[], int start, int length)
{
	std::string final = "";
	// Pick starting point
	for (int i = start; i < start + length; i++)
	{
		final += arr[i];
	}
	return final;
}

//tcp ------------
void sendTCPMessage(SOCKET clientSocket, std::string message)
{
	//std::cout << "Sent TCP message: " << message << std::endl;
	message += "|";
	int len = message.length();
	send(clientSocket, message.c_str(), len, 0);
}

void handleTCPClient(SOCKET clientSocket) {
	std::cout << "Opened tcp socket" << std::endl;

	//get socket ID
	int clientID = currentClientID;
	currentClientID++;
	addClientData(clientID);
	addTCPMessage(clientID, "clientInfo~" + std::to_string(clientID));

	//set non-blocking
	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);

	//read vars
	char message[BUFFER_LEN] = {};
	int bytesRead;

	while (serverOnline) {
		//read from stream
		bytesRead = recv(clientSocket, message, sizeof(message), 0);

		if (bytesRead == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				std::cerr << "Error in recv: " << error << std::endl;
				break;
			}
		}
		else if (bytesRead == 0) {
			std::cout << "Stream " << clientID << " closed by the client" << std::endl;
			closesocket(clientSocket);
			removeClientData(clientID);
			return;
		}
		else {
			//have to cut message because of tcp stream echoes
			std::string cutMessage = subCharArray(message, 0, bytesRead);
			//std::cout << "(" << bytesRead << ") Got TCP message: " << cutMessage << std::endl;

			//loop through messsages (they get mashed)
			std::vector<std::string> messages = splitString(cutMessage, '|');
			for(std::string finalMessage : messages) {
				if (finalMessage == "ping") {
					sendTCPMessage(clientSocket, "pong");
				}
				else {
					//std::cout << "Got TCP message: " << finalMessage << std::endl;
					addTCPMessageToAll(finalMessage);
				}
			}
		}

		int clientIndex = getClientIndex(clientID);
		if (clientIndex == -1) {
			std::cout << "Closing TCP stream since there is no active ID" << std::endl;
			closesocket(clientSocket);
			return;
		}

		//write to stream
		if (tcpMessagesToSend[clientIndex] != "")
		{
			sendTCPMessage(clientSocket, tcpMessagesToSend[clientIndex]);
			tcpMessagesToSend[clientIndex] = "";
		}
	}

	//close stream
	std::cout << "Stream " << clientID << " closed by the server" << std::endl;
	removeClientData(clientID);
	closesocket(clientSocket);
}

void createTCPServer() {
	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << std::endl;
		return;
	}

	serverSocketTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocketTCP == INVALID_SOCKET) {
		std::cerr << "Error creating TCP socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	serverAddressTCP.sin_family = AF_INET;
	serverAddressTCP.sin_addr.s_addr = INADDR_ANY;
	serverAddressTCP.sin_port = htons(TCP_PORT);

	if (bind(serverSocketTCP, (struct sockaddr*)&serverAddressTCP, sizeof(serverAddressTCP)) == SOCKET_ERROR) {
		std::cerr << "Error binding TCP socket: " << WSAGetLastError() << std::endl;
		closesocket(serverSocketTCP);
		WSACleanup();
		return;
	}

	if (listen(serverSocketTCP, 5) == SOCKET_ERROR) {
		std::cerr << "Error listening on TCP socket: " << WSAGetLastError() << std::endl;
		closesocket(serverSocketTCP);
		WSACleanup();
		return;
	}

	std::cout << "TCP Server listening on port " + std::to_string(TCP_PORT) + "\n" << std::endl;

	//checking for new clients
	sockaddr_in clientAddress;
	int clientAddressLength = sizeof(clientAddress);
	SOCKET clientSocket;

	while (true) {
		clientSocket = accept(serverSocketTCP, (struct sockaddr*)&clientAddress, &clientAddressLength);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Error accepting TCP connection: " << WSAGetLastError() << std::endl;
			continue;
		}

		//open a new thread for a client
		std::thread clientThread(handleTCPClient, clientSocket);
		clientThread.detach();
	}

	closesocket(serverSocketTCP);
	WSACleanup();
}

//udp ----------
void sendUDPMessage(std::string message, sockaddr_in clientAddressUDP, int clientAddressLength)
{
	sendto(serverSocketUDP, message.c_str(), strlen(message.c_str()), 0, (sockaddr*)&clientAddressUDP, clientAddressLength);
}

void sendUDPMessageToAll(std::string message, int excludedIndex = -1) 
{
	for (int index = 0; index < clientUDPSockets.size(); index++) {
		if (excludedIndex != index) {
			sendUDPMessage(message, clientUDPSockets[index], sizeof(clientUDPSockets[index]));
		}
	}
}

void processUDPMessage(std::string message, int clientIndex)
{
	std::cout << "Got UDP message: " + message << std::endl;

	sendUDPMessageToAll(message, clientIndex);
}

void udpReciever() {
	sockaddr_in clientAddress;
	int clientAddressLength = sizeof(clientAddress);

	while (true) {
		char message[BUFFER_LEN] = {};

		int bytesRead = recvfrom(serverSocketUDP, message, BUFFER_LEN, 0, (sockaddr*)&clientAddress, &clientAddressLength);
		if (bytesRead < 0) {
			std::cerr << "Error receiving UDP data: " << WSAGetLastError() << std::endl;
			continue;
		};

		std::string finalMessage = message;

		if(finalMessage == "ping") {
			sendUDPMessage("pong", clientAddress, clientAddressLength);
		}
		else{
			std::vector<std::string> peices = splitString(finalMessage, '~');

			//add udp socket if it doesnt exist
			int clientID = std::stoi(peices[0]);
			int clientIndex = findIndex(clientIDs, clientID);
			//std::cout << clientUDPSockets.size() << std::endl;
			if (clientUDPSockets.size() <= clientIndex) {
				clientUDPSockets.push_back(clientAddress);
				std::cout << "Added UDP port for client " << clientID << std::endl;
			}

			processUDPMessage(finalMessage, clientIndex);
		}
	}
}

void createUDPServer() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << std::endl;
		return;
	}

	serverSocketUDP= socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocketUDP == INVALID_SOCKET) {
		std::cerr << "Error creating UDP socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	serverAddressUDP.sin_family = AF_INET;
	serverAddressUDP.sin_addr.s_addr = INADDR_ANY;
	serverAddressUDP.sin_port = htons(UDP_PORT);

	if (bind(serverSocketUDP, (sockaddr*)&serverAddressUDP, sizeof(serverAddressUDP)) == SOCKET_ERROR) {
		std::cerr << "Error binding UDP socket: " << WSAGetLastError() << std::endl;
		closesocket(serverSocketUDP);
		WSACleanup();
		return;
	}

	std::cout << "UDP Server listening on port " + std::to_string(UDP_PORT) + "\n" << std::endl;

	udpReciever();

	closesocket(serverSocketUDP);
	WSACleanup();
}


int main() {
	//set console output to file
	/*std::ofstream out("output.txt");
	std::streambuf* coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());*/

	// Run both TCP and UDP servers concurrently
	if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)createTCPServer, NULL, 0, NULL) == NULL) {
		std::cerr << "Error creating TCP server thread" << std::endl;
		return 1;
	}

	if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)createUDPServer, NULL, 0, NULL) == NULL) {
		std::cerr << "Error creating UDP server thread" << std::endl;
		return 1;
	}

	// Wait for servers to finish (which will never happen as they run in infinite loops)
	WaitForSingleObject(GetCurrentThread(), INFINITE);
	return 0;
}