#include "lbio.h"
#include "tty.h"

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

sig_atomic_t lb_termination_pending;

typedef struct LineBufferImpl {
    int fd;
    bool isatty;
    TtySize tty_size;
    short offset;
    short end;
    char buffer[256];
} LineBufferImpl;

static_assert(sizeof(LineBufferImpl) == sizeof(LineBuffer), "LineBuffer has wrong size");
static_assert(alignof(LineBufferImpl) <= alignof(LineBuffer), "LineBuffer has wrong alignment");

// This is probably still UB but I just can't bring myself to memcpy-ing structs to and fro.
typedef union LineBufferUnion {
    LineBuffer ds;
    LineBufferImpl lb;
} LineBufferUnion;

#define RECAST LineBufferImpl *lb = &((LineBufferUnion *)blob)->lb

void lb_init_from_fd(LineBuffer *blob, int fd) {
    RECAST;
    lb->fd = fd;
    lb->isatty = isatty(fd);
    if (!lb->isatty || !get_tty_size(fd, &lb->tty_size)) {
        lb->tty_size.lines = lb->tty_size.cols = 0;
    }
    lb->offset = lb->end = 0;
}

int lb_fileno(const LineBuffer *blob) {
    const RECAST;
    return lb->fd;
}

bool lb_isatty(const LineBuffer *blob) {
    const RECAST;
    return lb->isatty;
}

unsigned short lb_lines(const LineBuffer *blob) {
    const RECAST;
    return lb->tty_size.lines;
}

unsigned short lb_cols(const LineBuffer *blob) {
    const RECAST;
    return lb->tty_size.cols;
}

void lb_flush(LineBuffer *blob) {
    RECAST;
    for (short offset = 0, count = lb->offset; count; ) {
        int wrote = write(lb->fd, &lb->buffer[offset], count);

        if (wrote < 0) {
            if (errno == EINTR) {
                wrote = 0;
                continue;
            }

            break;
        }

        offset += wrote;
        count -= wrote;
    }

    lb->offset = 0;
}

void lb_putc(LineBuffer *blob, char ch) {
    RECAST;
    lb->buffer[lb->offset++] = ch;
    if (lb->offset == sizeof(lb->buffer) || ch == '\n') { lb_flush(blob); }
}

void lb_puts(LineBuffer *blob, const char *str, size_t str_len) {
    for (size_t i = 0; i < str_len; i++) {
        lb_putc(blob, str[i]);
    }
}

void lb_repc(LineBuffer *blob, char ch, int count) {
    if (ch == '\n') {
        for (int i = 0; i < count; i++) {
            lb_putc(blob, ch);
        }
        return;
    }

    RECAST;
    while (count > 0) {
        int chunk = sizeof(lb->buffer) - lb->offset;
        if (chunk > count) { chunk = count; }

        memset(&lb->buffer[lb->offset], ch, chunk);
        lb->offset += chunk;
        count -= chunk;

        if (lb->offset == sizeof(lb->buffer)) { lb_flush(blob); }
    }
}

void lb_reps(LineBuffer *blob, const char *str, size_t str_len, int count) {
    for (int i = 0; i < count; i++) {
        lb_puts(blob, str, str_len);
    }
}

void lb_pad_left(LineBuffer *blob, int field_width, int padding) {
    if (padding > 0) {
        lb_repc(blob, '\x20', padding - field_width);
    }
}

void lb_pad_right(LineBuffer *blob, int field_width, int padding) {
    if (padding < 0) {
        lb_repc(blob, '\x20', -padding - field_width);
    }
}

bool lb_fill_rdbuf(LineBuffer *blob) {
    RECAST;
    lb->offset = lb->end = 0;

    while (true) {
        int count = read(lb->fd, lb->buffer, sizeof(lb->buffer));

        if (count < 0 && errno == EINTR && lb_termination_pending == 0) {
            continue;
        }
        if (count <= 0) {
            return false;
        }

        lb->end = count;
        return true;
    }
}

int lb_getc(LineBuffer *blob) {
    RECAST;

    if (lb->offset == lb->end) {
        if (!lb_fill_rdbuf(blob)) {
            return -1;
        }
    }

    return (unsigned char)lb->buffer[lb->offset++];
}

int lb_gets(LineBuffer *blob, char *buffer, int buffer_size) {
    RECAST;
    if (buffer_size < 0) { return -1; }
    if (buffer_size == 0) { return 0; }

    int total_read_count = 0;

    do {
        if (lb->offset == lb->end) {
            if (!lb_fill_rdbuf(blob)) {
                if (total_read_count == 0) { total_read_count = -1; }
                break;
            }
        }

        int len = lb->end - lb->offset;
        if (len > buffer_size - total_read_count) { len = buffer_size - total_read_count; }

        char *newline = memchr(&lb->buffer[lb->offset], '\n', len);
        if (newline != NULL) {
            newline++;
            len = newline - &lb->buffer[lb->offset];
            memcpy(buffer, &lb->buffer[lb->offset], len);
            lb->offset += len;
            total_read_count += len;
            break;
        } else {
            memcpy(buffer, &lb->buffer[lb->offset], len);
            buffer += len;
            lb->offset += len;
            total_read_count += len;
        }
    } while(total_read_count < buffer_size);

    return total_read_count;
}
