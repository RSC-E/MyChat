#include <winsock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <windows.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define SERVER_PORT 8888
#define QUEUE_UPDATE_INTERVAL 3 // 3秒更新一次

#pragma comment(lib,"ws2_32.lib")

struct ClientInfo {
    SOCKET socket;
    sockaddr_in address;
    std::string id;
    bool isInQueue;
    int queuePosition;
};

std::vector<ClientInfo> clients;
std::vector<ClientInfo*> waitingQueue;
bool serverRunning = true;

std::string GetClientID(sockaddr_in addr) {
    char* ipStr = inet_ntoa(addr.sin_addr);
    if (ipStr) {
        return std::string(ipStr) + ":" + std::to_string(ntohs(addr.sin_port));
    }
    return "Unknown:" + std::to_string(ntohs(addr.sin_port));
}

void SendToClient(SOCKET socket, const std::string& message) {
    send(socket, message.c_str(), message.length(), 0);
}

void BroadcastMessage(const std::string& message, SOCKET senderSocket = INVALID_SOCKET) {
    for (const auto& client : clients) {
        if (client.socket != senderSocket && !client.isInQueue) {
            SendToClient(client.socket, message);
        }
    }
}

void UpdateQueuePositions() {
    for (size_t i = 0; i < waitingQueue.size(); i++) {
        waitingQueue[i]->queuePosition = i + 1;
        std::string queueMsg = "Position in queue: " + std::to_string(i + 1) + 
                              " (Total in queue: " + std::to_string(waitingQueue.size()) + ")\n";
        SendToClient(waitingQueue[i]->socket, queueMsg);
    }
}

DWORD WINAPI QueueUpdateThread(LPVOID lpParam) {
    while (serverRunning) {
        Sleep(QUEUE_UPDATE_INTERVAL * 1000);
        UpdateQueuePositions();
    }
    return 0;
}

DWORD WINAPI HandleClient(LPVOID lpParam) {
    ClientInfo* clientInfo = (ClientInfo*)lpParam;
    SOCKET clientSocket = clientInfo->socket;
    std::string clientID = clientInfo->id;
    
    char buffer[BUFFER_SIZE];
    int recvSize;

    std::string welcomeMsg = "Welcome to the chat room! There are " + 
                            std::to_string(clients.size() - waitingQueue.size()) + 
                            " users online.\n";
    SendToClient(clientSocket, welcomeMsg);

    std::string joinMsg = "User " + clientID + " joined the room.\n";
    BroadcastMessage(joinMsg, clientSocket);
    std::cout << joinMsg;

    while ((recvSize = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[recvSize] = '\0';
        std::string message(buffer);
        
        if (message == "/exit" || message == "/quit") {
            break;
        }
        
        std::string chatMsg = "[" + clientID + "]: " + message + "\n";
        std::cout << chatMsg;
        BroadcastMessage(chatMsg, clientSocket);
    }

    closesocket(clientSocket);
    
    if (clientInfo->isInQueue) {
        auto it = std::find(waitingQueue.begin(), waitingQueue.end(), clientInfo);
        if (it != waitingQueue.end()) {
            waitingQueue.erase(it);
            UpdateQueuePositions();
        }
    } else {
        auto it = std::remove_if(clients.begin(), clients.end(), 
                               [clientSocket](const ClientInfo& ci) { 
                                   return ci.socket == clientSocket; 
                               });
        if (it != clients.end()) {
            clients.erase(it, clients.end());
        }
    }

    std::string leaveMsg = "User " + clientID + " left the room.\n";
    if (!clientInfo->isInQueue) {
        BroadcastMessage(leaveMsg);
    }
    std::cout << leaveMsg;

    delete clientInfo;
    return 0;
}

DWORD WINAPI HandleQueuedClient(LPVOID lpParam) {
    ClientInfo* clientInfo = (ClientInfo*)lpParam;
    
    while (clientInfo->isInQueue && serverRunning) {
        Sleep(2000);
        
        int activeClients = clients.size() - waitingQueue.size();
        if (activeClients < MAX_CLIENTS) {
            clientInfo->isInQueue = false;
            
            auto it = std::find(waitingQueue.begin(), waitingQueue.end(), clientInfo);
            if (it != waitingQueue.end()) {
                waitingQueue.erase(it);
            }
            
            UpdateQueuePositions();
            
            std::string successMsg = "You have joined the chat room! There are " + 
                                    std::to_string(activeClients + 1) + " users online.\n";
            SendToClient(clientInfo->socket, successMsg);
            
            std::string joinMsg = "User " + clientInfo->id + " joined the room from queue.\n";
            BroadcastMessage(joinMsg, clientInfo->socket);
            std::cout << joinMsg;
            
            CreateThread(NULL, 0, HandleClient, clientInfo, 0, NULL);
            break;
        }
    }
    
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started on port " << SERVER_PORT << ". Waiting for connections...\n";
    CreateThread(NULL, 0, QueueUpdateThread, NULL, 0, NULL);

    while (true) {
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        std::string clientID = GetClientID(clientAddr);
        std::cout << "Connection from: " << clientID << std::endl;

        ClientInfo* clientInfo = new ClientInfo{clientSocket, clientAddr, clientID, false, 0};
        clients.push_back(*clientInfo);

        int activeClients = clients.size() - waitingQueue.size() - 1;
        if (activeClients >= MAX_CLIENTS) {
            clientInfo->isInQueue = true;
            clientInfo->queuePosition = waitingQueue.size() + 1;
            waitingQueue.push_back(clientInfo);
            
            std::string queueMsg = "Position in queue: " + 
                                  std::to_string(clientInfo->queuePosition) + 
                                  " (Total in queue: " + std::to_string(waitingQueue.size()) + ")\n";
            SendToClient(clientSocket, queueMsg);
            
            std::cout << clientID << " added to queue. Position: " << clientInfo->queuePosition << std::endl;
            CreateThread(NULL, 0, HandleQueuedClient, clientInfo, 0, NULL);
        } else {
            std::string welcomeMsg = "Welcome to the chat room! There are " + 
                                    std::to_string(activeClients + 1) + " users online.\n";
            SendToClient(clientSocket, welcomeMsg);
            CreateThread(NULL, 0, HandleClient, clientInfo, 0, NULL);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}