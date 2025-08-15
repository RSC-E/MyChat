#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>  // ȷ���������ͷ�ļ�
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
            cout << "��������Ͽ�����" << endl;
            running = false;
            break;
        }
        
        buffer[bytesReceived] = '\0';
        cout << "������: " << buffer << endl;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartupʧ��" << endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "����socketʧ��" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    u_short loadport;
    cout<<"����Ҫ���ӵķ����";
    cin>>loadport;
    serverAddr.sin_port = htons(loadport);
    
    // ������ʹ��inet_addr���inet_pton�Լ��ݾɰ�Windows
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "���ӷ�����ʧ��" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "�����ӵ������������Կ�ʼ�����ˣ�" << endl;
    cout << "������Ϣ(����'exit'�˳�):" << endl;

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
