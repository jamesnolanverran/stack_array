#include "stack_array.h"

#include <stdarg.h>
#include <stdlib.h>

static int sa_vcountf(const char *fmt, va_list args)
{
    va_list args_copy;
    int needed;

    va_copy(args_copy, args);
#if defined(_MSC_VER)
    needed = _vscprintf(fmt, args_copy);
#else
    needed = vsnprintf(NULL, 0, fmt, args_copy);
#endif
    va_end(args_copy);
    return needed;
}

void ss_clear(char *str) {
    SA_Header *h = sa_hdr(str);
    h->len = 0;
    str[0] = '\0';
}

void ss_append_n(char *dst, const char *src, size_t n) {
    SA_Header *h = sa_hdr(dst);

    size_t rem = h->cap - h->len;
    if (rem == 0 || n > rem - 1) {
        sa_overflow_abort(h->cap);
        return;
    }
    memcpy(dst + h->len, src, n);
    h->len += n;
    dst[h->len] = '\0';
}

void ss_append(char *dst, const char *src) {
    ss_append_n(dst, src, strlen(src));
}

void ss_pushc(char *str, char c) {
    SA_Header *h = sa_hdr(str);

    size_t rem = h->cap - h->len;
    if (rem < 2) {
        sa_overflow_abort(h->cap);
        return;
    }
    str[h->len++] = c;
    str[h->len] = '\0';
}

static int ss_vappendf_impl(char *dst, const char *fmt, va_list args) {
    SA_Header *h = sa_hdr(dst);
    int needed = sa_vcountf(fmt, args);

    if (needed < 0) {
        sa_overflow_abort(h->cap);
        return 0;
    }

    size_t rem = h->cap - h->len;
    size_t n = (size_t)needed;
    if (rem == 0 || n > rem - 1) {
        sa_overflow_abort(h->cap);
        return 0;
    }
    {
        va_list args_copy;
        va_copy(args_copy, args);
        (void)vsnprintf(dst + h->len, h->cap - h->len, fmt, args_copy);
        va_end(args_copy);
    }

    h->len += (size_t)needed;
    return 1;
}

static int ss_vsprintf_impl(char *dst, const char *fmt, va_list args) {
    SA_Header *h = sa_hdr(dst);
    int needed = sa_vcountf(fmt, args);

    if (needed < 0) {
        sa_overflow_abort(h->cap);
        return 0;
    }

    size_t n = (size_t)needed;
    if (n >= h->cap) {
        sa_overflow_abort(h->cap);
        return 0;
    }
    va_list args_copy;
    va_copy(args_copy, args);
    (void)vsnprintf(dst, h->cap, fmt, args_copy);
    va_end(args_copy);

    h->len = (size_t)needed;
    return 1;
}

void ss_appendf(char *dst, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    (void)ss_vappendf_impl(dst, fmt, args);
    va_end(args);
}

void ss_sprintf(char *dst, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    (void)ss_vsprintf_impl(dst, fmt, args);
    va_end(args);
}

static void sa_default_overflow(size_t cap)
{
    fprintf(stderr, "stack_array overflow (cap=%zu)\n", cap);
    abort();
}

static void sa_default_underflow(void)
{
    fprintf(stderr, "stack_array underflow\n");
    abort();
}

static sa_error_overflow_fn sa_overflow_handler = sa_default_overflow;
static sa_error_underflow_fn sa_underflow_handler = sa_default_underflow;

sa_error_overflow_fn sa_set_overflow_handler(sa_error_overflow_fn handler)
{
    sa_error_overflow_fn previous = sa_overflow_handler;
    sa_overflow_handler = handler ? handler : sa_default_overflow;
    return previous;
}

sa_error_underflow_fn sa_set_underflow_handler(sa_error_underflow_fn handler)
{
    sa_error_underflow_fn previous = sa_underflow_handler;
    sa_underflow_handler = handler ? handler : sa_default_underflow;
    return previous;
}

void sa_overflow_abort(size_t cap)
{
    sa_overflow_handler(cap);
}

void sa_underflow_abort(void)
{
    sa_underflow_handler();
}
