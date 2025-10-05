#include <winsock2.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <atomic>
#include <iomanip>
#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET clientSocket;
std::atomic<bool> isInQueue(false);
std::atomic<int> queuePosition(0);
std::atomic<int> totalInQueue(0);
HANDLE hConsole;

// 清空当前行
void ClearCurrentLine() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD cursorPosition = csbi.dwCursorPosition;
    cursorPosition.X = 0;
    SetConsoleCursorPosition(hConsole, cursorPosition);
    
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, cursorPosition, &written);
    SetConsoleCursorPosition(hConsole, cursorPosition);
}

// 显示动态排队信息
void DisplayQueueStatus() {
    while (isInQueue) {
        ClearCurrentLine();
        std::cout << "Position in queue: " << queuePosition 
                  << " (Total in queue: " << totalInQueue << ")" 
                  << " - Waiting...";
        std::cout.flush();
        Sleep(1000); // 每秒更新一次
    }
}

DWORD WINAPI ReceiveMessages(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    int recvSize;
    
    while ((recvSize = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[recvSize] = '\0';
        std::string message(buffer);
        
        // 检查是否是排队信息
        if (message.find("Position in queue:") != std::string::npos) {
            isInQueue = true;
            
            // 解析排队信息
            size_t posStart = message.find(":") + 2;
            size_t posEnd = message.find(" ", posStart);
            std::string posStr = message.substr(posStart, posEnd - posStart);
            
            size_t totalStart = message.find(": ", message.find("Total")) + 2;
            size_t totalEnd = message.find(")", totalStart);
            std::string totalStr = message.substr(totalStart, totalEnd - totalStart);
            
            queuePosition = std::stoi(posStr);
            totalInQueue = std::stoi(totalStr);
        } 
        // 检查是否加入聊天室
        else if (message.find("joined the chat room") != std::string::npos) {
            if (isInQueue) {
                isInQueue = false;
                ClearCurrentLine();
                std::cout << message;
            } else {
                std::cout << message;
            }
        }
        else {
            // 普通聊天消息
            if (!isInQueue) {
                std::cout << message;
            }
        }
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    sockaddr_in serverAddr;
    std::string serverIP;
    int port;
    
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    std::cout << "=== Chat Client ===\n";
    std::cout << "Enter server IP: ";
    std::cin >> serverIP;
    std::cout << "Enter server port: ";
    std::cin >> port;
    std::cin.ignore(); // 清除输入缓冲区

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_port = htons(port);

    std::cout << "Connecting to server...\n";
    
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server. Waiting for response...\n";

    // 创建接收消息线程
    HANDLE hRecvThread = CreateThread(NULL, 0, ReceiveMessages, NULL, 0, NULL);
    
    // 如果是排队状态，启动排队显示线程
    HANDLE hQueueThread = NULL;
    if (isInQueue) {
        hQueueThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DisplayQueueStatus, NULL, 0, NULL);
    }

    std::string message;
    bool firstMessage = true;
    
    while (true) {
        if (!isInQueue) {
            if (firstMessage) {
                std::cout << "\nYou are now in the chat room! Type your messages:\n";
                std::cout << "Type '/exit' to quit or '/users' to see online users\n";
                firstMessage = false;
            }
            
            std::cout << "You: ";
            std::getline(std::cin, message);
            
            if (message == "/exit" || message == "/quit") {
                send(clientSocket, message.c_str(), message.length(), 0);
                break;
            }
            
            if (!message.empty()) {
                send(clientSocket, message.c_str(), message.length(), 0);
            }
        } else {
            // 在排队时，可以输入特殊命令
            std::getline(std::cin, message);
            if (message == "/exit" || message == "/quit") {
                send(clientSocket, message.c_str(), message.length(), 0);
                break;
            }
            else if (message == "/status") {
                ClearCurrentLine();
                std::cout << "Current status: Position " << queuePosition 
                          << " of " << totalInQueue << " in queue\n";
            }
        }
    }

    // 清理
    if (hQueueThread) {
        WaitForSingleObject(hQueueThread, 1000);
        CloseHandle(hQueueThread);
    }
    
    WaitForSingleObject(hRecvThread, 1000);
    CloseHandle(hRecvThread);
    
    closesocket(clientSocket);
    WSACleanup();
    
    std::cout << "Disconnected from server.\n";
    return 0;
}