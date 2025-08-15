#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class ChatServer {
public:
    ChatServer() : is_running_(false) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw runtime_error("WSAStartup failed");
        }
    }

    ~ChatServer() {
        stop();
        WSACleanup();
    }

    void start(unsigned short port) {
        server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket_ == INVALID_SOCKET) {
            throw runtime_error("Socket creation failed");
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(server_socket_);
            throw runtime_error("Bind failed on port " + to_string(port));
        }

        if (listen(server_socket_, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(server_socket_);
            throw runtime_error("Listen failed");
        }

        is_running_ = true;
        accept_thread_ = thread(&ChatServer::accept_connections, this);
    }

    void stop() {
        is_running_ = false;
        closesocket(server_socket_);

        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }

        lock_guard<mutex> lock(clients_mutex_);
        for (auto& client : clients_) {
            closesocket(client);
        }
        clients_.clear();
    }

private:
    void accept_connections() {
        while (is_running_) {
            sockaddr_in client_addr{};
            int addr_len = sizeof(client_addr);
            SOCKET client_socket = accept(server_socket_, 
                                        (sockaddr*)&client_addr, 
                                        &addr_len);

            if (client_socket == INVALID_SOCKET) {
                if (is_running_) {
                    cerr << "Accept error: " << WSAGetLastError() << endl;
                }
                continue;
            }

            {
                lock_guard<mutex> lock(clients_mutex_);
                clients_.push_back(client_socket);
            }

            thread(&ChatServer::handle_client, this, client_socket).detach();
        }
    }

    void handle_client(SOCKET client_socket) {
    char buffer[1024];
    time_t last_active = time(nullptr);
    
    while (is_running_) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(client_socket, &read_set);
        timeval timeout{0, 300000}; 
        
        int sel = select(0, &read_set, nullptr, nullptr, &timeout);
        if (sel == SOCKET_ERROR) break;
        
        if (sel > 0 && FD_ISSET(client_socket, &read_set)) {
            int bytes_received = recv(client_socket, buffer, sizeof(buffer)-1, 0);
            if (bytes_received <= 0) break;
            
            buffer[bytes_received] = '\0';
            last_active = time(nullptr);
            if (strcmp(buffer, "HEARTBEAT") != 0) {
                broadcast_message(client_socket, buffer, bytes_received);
            }
        }
        
        if (time(nullptr) - last_active > 30) {
            if (send(client_socket, "HEARTBEAT", 9, 0) == SOCKET_ERROR) {
                break;
            }
            last_active = time(nullptr);
        }
    }
    
    remove_client(client_socket);
    closesocket(client_socket);
}

    void broadcast_message(SOCKET sender, const char* message, size_t length) {
    lock_guard<mutex> lock(clients_mutex_);
    auto it = clients_.begin();
    while (it != clients_.end()) {
        if (*it != sender) {
            if (send(*it, message, length, 0) == SOCKET_ERROR) {
                closesocket(*it);
                it = clients_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void remove_client(SOCKET client_socket) {
    lock_guard<mutex> lock(clients_mutex_);
    auto it = find(clients_.begin(), clients_.end(), client_socket);
    if (it != clients_.end()) {
        closesocket(*it);
        clients_.erase(it);
    }
}

    SOCKET server_socket_ = INVALID_SOCKET;
    vector<SOCKET> clients_;
    mutex clients_mutex_;
    thread accept_thread_;
    atomic<bool> is_running_;
};

int main() {
    try {
        ChatServer server;
        unsigned short port;
        cout << "输入服务器端口号: ";
        cin >> port;
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
        server.start(port);
        cout << "服务器已启动，按回车键停止..." << endl;
        cin.get();
        
        server.stop();
        cout << "服务器已停止" << endl;
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
        return 1;
    }
    return 0;
}
