#include <X11/Xlib.h>
#include <csignal>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <chrono>

#define PORT 12667
#define F1 67
#define KEYCODE F1

static int sockfd, client_sockfd;

void on_keypress() {
    char buf = 1;
    assert(write(client_sockfd, &buf, 1) >= 0);

    char msg [11];
    using sysclock = std::chrono::system_clock;
    std::time_t t = sysclock::to_time_t(sysclock::now());
    std::tm* local = std::localtime(&t);
    std::strftime(msg, 11, "[%H:%M:%S]", local);
    std::cout << msg << " Notify sent\n";
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

    struct sockaddr_in cli_addr;
    socklen_t client_len = sizeof(cli_addr);
    client_sockfd = accept(sockfd, 
            (struct sockaddr*) &cli_addr,
            &client_len
            );
}

int main(int argc, const char** argv) {
    // block SIGPIPE
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    // register SIGINT handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigfillset(&sa.sa_mask);
    sa.sa_handler = [](int) {
        close(sockfd);
        close(client_sockfd);
        exit(0);
    };
    sigaction(SIGINT, &sa, NULL);

    init_sockets();
    key_listener();
}
