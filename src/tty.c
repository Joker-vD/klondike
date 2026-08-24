#include "tty.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "basics.h"

// Terminal-related bullshit

static_assert(sizeof(struct termios) == sizeof(TtyCookie), "TtyCookie has wrong size");
static_assert(alignof(struct termios) <= alignof(TtyCookie), "TtyCookie has wrong alignment");

typedef union TermiosUnion {
    TtyCookie cookie;
    struct termios termios;
} TermiosUnion;

#define RECAST struct termios *old_tty_mode = &((TermiosUnion *)cookie)->termios

bool enter_visual_mode(LineBuffer *lb, TtyCookie *cookie) {
    if (!lb_isatty(lb)) { return false; }

    RECAST;
    int fd = lb_fileno(lb);
    if (tcgetattr(fd, old_tty_mode) != 0) { return false; }

    struct termios new_tty_mode = *old_tty_mode;
    new_tty_mode.c_iflag |= ICRNL;
    new_tty_mode.c_lflag &= ~(ECHO | ICANON);
    new_tty_mode.c_oflag |= OPOST | ONLCR;
    new_tty_mode.c_cc[VMIN] = 1;
    new_tty_mode.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &new_tty_mode) != 0) { return false; }

    // An interesting question: should we write to stdin, or to stdout? What if
    // stdin and stout are two different terminals? Probably best not to even think
    // about this stuff.
    lb_puts(lb, S("\x1B[?1049h\x1B[?25l"));
    lb_flush(lb);

    return true;
}

bool leave_visual_mode(LineBuffer *lb, const TtyCookie *cookie) {
    if (!lb_isatty(lb)) { return false; }

    lb_puts(lb, S("\x1B[?1049l\x1B[?25h"));
    lb_flush(lb);

    RECAST;
    return tcsetattr(lb_fileno(lb), TCSANOW, old_tty_mode) == 0;
}

bool drop_into_cooked_mode(LineBuffer *lb) {
    if (!lb_isatty(lb)) { return false; }

    int fd = lb_fileno(lb);
    struct termios tty_mode;
    if (tcgetattr(fd, &tty_mode) != 0) { return false; }

    tty_mode.c_lflag |= ECHO | ICANON;
    if (tcsetattr(fd, TCSANOW, &tty_mode) != 0) { return false; }

    lb_puts(lb, S("\x1B[?25h"));
    lb_flush(lb);

    return true;
}

bool drop_out_of_cooked_mode(LineBuffer *lb) {
    if (!lb_isatty(lb)) { return false; }

    int fd = lb_fileno(lb);
    struct termios tty_mode;
    if (tcgetattr(fd, &tty_mode) != 0) { return false; }

    tty_mode.c_lflag &= ~(ECHO | ICANON);
    if (tcsetattr(fd, TCSANOW, &tty_mode) != 0) { return false; }

    lb_puts(lb, S("\x1B[?25l"));
    lb_flush(lb);

    return true;
}

#if _POSIX_VERSION < 202405L
#include <sys/ioctl.h>

#define tcgetwinsize(fd, winsize)  ioctl((fd), TIOCGWINSZ, (winsize))
#endif

void atohu(const char *s, unsigned short *p) {
    if (s == NULL || s[0] == 0) { return; }

    unsigned short result = 0;
    while (true) {
        char ch = *s++;
        if (ch == 0) {
            *p = result;
            return;
        }
        if (ch >= '0' && ch <= '9') {
            unsigned short tmp = result * 10 + ch - '0';
            if (tmp < result) {
                return;
            }
            result = tmp;
        } else {
            return;
        }
    }
}

bool get_tty_size(int fd, TtySize *size) {
    if (!isatty(fd)) { return false; }

    struct winsize tty_size;
    if (tcgetwinsize(fd, &tty_size) == 0) {
        size->lines = tty_size.ws_row;
        size->cols = tty_size.ws_col;
    } else {
        size->lines = 24;
        size->cols = 80;
    }

    atohu(getenv("LINES"), &size->lines);
    atohu(getenv("COLUMNS"), &size->cols);

    return true;
}
