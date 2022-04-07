#include <X11/Xlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <cassert>

#define PORT 27015
#define KEEPALIVE 120s
#define F1 67
#define KEYCODE F1

static char time_msg[11];
static int sockfd, client_sockfd;
static std::mutex cfd_mutex;
static struct sockaddr_in cli_addr;

void update_time() {
    using namespace std;
    using sclk = chrono::system_clock;
    time_t t = sclk::to_time_t(sclk::now());
    tm* local = localtime(&t);
    strftime(time_msg, 11, "[%H:%M:%S]", local);
}

void on_keypress() {
    const std::lock_guard<std::mutex> lock(cfd_mutex);

    char buf = 1;
    assert(write(client_sockfd, &buf, 1) >= 0);

    update_time();
    std::cout << time_msg << " Notify sent\n";
}

void key_listener() {
    Display* dsp = XOpenDisplay(nullptr);
    assert(dsp);
    Window root = DefaultRootWindow(dsp);

    XSetErrorHandler([](Display*,XErrorEvent*){return 0;});

    Window curr_win;
    int curr_foc_state;
    long event_mask = KeyPressMask | FocusChangeMask;

    XGetInputFocus(dsp, &curr_win, &curr_foc_state);
    XSelectInput(dsp, curr_win, event_mask);

    XEvent e;
    while (true) {
        XNextEvent(dsp, &e);
        switch (e.type) {
            case FocusOut:
                XGetInputFocus(dsp, &curr_win, &curr_foc_state);
                XSelectInput(dsp, curr_win, event_mask);
                break;
            case KeyPress:
                if (e.xkey.keycode == KEYCODE)
                    on_keypress();
                break;
        }
    }
}

void init_sockets() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    assert(bind(sockfd,
                (struct sockaddr*) &serv_addr,
                sizeof(serv_addr)) >= 0);

    listen(sockfd, 5);

    socklen_t client_len = sizeof(cli_addr);
    client_sockfd = accept(sockfd, 
            (struct sockaddr*) &cli_addr,
            &client_len
            );
    assert(client_sockfd >= 0);
}

int main(int argc, const char** argv) {
    // block SIGPIPE
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    init_sockets();

    char addr[16];
    inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, addr, sizeof(addr));
    update_time();
    std::cout 
        << time_msg
        << " Connection from "
        << addr << ':' << cli_addr.sin_port << '\n';

    auto keepalive = []{
        using namespace std::chrono_literals;

        char buf = 2;
        while (true) {
            std::this_thread::sleep_for(KEEPALIVE);

            const std::lock_guard<std::mutex> lock(cfd_mutex);

            assert(write(client_sockfd, &buf, 1) >= 0);
        }
    };
    std::thread(keepalive).detach();

    key_listener();
}
