#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>  // 确保包含这个头文件
#include <thread>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

bool running = true;

void ReceiveMessages(SOCKET sock) {
    char buffer[1024];
    int bytesReceived;
    
    while (running) {
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
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
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup失败" << endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "创建socket失败" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    u_short loadport;
    cout<<"输入要连接的房间号";
    cin>>loadport;
    serverAddr.sin_port = htons(loadport);
    
    // 修正：使用inet_addr替代inet_pton以兼容旧版Windows
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "连接服务器失败" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "已连接到服务器，可以开始聊天了！" << endl;
    cout << "输入消息(输入'exit'退出):" << endl;

    thread receiver(ReceiveMessages, sock);
    receiver.detach();

    string message;
    while (running) {
        getline(cin, message);
        
        if (message == "exit") {
            running = false;
            break;
        }
        
        send(sock, message.c_str(), message.size() + 1, 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
