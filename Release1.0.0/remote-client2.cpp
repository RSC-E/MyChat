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
            cout << "��������Ͽ�����" << endl;
            running = false;
            break;
        }
        buffer[bytesReceived] = '\0'; 
        cout << "������: " << buffer << endl;
    }
}

int main() {
    // ��ʼ��Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartupʧ��" << endl;
        return 1;
    }

    // ����Socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "����socketʧ��: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // �û������������ַ�Ͷ˿�
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    
    string host;
    u_short port;
    
    cout << "���������������IP��ַ������: ";
    getline(cin, host);  
    
    cout << "����Ҫ���ӵĶ˿ں�: ";
    cin >> port;
    cin.ignore();  
    
    serverAddr.sin_port = htons(port);
    sockaddr_in tempAddr;
    int tempAddrLen = sizeof(tempAddr);
    if (WSAStringToAddressA(
            (LPSTR)host.c_str(),  // �����IP�ַ���
            AF_INET,              // IPv4��ַ��
            NULL,                 // ��Э����Ϣ
            (LPSOCKADDR)&tempAddr, // �����ַ�ṹ
            &tempAddrLen          // ��ַ�ṹ����
        ) == 0) {
        // �����ɹ���ֱ��ʹ��IP��ַ
        serverAddr.sin_addr = tempAddr.sin_addr;
    } else {
        hostent* hostEntry = gethostbyname(host.c_str());
        if (hostEntry == nullptr) {
            cerr << "�޷�����������/IP��ַ: " << WSAGetLastError() << endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        serverAddr.sin_addr = *((in_addr*)hostEntry->h_addr);
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "���ӷ�����ʧ��: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "�����ӵ������������Կ�ʼ�����ˣ�" << endl;
    cout << "������Ϣ������ 'exit' �˳���:" << endl;

    thread receiver(ReceiveMessages, sock);
    receiver.detach(); 

    string message;
    while (running) {
        getline(cin, message);
        
        if (message == "exit") {
            running = false;
            break;
        }
        
        // ������Ϣ��������
        if (send(sock, message.c_str(), message.size() + 1, 0) == SOCKET_ERROR) {
            cerr << "������Ϣʧ��: " << WSAGetLastError() << endl;
            running = false;
            break;
        }
    }
    closesocket(sock);
    WSACleanup();
    return 0;
}
