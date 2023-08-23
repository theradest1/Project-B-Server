#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <thread>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

const int serverVersion = 8;
constexpr int TCP_PORT = 4242;
constexpr int UDP_PORT = 6969;

const int autoDisconnectSeconds = 3;

int lastGivenTeam = 1;

int currentClientID = 0;

int serverSocketUDP, serverSocketTCP;
sockaddr_in serverAddressUDP, serverAddressTCP;

std::vector<sockaddr_in> clients{};

const int BUFFER_LEN = 2048; // max message length

bool serverOnline = true; // set to false to close all tcp streams

std::vector<int> clientIDs{};
std::vector<int> clientDisconnectTimers{};
std::vector<std::string> clientIPs{};
std::vector<sockaddr_in> clientUDPSockets{};
std::vector<bool> assignedUDPSocket{};
std::vector<int> clientTCPSockets{}; // Using integers for file descriptors
std::string settings = "";

// utill ----------
void debug(std::string message)
{
	std::cout << message << std::endl;
	std::cerr << message << std::endl;
}

void resetClientDisconnectTimer(int clientIndex)
{
	if (clientIndex == -1)
	{
		return;
	}
	clientDisconnectTimers[clientIndex] = 0;
}

std::string condenseStringVector(std::vector<std::string> stringVector, std::string devider = " ", int startIndex = 0)
{
	std::string finalString = "";
	for (int index = startIndex; index < stringVector.size(); index++)
	{
		finalString += stringVector[index];
		if (index != stringVector.size() - 1)
		{
			finalString += devider;
		}
	}
	return finalString;
}
int findIndex(std::vector<int> v, int element)
{
	auto it = std::find(v.begin(), v.end(), element);
	if (it != v.end())
	{
		return it - v.begin();
	}
	else
	{
		return -1;
	}
}
int getClientIndex(int clientID)
{
	int clientIndex = findIndex(clientIDs, clientID);
	if (clientIndex == -1)
	{
		debug("Could not find client index: " + std::to_string(clientID));
	}
	return clientIndex;
}
void addClientData(int clientID, int clientSocket)
{
	clientIDs.push_back(clientID);
	clientDisconnectTimers.push_back(0);
	clientTCPSockets.push_back(clientSocket);
	sockaddr_in blankSocket;
	clientUDPSockets.push_back(blankSocket);
	assignedUDPSocket.push_back(false);
}
std::vector<std::string> splitString(const std::string &input, char delimiter)
{
	std::vector<std::string> result;
	std::string current;

	for (char c : input)
	{
		if (c == delimiter)
		{
			result.push_back(current);
			current.clear();
		}
		else
		{
			current += c;
		}
	}

	if (!current.empty())
	{
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

// tcp ------------

void sendTCPMessageSocket(int clientSocket, std::string message)
{
	message += "|";
	int len = message.length();
	send(clientSocket, message.c_str(), len, 0);
}

void sendTCPMessageID(int clientID, std::string message)
{
	int clientIndex = getClientIndex(clientID);

	if (clientIndex == -1)
	{
		return;
	}

	int clientSocket = clientTCPSockets[clientIndex];
	sendTCPMessageSocket(clientSocket, message);
}
void sendTCPMessageToAll(std::string message)
{
	for (int clientIndex = 0; clientIndex < clientTCPSockets.size(); clientIndex++)
	{
		sendTCPMessageSocket(clientTCPSockets[clientIndex], message + "|");
	}
}
void sendTCPMessageToAll(std::string message, int excludedID)
{
	for (int clientIndex = 0; clientIndex < clientTCPSockets.size(); clientIndex++)
	{
		if (clientIDs[clientIndex] != excludedID)
		{
			sendTCPMessageSocket(clientTCPSockets[clientIndex], message + "|");
		}
	}
}
void removeClientData(int clientID)
{
	int index = getClientIndex(clientID);

	if (index == -1)
	{
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
	// message format:
	// messageType~info1~info2~info3
	std::vector<std::string> messageParts = splitString(message, '~');
	std::string messageType = messageParts[0];

	// message types: (short for bandwidth)
	// g - global event (echoes event to all clients) - g~info1~info2
	// o - excluding event (echoes event to everyone but sender) - o~info1~info2
	// d - direct event (echoes event to one client) - d~clientID~info1~info2
	// s - server event (has the server process the event) - s~info1~info2

	// c++ doesn't allow switch statements for strings, so here is a if block ):
	if (messageType == "g")
	{
		std::string messageToSend = condenseStringVector(messageParts, "~", 1);
		sendTCPMessageToAll(messageToSend);
	}
	else if (messageType == "o")
	{
		std::string messageToSend = condenseStringVector(messageParts, "~", 1);
		sendTCPMessageToAll(messageToSend, clientID);
	}
	else if (messageType == "d")
	{
		int recvClient = std::stoi(messageParts[1]);
		std::string messageToSend = condenseStringVector(messageParts, "~", 2);
		sendTCPMessageID(recvClient, messageToSend);
	}
	else if (messageType == "s")
	{
		std::string serverEvent = messageParts[1];
		if (serverEvent == "ping")
		{
			sendTCPMessageID(clientID, "pong");
		}
		else if (message == "quit")
		{
			removeClientData(clientID);
		}
		else
		{
			debug("Unknown server event from client " + std::to_string(clientID) + ": " + serverEvent);
		}
	}
	else
	{
		debug("Unknown message type from client " + std::to_string(clientID) + ": " + messageType);
	}
}
void handleTCPClient(int clientSocket)
{
	debug("Opened tcp socket");

	// get socket ID
	int clientID = currentClientID;
	currentClientID++;
	addClientData(clientID, clientSocket);

	sendTCPMessageID(clientID, "setClientInfo~" + std::to_string(clientID));
	sendTCPMessageID(clientID, "quickSettings~" + settings);

	// read vars
	char message[BUFFER_LEN] = {};
	int bytesRead;

	while (serverOnline)
	{
		// read from stream (blocking)
		bytesRead = recv(clientSocket, message, sizeof(message), 0);

		if (bytesRead == -1)
		{
			if (errno != EWOULDBLOCK)
			{
				std::cerr << "Error in recv: " << strerror(errno) << std::endl;
				break;
			}
		}
		else if (bytesRead == 0)
		{
			debug("Stream " + std::to_string(clientID) + " closed by the client");
			close(clientSocket);
			removeClientData(clientID);
			return;
		}
		else
		{
			// have to cut message because of tcp stream echoes
			std::string cutMessage = subCharArray(message, 0, bytesRead);

			// loop through messsages (they get mashed)
			std::vector<std::string> messages = splitString(cutMessage, '|');
			for (std::string finalMessage : messages)
			{
				processTCPMessage(finalMessage, clientID);
			}
		}

		int clientIndex = getClientIndex(clientID);
		if (clientIndex == -1)
		{
			debug("Closing TCP stream since there is no active ID");
			close(clientSocket);
			return;
		}
	}

	// close stream
	debug("Stream " + std::to_string(clientID) + " closed by the server");
	removeClientData(clientID);
	close(clientSocket);
}
void createTCPServer()
{
	serverSocketTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocketTCP == -1)
	{
		std::cerr << "Error creating TCP socket: " << strerror(errno) << std::endl;
		return;
	}

	serverAddressTCP.sin_family = AF_INET;
	serverAddressTCP.sin_addr.s_addr = INADDR_ANY;
	serverAddressTCP.sin_port = htons(TCP_PORT);

	if (bind(serverSocketTCP, (struct sockaddr *)&serverAddressTCP, sizeof(serverAddressTCP)) == -1)
	{
		std::cerr << "Error binding TCP socket: " << strerror(errno) << std::endl;
		close(serverSocketTCP);
		return;
	}

	if (listen(serverSocketTCP, 5) == -1)
	{
		std::cerr << "Error listening on TCP socket: " << strerror(errno) << std::endl;
		close(serverSocketTCP);
		return;
	}

	debug("TCP Server listening on port " + std::to_string(TCP_PORT));

	// checking for new clients
	sockaddr_in clientAddress;
	int clientAddressLength = sizeof(clientAddress);
	int clientSocket;

	while (true)
	{
		clientSocket = accept(serverSocketTCP, (struct sockaddr *)&clientAddress, (socklen_t *)&clientAddressLength);
		if (clientSocket == -1)
		{
			debug("Error accepting TCP connection: (Error message disabled until things are working)"); //+ strerror(errno));
			continue;
		}

		// open a new thread for a client
		std::thread clientThread(handleTCPClient, clientSocket);
		clientThread.detach();
	}

	close(serverSocketTCP);
}

// udp ----------
void sendUDPMessage(std::string message, sockaddr_in clientAddressUDP, int clientAddressLength)
{
	sendto(serverSocketUDP, message.c_str(), strlen(message.c_str()), 0, (sockaddr *)&clientAddressUDP, clientAddressLength);
}
void sendUDPMessageToAll(std::string message, int excludedIndex = -1)
{
	for (int index = 0; index < clientUDPSockets.size(); index++)
	{
		if (excludedIndex != index && assignedUDPSocket[index])
		{
			sendUDPMessage(message, clientUDPSockets[index], sizeof(clientUDPSockets[index]));
		}
	}
}
void processUDPMessage(std::string message, int clientIndex)
{
	sendUDPMessageToAll(message, clientIndex);
}
void udpReciever()
{
	sockaddr_in clientAddress;
	int clientAddressLength = sizeof(clientAddress);

	while (true)
	{
		char message[BUFFER_LEN] = {};

		int bytesRead = recvfrom(serverSocketUDP, message, BUFFER_LEN, 0, (sockaddr *)&clientAddress, (socklen_t *)&clientAddressLength);
		if (bytesRead < 0)
		{
			std::cerr << "Error receiving UDP data: " << strerror(errno) << std::endl;
			continue;
		};

		std::string finalMessage = message;

		if (finalMessage == "ping")
		{
			sendUDPMessage("pong", clientAddress, clientAddressLength);
		}
		else if (finalMessage == "Iping")
		{ // An information ping
			sendUDPMessage("pong~" + std::to_string(serverVersion) + '~' + std::to_string(clientIDs.size()), clientAddress, clientAddressLength);
		}
		else
		{
			std::vector<std::string> peices = splitString(finalMessage, '~');

			// add udp socket if it doesnt exist
			try
			{
				int clientID = std::stoi(peices[0]);
				int clientIndex = findIndex(clientIDs, clientID);

				if (clientIndex != -1 && !assignedUDPSocket[clientIndex])
				{
					assignedUDPSocket[clientIndex] = true;
					clientUDPSockets[clientIndex] = clientAddress;
				}

				resetClientDisconnectTimer(clientIndex);
				processUDPMessage(finalMessage, clientIndex);
			}
			catch (const std::invalid_argument &e)
			{
				debug("Message from new client doesnt have valid start char: " + finalMessage);
			}
		}
	}
}
void createUDPServer()
{
	serverSocketUDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocketUDP == -1)
	{
		std::cerr << "Error creating UDP socket: " << strerror(errno) << std::endl;
		return;
	}

	serverAddressUDP.sin_family = AF_INET;
	serverAddressUDP.sin_addr.s_addr = INADDR_ANY;
	serverAddressUDP.sin_port = htons(UDP_PORT);

	if (bind(serverSocketUDP, (sockaddr *)&serverAddressUDP, sizeof(serverAddressUDP)) == -1)
	{
		std::cerr << "Error binding UDP socket: " << strerror(errno) << std::endl;
		close(serverSocketUDP);
		return;
	}

	debug("UDP Server listening on port " + std::to_string(UDP_PORT));

	udpReciever();

	close(serverSocketUDP);
}

// other
void checkDisconnectTimers()
{
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
		} })
		.detach();
}

void checkQuickSettings()
{

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
		} })
		.detach();
}

int main()
{
	// set console output to file
	std::ofstream out("output.txt");
	std::streambuf *coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());

	// Run both TCP and UDP servers concurrently
	std::thread tcpThread(createTCPServer);
	std::thread udpThread(createUDPServer);

	tcpThread.detach();
	udpThread.detach();

	checkDisconnectTimers();
	checkQuickSettings();

	// Keep the main thread alive
	while (serverOnline)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return 0;
}