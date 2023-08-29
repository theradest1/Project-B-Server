#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <Ws2tcpip.h>
#include <thread>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

const int serverVersion = 8;
constexpr int TCP_PORT = 4242;
constexpr int UDP_PORT = 6969;

const int autoDisconnectSeconds = 3;

int lastGivenTeam = 1;

int currentClientID = 0;

SOCKET serverSocketUDP, serverSocketTCP;
sockaddr_in serverAddressUDP, serverAddressTCP;

std::vector<sockaddr_in> clients{};

const int BUFFER_LEN = 2048; //max message length

bool serverOnline = true; //set to false to close all tcp streams

std::vector<int> clientIDs{};
std::vector<int> clientDisconnectTimers{};
std::vector<std::string> clientIPs{};
std::vector<sockaddr_in> clientUDPSockets{};
std::vector<bool> assignedUDPSocket{};
std::vector<SOCKET> clientTCPSockets{};
std::string settings = "";

//utill ----------
void debug(std::string message) {
	std::cout << message << std::endl;
	std::cerr << message << std::endl;
}

void resetClientDisconnectTimer(int clientIndex) {
	if (clientIndex == -1) {
		return;
	}
	clientDisconnectTimers[clientIndex] = 0;
}

std::string condenseStringVector(std::vector<std::string> stringVector, std::string devider = " ", int startIndex = 0) {
	std::string finalString = "";
	for (int index = startIndex; index < stringVector.size(); index++) {
		finalString += stringVector[index];
		if (index != stringVector.size() - 1) {
			finalString += devider;
		}
	}
	return finalString;
}
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
		debug("Could not find client index: " + std::to_string(clientID));
	}
	return clientIndex;
}
void addClientData(int clientID, SOCKET clientSocket)
{
	clientIDs.push_back(clientID);
	clientDisconnectTimers.push_back(0);
	clientTCPSockets.push_back(clientSocket);
	sockaddr_in blankSocket;
	clientUDPSockets.push_back(blankSocket);
	assignedUDPSocket.push_back(false);
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
	message += "|";
	int len = message.length();
	send(clientSocket, message.c_str(), len, 0);
}
void sendTCPMessage(int clientID, std::string message) {
	int clientIndex = getClientIndex(clientID);

	if (clientIndex == -1) {
		return;
	}

	SOCKET clientSocket = clientTCPSockets[clientIndex];
	sendTCPMessage(clientSocket, message);
}
void sendTCPMessageToAll(std::string message)
{
	for (int clientIndex = 0; clientIndex < clientTCPSockets.size(); clientIndex++) {
		sendTCPMessage(clientTCPSockets[clientIndex], message + "|");
	}
}
void sendTCPMessageToAll(std::string message, int excludedID)
{
	for (int clientIndex = 0; clientIndex < clientTCPSockets.size(); clientIndex++) {
		if (clientIDs[clientIndex] != excludedID) {
			sendTCPMessage(clientTCPSockets[clientIndex], message + "|");
		}
	}
}
void removeClientData(int clientID)
{
	int index = getClientIndex(clientID);

	if (index == -1) {
		return;
	}

	clientIDs.erase(clientIDs.begin() + index);
	clientDisconnectTimers.erase(clientDisconnectTimers.begin() + index);
	clientUDPSockets.erase(clientUDPSockets.begin() + index);
	clientTCPSockets.erase(clientTCPSockets.begin() + index);
	assignedUDPSocket.erase(assignedUDPSocket.begin() + index);

	sendTCPMessageToAll("removeClient~" + std::to_string(clientID));
}
void processTCPMessage(std::string message, int clientID) 
{
	//message format:
	//messageType~info1~info2~info3
	std::vector<std::string> messageParts = splitString(message, '~');
	std::string messageType = messageParts[0];

	//message types: (short for bandwidth)
	//g - global event (echoes event to all clients) - g~info1~info2
	//o - excluding event (echoes event to everyone but sender) - o~info1~info2
	//d - direct event (echoes event to one client) - d~clientID~info1~info2
	//s - server event (has the server process the event) - s~info1~info2

	//c++ doesn't allow switch statements for strings, so here is a if block ):
	if (messageType == "g") {
		std::string messageToSend = condenseStringVector(messageParts, "~", 1);
		sendTCPMessageToAll(messageToSend);
	}
	else if (messageType == "o") { 
		std::string messageToSend = condenseStringVector(messageParts, "~", 1);
		sendTCPMessageToAll(messageToSend, clientID);
	}
	else if (messageType == "d") {
		int recvClient = std::stoi(messageParts[1]);
		std::string messageToSend = condenseStringVector(messageParts, "~", 2);
		sendTCPMessage(recvClient, messageToSend);
	}
	else if (messageType == "s") {
		std::string serverEvent = messageParts[1];
		if (serverEvent == "ping") {
			sendTCPMessage(clientID, "pong");
		}
		else if (message == "quit") {
			removeClientData(clientID);
		}
		else {
			debug("Unknown server event from client " + std::to_string(clientID) + ": " + serverEvent);
		}
	}
	else {
		debug("Unknown message type from client " + std::to_string(clientID) + ": " + messageType);
	}
}
void handleTCPClient(SOCKET clientSocket) {
	debug("Opened tcp socket");

	//get socket ID
	int clientID = currentClientID;
	currentClientID++;
	addClientData(clientID, clientSocket);

	sendTCPMessage(clientID, "setClientInfo~" + std::to_string(clientID));
	sendTCPMessage(clientID, "quickSettings~" + settings);

	//read vars
	char message[BUFFER_LEN] = {};
	int bytesRead;

	while (serverOnline) {
		//read from stream (blocking)
		bytesRead = recv(clientSocket, message, sizeof(message), 0);

		if (bytesRead == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				std::cerr << "Error in recv: " << error << std::endl;
				break;
			}
		}
		else if (bytesRead == 0) {
			debug("Stream " + std::to_string(clientID) + " closed by the client");
			closesocket(clientSocket);
			removeClientData(clientID);
			return;
		}
		else {
			//have to cut message because of tcp stream echoes
			std::string cutMessage = subCharArray(message, 0, bytesRead);

			//loop through messsages (they get mashed)
			std::vector<std::string> messages = splitString(cutMessage, '|');
			for(std::string finalMessage : messages) {
				processTCPMessage(finalMessage, clientID);
			}
		}

		int clientIndex = getClientIndex(clientID);
		if (clientIndex == -1) {
			debug("Closing TCP stream since there is no active ID");
			closesocket(clientSocket);
			return;
		}
	}

	//close stream
	debug("Stream " + std::to_string(clientID) + " closed by the server");
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

	debug("TCP Server listening on port " + std::to_string(TCP_PORT));

	//checking for new clients
	sockaddr_in clientAddress;
	int clientAddressLength = sizeof(clientAddress);
	SOCKET clientSocket;

	while (true) {
		clientSocket = accept(serverSocketTCP, (struct sockaddr*)&clientAddress, &clientAddressLength);
		if (clientSocket == INVALID_SOCKET) {
			debug("Error accepting TCP connection: " + std::to_string(WSAGetLastError()));
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
		if (excludedIndex != index && assignedUDPSocket[index]) {
			sendUDPMessage(message, clientUDPSockets[index], sizeof(clientUDPSockets[index]));
		}
	}
}
void processUDPMessage(std::string message, int clientIndex)
{
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
		else if (finalMessage == "Iping") { //A ping, but requesting server info
			sendUDPMessage("pong~" + std::to_string(serverVersion) + '~' + std::to_string(clientIDs.size()), clientAddress, clientAddressLength);
		}
		else{
			std::vector<std::string> peices = splitString(finalMessage, '~');

			//add udp socket if it doesnt exist
			try {
				int clientID = std::stoi(peices[0]);
				int clientIndex = findIndex(clientIDs, clientID);

				if (clientIndex != -1 && !assignedUDPSocket[clientIndex]) {
					assignedUDPSocket[clientIndex] = true;
					clientUDPSockets[clientIndex] = clientAddress;
				}

				resetClientDisconnectTimer(clientIndex);
				processUDPMessage(finalMessage, clientIndex);
			}
			catch(const std::invalid_argument& e){
				debug("Message from new client doesnt have valid start char: " + finalMessage);
			}
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

	debug("UDP Server listening on port " + std::to_string(UDP_PORT));

	udpReciever();

	closesocket(serverSocketUDP);
	WSACleanup();
}

// other 
void checkDisconnectTimers() {
	std::thread([=]()
	{
		while (true) {
			std::vector<int> idsToDisconnect = {};
			for (int index = 0; index < clientDisconnectTimers.size(); index++) {
				clientDisconnectTimers[index]++;
				if (clientDisconnectTimers[index] >= autoDisconnectSeconds) {
					idsToDisconnect.push_back(clientIDs[index]);
				}
			}

			for (int index = 0; index < idsToDisconnect.size(); index++)
			{
				int id = idsToDisconnect[index];
				debug("Auto disconnected client " + std::to_string(id));
				removeClientData(id);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}).detach();
}

void checkQuickSettings() {

	std::thread([=]()
	{
		std::string commentString = "//";

		while (true) {
			//open file
			std::ifstream inputFile("quickSettings.txt");
			if (!inputFile.is_open()) {
				std::cerr << "Error opening file, breaking quick update" << std::endl;
			}
			
			//read lines
			std::string finalSettings = "";
			std::string line;
			while (std::getline(inputFile, line)) {
				if (line.substr(0, 2) != commentString) {
					finalSettings += line;
				}
			}
			inputFile.close();

			if (finalSettings != settings) {
				settings = finalSettings;
				debug("New Settings: " + finalSettings);
				sendTCPMessageToAll("quickSettings~" + finalSettings);
			}


			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		}
	}).detach();
}

int main() {
	//set console output to file
	std::ofstream out("output.txt");
	std::streambuf* coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());
	
	debug("Server V" + std::to_string(serverVersion));
	debug("If you have problems, check for updates on github\n");

	// Run both TCP and UDP servers concurrently
	if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)createTCPServer, NULL, 0, NULL) == NULL) {
		std::cerr << "Error creating TCP server thread" << std::endl;
		return 1;
	}

	if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)createUDPServer, NULL, 0, NULL) == NULL) {
		std::cerr << "Error creating UDP server thread" << std::endl;
		return 1;
	}
	
	checkDisconnectTimers();
	checkQuickSettings();


	//wait for servers to finish (which will never happen as they run in infinite loops)
	WaitForSingleObject(GetCurrentThread(), INFINITE);
	return 0;
}