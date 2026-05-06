// client.cpp
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <list>
#include <thread>
#include <mutex>
#include <vector>
#include <winsock2.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    while (true) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in serv{};
        serv.sin_family = AF_INET;
        serv.sin_port = htons(8080);
        serv.sin_addr.s_addr = inet_addr("127.0.0.1");

        connect(sock, (sockaddr*)&serv, sizeof(serv));

        cout << "Enter command: ";
        string input;
        getline(cin, input);

        send(sock, input.c_str(), input.size(), 0);

        char buffer[1024] = {0};
        recv(sock, buffer, 1024, 0);

        cout << "Response: " << buffer << endl;

        closesocket(sock);
    }

    WSACleanup();
}