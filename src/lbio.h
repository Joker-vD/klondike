#ifndef LBIO_H_
#define LBIO_H_

#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>

typedef struct LineBufferStorage {
    alignas(int) unsigned char blob[268];
} LineBuffer;

void lb_init_from_fd(LineBuffer *lb, int fd);

bool lb_isatty(LineBuffer *lb);

void lb_flush(LineBuffer *lb);
void lb_abort(LineBuffer *lb);

void lb_putc(LineBuffer *lb, char ch);
void lb_puts(LineBuffer *lb, const char *str, size_t str_len);

// If padding is positive, prints max(0, padding - field_width) spaces
void lb_pad_left(LineBuffer *lb, int field_width, int padding);
// If padding is negative, prints max(0, abs(padding) - field_width) spaces
void lb_pad_right(LineBuffer *lb, int field_width, int padding);

// Reads bytes from the input and writes them into the buffer, until either buffer_size bytes are written,
// or LF is written, or EOF/error is encountered. Returns the number of bytes written or, if 0 would be
// returned because of encountering EOF/error early, returns -1 instead.
int lb_gets(LineBuffer *lb, char *buffer, int buffer_size);

#endif // LBIO_H_
