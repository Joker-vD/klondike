#ifndef LBIO_H_
#define LBIO_H_

#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>

typedef struct LineBufferStorage {
    alignas(int) unsigned char blob[270];
} LineBuffer;

void lb_init_from_fd(LineBuffer *lb, int fd);

int lb_fileno(const LineBuffer *lb);
bool lb_isatty(const LineBuffer *lb);
unsigned short lb_lines(const LineBuffer *lb);
unsigned short lb_cols(const LineBuffer *lb);

void lb_flush(LineBuffer *lb);

void lb_putc(LineBuffer *lb, char ch);
void lb_puts(LineBuffer *lb, const char *str, size_t str_len);
void lb_repc(LineBuffer *lb, char ch, int count);
void lb_reps(LineBuffer *lb, const char *str, size_t str_len, int count);

// If padding is positive, prints max(0, padding - field_width) spaces
void lb_pad_left(LineBuffer *lb, int field_width, int padding);
// If padding is negative, prints max(0, abs(padding) - field_width) spaces
void lb_pad_right(LineBuffer *lb, int field_width, int padding);

int lb_getc(LineBuffer *lb);
// Reads bytes from the input and writes them into the buffer, until either buffer_size bytes are written,
// or LF is written, or EOF/error is encountered. Returns the number of bytes written or, if 0 would be
// returned because of encountering EOF/error early, returns -1 instead.
int lb_gets(LineBuffer *lb, char *buffer, int buffer_size);

#endif // LBIO_H_
