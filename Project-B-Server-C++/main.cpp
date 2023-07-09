#include <iostream>
#include <list>

using namespace std;






list<string> validCommands{"tu", "eu", "e", "newClient", "leave", "ping"};

int currentID = 0;
int eventTPS = 40;
int transformTPS = 14;
int SERVERPORT = 6969;
int serverVersion = 3;

const int checksBeforeDisconnect = 3;
//setInterval(checkDisconnectTimers, 1000);

list<string> playerTransformInfo{};
list<string> playerInfo{};
list<int> currentPlauerIDs{};
list<int> playerDisconnectTimers{};
list<string> eventsToSend{};
