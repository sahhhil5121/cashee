#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <list>
#include <thread>
#include <mutex>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ================= LRU CACHE =================
class LRUCache {
    int cap;
    list<pair<string,string>> dq;
    unordered_map<string, list<pair<string,string>>::iterator> mp;
    mutex mtx;

public:
    LRUCache(int c): cap(c) {}

    string get(string key) {
        lock_guard<mutex> lock(mtx);

        if (!mp.count(key)) return "NOT_FOUND";

        auto it = mp[key];
        string val = it->second;

        dq.erase(it);
        dq.push_front({key, val});
        mp[key] = dq.begin();

        return val;
    }

    void put(string key, string val) {
        lock_guard<mutex> lock(mtx);

        if (mp.count(key)) {
            dq.erase(mp[key]);
        } 
        else if (dq.size() == cap) {
            auto last = dq.back();
            mp.erase(last.first);
            dq.pop_back();
        }

        dq.push_front({key, val});
        mp[key] = dq.begin();
    }

    bool exists(string key) {
        lock_guard<mutex> lock(mtx);
        return mp.count(key);
    }

    void removeKey(string key) {
        lock_guard<mutex> lock(mtx);
        if (mp.count(key)) {
            dq.erase(mp[key]);
            mp.erase(key);
        }
    }

    // ✅ NEW FUNCTION: get all cache data
    string getAllData() {
        lock_guard<mutex> lock(mtx);

        string result = "Cache Data: ";
        for (auto &p : dq) {
            result += "(" + p.first + "," + p.second + ") ";
        }

        return result;
    }
};

// ================= CALL SERVER =================
string callServer(string req) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9000);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&serv, sizeof(serv));

    send(sock, req.c_str(), req.size(), 0);

    char buffer[1024] = {0};
    recv(sock, buffer, 1024, 0);

    closesocket(sock);
    return string(buffer);
}

// ================= PROXY =================
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    cout << "Proxy running on port 8080...\n";

    LRUCache cache(3);

    while (true) {
        SOCKET client = accept(server_fd, NULL, NULL);

        thread([client, &cache]() {

            char buffer[1024] = {0};
            recv(client, buffer, 1024, 0);

            string cmd, key, value;
            stringstream ss(buffer);
            ss >> cmd >> key >> value;

            string response;

            if (cmd == "GET") {
                string val = cache.get(key);

                if (val != "NOT_FOUND") {
                    response = "From Cache: " + val;
                } else {
                    string serverRes = callServer("GET " + key);

                    if (serverRes != "NOT_FOUND") {
                        cache.put(key, serverRes);
                        response = "From Server: " + serverRes;
                    } else {
                        response = "NOT_FOUND";
                    }
                }
            }
            else if (cmd == "SET") {
                callServer("SET " + key + " " + value);

                if (cache.exists(key)) {
                    cache.put(key, value);
                    response = "Updated Cache + Server";
                } else {
                    response = "Updated Server only";
                }
            }
            else if (cmd == "DELETE") {
                cache.removeKey(key);
                callServer("DELETE " + key);
                response = "Deleted";
            }
            else if (cmd == "SHOW") {
                response = callServer("SHOW");
            }
            // ✅ NEW COMMAND
            else if (cmd == "CACHE") {
                response = cache.getAllData();
            }
            else {
                response = "Invalid Command";
            }

            send(client, response.c_str(), response.size(), 0);
            closesocket(client);

        }).detach();
    }

    WSACleanup();
}