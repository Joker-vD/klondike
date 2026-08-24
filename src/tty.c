#include "tty.h"

#include <assert.h>
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

void leave_visual_mode(LineBuffer *lb, const TtyCookie *cookie) {
    if (!lb_isatty(lb)) { return; }

    lb_puts(lb, S("\x1B[?1049l\x1B[?25h"));
    lb_flush(lb);

    RECAST;
    tcsetattr(lb_fileno(lb), TCSANOW, old_tty_mode);
}

bool enter_cooked_mode(LineBuffer *lb, TtyCookie *cookie) {
    if (!lb_isatty(lb)) { return false; }

    RECAST;
    int fd = lb_fileno(lb);
    if (tcgetattr(fd, old_tty_mode) != 0) { return false; }

    struct termios new_tty_mode = *old_tty_mode;
    new_tty_mode.c_lflag |= ECHO | ICANON;
    if (tcsetattr(fd, TCSANOW, &new_tty_mode) != 0) { return false; }

    lb_puts(lb, S("\x1B[?25h"));
    lb_flush(lb);

    return true;
}

 void leave_cooked_mode(LineBuffer *lb, const TtyCookie *cookie) {
    if (!lb_isatty(lb)) { return; }

    lb_puts(lb, S("\x1B[?25l"));
    lb_flush(lb);

    RECAST;
    tcsetattr(lb_fileno(lb), TCSANOW, old_tty_mode);
}
