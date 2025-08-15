#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

bool running = true;


void ReceiveMessages(SOCKET sock) {
    char buffer[1024];
    int bytesReceived;
    
    while (running) {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0); 
        if (bytesReceived <= 0) {
            cout << "与服务器断开连接" << endl;
            running = false;
            break;
        }
        buffer[bytesReceived] = '\0'; 
        cout << "其他人: " << buffer << endl;
    }
}

int main() {
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup失败" << endl;
        return 1;
    }

    // 创建Socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "创建socket失败: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // 用户输入服务器地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    
    string host;
    u_short port;
    
    cout << "请输入聊天服务器IP地址或域名: ";
    getline(cin, host);  
    
    cout << "输入要连接的端口号: ";
    cin >> port;
    cin.ignore();  
    
    serverAddr.sin_port = htons(port);
    sockaddr_in tempAddr;
    int tempAddrLen = sizeof(tempAddr);
    if (WSAStringToAddressA(
            (LPSTR)host.c_str(),  // 输入的IP字符串
            AF_INET,              // IPv4地址族
            NULL,                 // 无协议信息
            (LPSOCKADDR)&tempAddr, // 输出地址结构
            &tempAddrLen          // 地址结构长度
        ) == 0) {
        // 解析成功，直接使用IP地址
        serverAddr.sin_addr = tempAddr.sin_addr;
    } else {
        hostent* hostEntry = gethostbyname(host.c_str());
        if (hostEntry == nullptr) {
            cerr << "无法解析主机名/IP地址: " << WSAGetLastError() << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        serverAddr.sin_addr = *((in_addr*)hostEntry->h_addr);
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "连接服务器失败: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "已连接到服务器，可以开始聊天了！" << endl;
    cout << "输入消息（输入 'exit' 退出）:" << endl;

    thread receiver(ReceiveMessages, sock);
    receiver.detach(); 

    string message;
    while (running) {
        getline(cin, message);
        
        if (message == "exit") {
            running = false;
            break;
        }
        
        // 发送消息到服务器
        if (send(sock, message.c_str(), message.size() + 1, 0) == SOCKET_ERROR) {
            cerr << "发送消息失败: " << WSAGetLastError() << endl;
            running = false;
            break;
        }
    }
    closesocket(sock);
    WSACleanup();
    return 0;
}
