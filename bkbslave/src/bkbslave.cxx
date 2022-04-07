#include <ws2tcpip.h>
#include <chrono>

#pragma comment (lib, "Ws2_32.lib")

#define TIME_FMT "[%H:%M:%S]"

char*  time_msg;
int    tmsg_len;

void update_time() {
    using namespace std;
    using sclk = chrono::system_clock;
    time_t t = sclk::to_time_t(sclk::now());
    tm* local = localtime(&t);
    strftime(time_msg, tmsg_len, TIME_FMT, local);
}

void pmsg(const char* str, FILE* f) {
    update_time();
    fprintf(f, "%s %s\n", time_msg, str);
}

SOCKET init_socket(const char* host, const char* port) {
    WSAData wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        pmsg("init_socket: WSAStartup failed", stderr);
        exit(1);
    }

    struct addrinfo hints, *result = nullptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(host, port, &hints, &result)) {
        pmsg("init_socket: getaddrinfo failed", stderr);
        WSACleanup();
        exit(1);
    }

    SOCKET sock = socket(
            result->ai_family,
            result->ai_socktype,
            result->ai_protocol
            );
    if (sock == INVALID_SOCKET) {
        pmsg("init_socket: socket init failure", stderr);
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if (sock == INVALID_SOCKET) {
        pmsg("init_socket: socket connect failure", stderr);
        WSACleanup();
        exit(1);
    }

    return sock;
}

int main(int argc, char** argv) {
    tmsg_len = strlen(TIME_FMT) + 1;
    time_msg = new char[tmsg_len];

    auto bail = [] {
        pmsg("Exit: invalid args", stdout);
        exit(1);
    };

    if (argc != 4) bail();
    unsigned short vk = strtoul(argv[3], nullptr, 16);
    if (vk < 0x01 || vk > 0xFE) bail();

    pmsg("Initiating connection...", stdout);
    SOCKET sock = init_socket(argv[1], argv[2]);
    pmsg("Handshake with remote, blocking on recv", stdout);

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
            if (count < ARRAYSIZE(inputs))
                pmsg("Insertion into kbd inputstream was blocked", stdout);
            else
                pmsg("Keypress", stdout);
        }
    } while (n > 0);

    closesocket(sock);
    WSACleanup();
    pmsg("Exit: remote closed connection", stdout);
    return 0;
}
