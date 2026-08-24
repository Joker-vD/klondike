#ifndef TTY_H_
#define TTY_H_

#include "config.h"

#include <stdbool.h>
#include <stdalign.h>

#include "lbio.h"

typedef struct TtyCookie {
    alignas(TTY_COOKIE_ALIGNMENT) unsigned char blob[TTY_COOKIE_SIZE];
} TtyCookie;

bool enter_visual_mode(LineBuffer *lb, TtyCookie *cookie);
void leave_visual_mode(LineBuffer *lb, const TtyCookie *cookie);

bool enter_cooked_mode(LineBuffer *lb, TtyCookie *cookie);
void leave_cooked_mode(LineBuffer *lb, const TtyCookie *cookie);

#endif // TTY_H_
