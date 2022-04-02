#include <X11/Xlib.h>
#include <cassert>
#include <iostream>

#define F1 67
#define KEYCODE F1

int main() {
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
                if (e.xkey.keycode == KEYCODE) {
                    std::cout << '@';
                    std::flush(std::cout);
                }
                break;
        }
    }
}
