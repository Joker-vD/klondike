#ifndef TTY_H_
#define TTY_H_

#include "config.h"

#include <stdbool.h>
#include <stdalign.h>

#include "lbio.h"

typedef struct TtySize {
    unsigned short lines;
    unsigned short cols;
} TtySize;

bool get_tty_size(int fd, TtySize *size);

typedef struct TtyCookie {
    alignas(TTY_COOKIE_ALIGNMENT) unsigned char blob[TTY_COOKIE_SIZE];
} TtyCookie;

bool enter_visual_mode(LineBuffer *lb, TtyCookie *cookie);
bool leave_visual_mode(LineBuffer *lb, const TtyCookie *cookie);

bool drop_into_cooked_mode(LineBuffer *lb);
bool drop_out_of_cooked_mode(LineBuffer *lb);

#endif // TTY_H_
