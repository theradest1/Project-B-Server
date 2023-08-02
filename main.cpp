//this server is for universal multiplayer, but I am hopefully going to be using it in other things too
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int TCP_PORT = 4242;
constexpr int UDP_PORT = 6969;

std::vector<std::string> clientTransforms{};

int count = 0;

SOCKET serverSocketUDP, serverSocketTCP, clientSocketTCP;
WSADATA wsaDataUDP, wsaDataTCP;
sockaddr_in serverAddressUDP, serverAddressTCP, clientAddressUDP, clientAddressTCP;


const int BUFFER_LEN = 1024; //max message length

std::vector<int> clientIDs{};
std::vector<std::string> clientIPs{};
std::vector<int> clientUDPPorts{};
std::vector<int> clientTCPPorts{};

int clientAddressLength;

void processTCPMessage(std::string message, SOCKET clientSocket)
{
	send(clientSocket, std::to_string(count).c_str(), bytesRead, 0);
}

void handleTCPClient(SOCKET clientSocket) {
	char message[BUFFER_LEN] = {};
	int bytesRead;

	while ((bytesRead = recv(clientSocket, message, BUFFER_LEN, 0)) > 0) {
		std::cout << message << std::endl;
		processTCPMessage(message, clientSocket);
	}
	closesocket(clientSocket);
}

void createTCPServer() {
	
	int result = WSAStartup(MAKEWORD(2, 2), &wsaDataTCP);
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

	while (true) {
		clientSocketTCP = accept(serverSocketTCP, (struct sockaddr*)&clientAddressTCP, &clientAddressLength);
		if (clientSocketTCP == INVALID_SOCKET) {
			std::cerr << "Error accepting TCP connection: " << WSAGetLastError() << std::endl;
			continue;
		}

		handleTCPClient(clientSocketTCP);
	}

	closesocket(serverSocketTCP);
	WSACleanup();
}


//udp ----------
void sendUDPMessage(std::string message) 
{
	sendto(serverSocketUDP, message.c_str(), strlen(message.c_str()), 0, (sockaddr*)&clientAddressUDP, clientAddressLength);
}

void processUDPMessage(std::string message) 
{
	std::cout << message << std::endl;
	sendUDPMessage("pong");
}

void udpReciever() {
	while (true) {
		char message[BUFFER_LEN] = {};

		int bytesRead = recvfrom(serverSocketUDP, message, BUFFER_LEN, 0, (sockaddr*)&clientAddressUDP, &clientAddressLength);
		if (bytesRead < 0) {
			std::cerr << "Error receiving UDP data: " << WSAGetLastError() << std::endl;
			continue;
		};

		processUDPMessage(message);
	}
}

void createUDPServer() {
	int result = WSAStartup(MAKEWORD(2, 2), &wsaDataUDP);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << std::endl;
		return;
	}

	clientAddressLength = sizeof(clientAddressUDP);

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