//this server is for universal multiplayer, but I am hopefully going to be using it in other things too
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int TCP_PORT = 4242;
constexpr int UDP_PORT = 6969;

void handleTCPClient(SOCKET clientSocket) {
	char buffer[1024];
	int bytesRead;
	while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
		send(clientSocket, buffer, bytesRead, 0);
	}
	closesocket(clientSocket);
}

void createTCPServer() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << std::endl;
		return;
	}

	SOCKET serverSocket, clientSocket;
	sockaddr_in serverAddress, clientAddress;
	int clientAddressLength = sizeof(clientAddress);

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Error creating TCP socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(TCP_PORT);

	if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "Error binding TCP socket: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	if (listen(serverSocket, 5) == SOCKET_ERROR) {
		std::cerr << "Error listening on TCP socket: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	std::cout << "TCP Server listening on port " << TCP_PORT << std::endl;

	while (true) {
		clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Error accepting TCP connection: " << WSAGetLastError() << std::endl;
			continue;
		}

		handleTCPClient(clientSocket);
	}

	closesocket(serverSocket);
	WSACleanup();
}

void createUDPServer() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << std::endl;
		return;
	}

	SOCKET serverSocket;
	sockaddr_in serverAddress, clientAddress;
	int clientAddressLength = sizeof(clientAddress);

	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Error creating UDP socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(UDP_PORT);

	if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "Error binding UDP socket: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	std::cout << "UDP Server listening on port " << UDP_PORT << std::endl;

	while (true) {
		char buffer[1024];
		int bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddress, &clientAddressLength);
		if (bytesRead < 0) {
			std::cerr << "Error receiving UDP data: " << WSAGetLastError() << std::endl;
			continue;
		}

		sendto(serverSocket, buffer, bytesRead, 0, (struct sockaddr*)&clientAddress, clientAddressLength);
	}

	closesocket(serverSocket);
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