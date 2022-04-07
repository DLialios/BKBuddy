#include <ws2tcpip.h>
#include <cassert>
#include <iostream>
#include <chrono>

#pragma comment (lib, "Ws2_32.lib")

char* time_msg = new char[11];

void update_time() {
    using namespace std;
    using sysclock = chrono::system_clock;
    time_t t = sysclock::to_time_t(sysclock::now());
    tm* local = localtime(&t);
    strftime(time_msg, 11, "[%H:%M:%S]", local);
}

SOCKET init_socket(const char* host, const char* port) {
    WSAData wsadata;
    assert(!WSAStartup(MAKEWORD(2, 2), &wsadata));

    struct addrinfo hints, *result = nullptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(host, port, &hints, &result)) {
        update_time();
        std::cerr << time_msg << " init_socket: getaddrinfo failed\n";
        WSACleanup();
        exit(1);
    }

    SOCKET sock = socket(
            result->ai_family,
            result->ai_socktype,
            result->ai_protocol
            );
    if (sock == INVALID_SOCKET) {
        update_time();
        std::cerr << time_msg << " init_socket: socket init failure\n";
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if (sock == INVALID_SOCKET) {
        update_time();
        std::cerr << time_msg << " init_socket: socket connect failure\n";
        WSACleanup();
        exit(1);
    }

    return sock;
}

int main(int argc, char** argv) {

    auto bail = [] {
        std::cout << "Exit: invalid args\n";
        return 1;
    };

    if (argc != 4) bail();
    unsigned short vk = strtoul(argv[3], nullptr, 16);
    if (vk < 0x01 || vk > 0xFE) bail();

    update_time();
    std::cout << time_msg << " Initiating connection...\n";

    SOCKET sock = init_socket(argv[1],argv[2]);

    update_time();
    std::cout << time_msg << " Handshake with remote, blocking on recv\n";

    INPUT inputs[2];
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    char n, buf;
    do {
        buf = -1;
        n = recv(sock, &buf, 1, 0);
        if (static_cast<int>(buf) == 1) {
            UINT count = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
            update_time();
            std::cout << time_msg;
            if (count < ARRAYSIZE(inputs))
                std::cout << " Insertion into kbd inputstream was blocked\n";
            else
                std::cout << " Key " << vk << " pressed\n";
        }
    } while (n > 0);

    closesocket(sock);
    WSACleanup();
    update_time();
    std::cout << time_msg << " Exit: remote closed connection\n";
    return 0;
}
