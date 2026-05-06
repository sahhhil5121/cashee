// server.cpp
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
#include <mysql.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ---------- CONNECT DB ----------
MYSQL* connectDB() {
    MYSQL* conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, "localhost", "root", "MYSQL8044",
                            "proxydb", 0, NULL, 0)) {
        cout << "DB Connection Failed\n";
        exit(1);
    }
    return conn;
}

// ---------- GET ----------
string fetch(string key) {
    MYSQL* conn = connectDB();

    string query = "SELECT v FROM kvstore WHERE k='" + key + "'";
    mysql_query(conn, query.c_str());

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    string ans = (row ? row[0] : "NOT_FOUND");

    mysql_free_result(res);
    mysql_close(conn);
    return ans;
}

// ---------- SET ----------
void updateDB(string key, string val) {
    MYSQL* conn = connectDB();

    string query =
        "INSERT INTO kvstore VALUES('" + key + "','" + val + "') "
        "ON DUPLICATE KEY UPDATE v='" + val + "'";

    mysql_query(conn, query.c_str());
    mysql_close(conn);
}

// ---------- DELETE ----------
void deleteDB(string key) {
    MYSQL* conn = connectDB();

    string query = "DELETE FROM kvstore WHERE k='" + key + "'";
    mysql_query(conn, query.c_str());

    mysql_close(conn);
}

// ---------- SHOW ALL ----------
string showAll() {
    MYSQL* conn = connectDB();

    string query = "SELECT k,v FROM kvstore";
    mysql_query(conn, query.c_str());

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;

    string output = "Server Data: ";

    while ((row = mysql_fetch_row(res))) {
        output += "(" + string(row[0]) + "," + string(row[1]) + ") ";
    }

    mysql_free_result(res);
    mysql_close(conn);
    return output;
}

// ---------- MAIN ----------
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(9000);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    cout << "MySQL Server running on port 9000...\n";

    while (true) {
        SOCKET sock = accept(server_fd, NULL, NULL);

        char buffer[1024] = {0};
        recv(sock, buffer, 1024, 0);

        string cmd, key, value;
        stringstream ss(buffer);
        ss >> cmd >> key >> value;

        string response;

        if (cmd == "GET") response = fetch(key);
        else if (cmd == "SET") { updateDB(key, value); response = "OK"; }
        else if (cmd == "DELETE") { deleteDB(key); response = "DELETED"; }
        else if (cmd == "SHOW") response = showAll();
        else response = "INVALID";

        send(sock, response.c_str(), response.size(), 0);
        closesocket(sock);
    }

    WSACleanup();
}