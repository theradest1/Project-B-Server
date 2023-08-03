//this server is for universal multiplayer, but I am hopefully going to be using it in other things too
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <Ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

constexpr int TCP_PORT = 4242;
constexpr int UDP_PORT = 6969;

std::vector<std::string> clientTransforms{};

int currentClientID = 0;

SOCKET serverSocketUDP, serverSocketTCP;
sockaddr_in serverAddressUDP, serverAddressTCP;

std::vector<sockaddr_in> clients{};

const int BUFFER_LEN = 1024; //max message length

std::vector<int> clientIDs{};
std::vector<std::string> clientIPs{};
std::vector<int> clientUDPPorts{};
std::vector<int> clientTCPPorts{};

std::vector<std::string> tcpMessagesToSend{};

//utill ----------
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

	// Don't forget to add the last segment if the string doesn't end with the delimiter.
	if (!current.empty()) {
		result.push_back(current);
	}

	return result;
}


//tcp ------------
void handleTCPClient(SOCKET clientSocket) {
	int clientID = currentClientID;
	currentClientID++;

	std::cout << "Opened tcp socket" << std::endl;

	u_long mode = 1; // Set non-blocking mode on Windows
	ioctlsocket(clientSocket, FIONBIO, &mode);

	char message[BUFFER_LEN] = {};
	int bytesRead;

	while (true) {
		bytesRead = recv(clientSocket, message, BUFFER_LEN, 0);

		if (bytesRead == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				std::cerr << "Error in recv: " << error << std::endl;
				break;
			}
		}
		else if (bytesRead == 0) {
			std::cout << "Connection closed by the client" << std::endl;
			break;
		}
		else {
			std::vector<std::string> messages = splitString(message, '|');
			//loop through messsages
			for(std::string finalMessage : messages) {
				//std::cout << "Got TCP message: " + finalMessage << std::endl;

				if (finalMessage == "ping") {
					std::string response = "pong";
					int len = response.length();
					send(clientSocket, response.c_str(), len, 0);
				}
			}
		}
	}

	std::cout << "Closed tcp socket" << std::endl;
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

void processUDPMessage(std::string message) 
{
	//std::cout << "Got UDP message: " + message << std::endl;
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

		processUDPMessage(finalMessage);
		if(finalMessage == "ping") {
			sendUDPMessage("pong", clientAddress, clientAddressLength);
		}
		else {
			sendUDPMessage("awhdhakwhdkhakwhkhawtgfauwyrgyagfkeasfgwuagefkaywetgfyuagwyfkgayuwkgfkeajfgbjakgefd", clientAddress, clientAddressLength);
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