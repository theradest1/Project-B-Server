#include <iostream>
#include <cstring>
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <list>
#include <vector>

using namespace std;
#pragma comment(lib, "ws2_32.lib")
#define SERVER_PORT 6969
#define BUFFER_SIZE 1024
#define SERVER_VERSION 3
const list<string> validCommands{"tu", "eu", "e", "newClient", "leave", "ping"};

int currentID = 0;
int eventTPS = 40;
int transformTPS = 14;


const int checksBeforeDisconnect = 3;
//setInterval(checkDisconnectTimers, 1000);

list<string> playerTransformInfo{};
list<string> playerInfo{};
list<int> currentPlauerIDs{};
list<int> playerDisconnectTimers{};
list<string> eventsToSend{};


int main() {
	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Failed to initialize Winsock." << std::endl;
		return 1;
	}

	SOCKET server_socket;
	struct sockaddr_in server_address, client_address;
	char buffer[BUFFER_SIZE];

	// Create server socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket == INVALID_SOCKET) {
		std::cerr << "Failed to create server socket." << std::endl;
		WSACleanup();
		return 1;
	}

	// Set server address
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// Bind server socket to server address
	if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
		std::cerr << "Failed to bind server socket." << std::endl;
		closesocket(server_socket);
		WSACleanup();
		return 1;
	}

	std::vector<SOCKET> client_sockets;  // Store client sockets

	while (true) {
		// Receive message from any client
		struct sockaddr_in client_address;
		int client_address_length = sizeof(client_address);
		int received_bytes = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_address, &client_address_length);
		if (received_bytes == SOCKET_ERROR) {
			std::cerr << "Failed to receive message." << std::endl;
			closesocket(server_socket);
			WSACleanup();
			return 1;
		}

		// Print received message and client info
		std::cout << "Received " << received_bytes << " bytes from " << inet_ntoa(client_address.sin_addr) << std::endl;

		// Add new client socket if not already in the list
		bool new_client = true;
		for (const SOCKET& client_socket : client_sockets) {
			if (client_socket == server_socket)
				continue;
			if (client_address.sin_addr.s_addr == client_address.sin_addr.s_addr) {
				new_client = false;
				break;
			}
		}
		if (new_client) {
			client_sockets.push_back(server_socket);
			std::cout << "New client connected: " << inet_ntoa(client_address.sin_addr) << std::endl;
		}

		// Send response back to the client
		if (sendto(server_socket, "pong", 4, 0, (struct sockaddr*)&client_address, client_address_length) == SOCKET_ERROR) {
			std::cerr << "Failed to send response." << std::endl;
			closesocket(server_socket);
			WSACleanup();
			return 1;
		}
	}

	// Close client sockets
	for (const SOCKET& client_socket : client_sockets) {
		closesocket(client_socket);
	}

	// Close server socket
	closesocket(server_socket);
	WSACleanup();

	return 0;
}
