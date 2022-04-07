#include <X11/Xlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sstream>
#include <thread>

#define TIME_FMT   "[%H:%M:%S]"
#define PORT       27015
#define KEEPALIVE  120s
#define KEYCODE    67

#define XSTR(s)  STR(s)
#define STR(s)   #s

#define bail_errno(expr, str) if (expr == -1) {\
    update_time();\
    fprintf(stderr, "%s %s: %s\n", time_msg, str, strerror(errno));\
    exit(1);\
}

typedef struct sockaddr_in  sockaddr_in;
typedef struct sockaddr     sockaddr;

const char  MSG_PRES = 1;
const char  MSG_ALIV = 2;
char*       time_msg;
int         tmsg_len;
int         sockfd;
int         csockfd;
std::mutex  cfd_mut;

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

void xevent_loop() {
    Display* dsp = XOpenDisplay(nullptr);
    if (!dsp) {
        pmsg("xevent_loop: failed to open display", stderr);
        exit(1);
    }
    Window root = DefaultRootWindow(dsp);

    XSetErrorHandler([](Display*, XErrorEvent*){return 0;});

    Window curr_win;
    int focus;
    long event_mask = KeyPressMask | FocusChangeMask;

    XGetInputFocus(dsp, &curr_win, &focus);
    XSelectInput(dsp, curr_win, event_mask);

    XEvent e;
    while (true) {
        XNextEvent(dsp, &e);
        switch (e.type) {
            case FocusOut:
                XGetInputFocus(dsp, &curr_win, &focus);
                XSelectInput(dsp, curr_win, event_mask);
                break;
            case KeyPress:
                if (e.xkey.keycode == KEYCODE) {
                    std::lock_guard lock(cfd_mut);
                    bail_errno(write(csockfd, &MSG_PRES, 1), "xevent_loop");
                    pmsg("Notify sent", stdout);
                }
                break;
        }
    }
}

void init_sockets() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bail_errno(sockfd, "init_sockets");

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    int bres = bind(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr));
    bail_errno(bres, "init_sockets");

    listen(sockfd, 5);
    pmsg("Listening on " XSTR(PORT) "...", stdout);

    sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    csockfd = accept(sockfd, (sockaddr*) &cli_addr, &cli_len);
    bail_errno(csockfd, "init_sockets");

    char addr[16];
    inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, addr, sizeof(addr));
    std::ostringstream os;
    os << "Connection from " << addr << ':' << cli_addr.sin_port;
    pmsg(os.str().c_str(), stdout);
}

int main(int argc, const char** argv) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    tmsg_len = strlen(TIME_FMT) + 1;
    time_msg = new char[tmsg_len];

    init_sockets();

    auto keepalive = []{
        using namespace std::chrono_literals;

        while (true) {
            std::this_thread::sleep_for(KEEPALIVE);

            std::lock_guard lock(cfd_mut);

            bail_errno(write(csockfd, &MSG_ALIV, 1), "keepalive");
        }
    };
    std::thread(keepalive).detach();

    xevent_loop();
}
