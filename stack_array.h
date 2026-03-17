#ifndef STACK_ARRAY_H
#define STACK_ARRAY_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct SA_Header {
    size_t cap;
    size_t len;
} SA_Header;

/*
===============================================================================
Internal helpers
===============================================================================
*/

#define sa_hdr(arr) (((SA_Header *)(arr)) - 1)
#define sa_len(arr) (sa_hdr((arr))->len)
#define sa_cap(arr) (sa_hdr((arr))->cap)

#define sa_clear(arr) (sa_hdr((arr))->len = 0)


/*
===============================================================================
Stack array declaration
===============================================================================
*/

#define stack_array(name, T, N)                     \
    struct {                                        \
        SA_Header hdr;                              \
        T data[N];                                  \
    } SA_##name = { { (N), 0 }, {0} };              \
    T *name = SA_##name.data

/*
===============================================================================
Struct field helpers
===============================================================================
*/
// inline fixed arrays
#define sa_field(name, T, N)    \
    SA_Header hdr_##name;       \
    T name[N]


#define sa_field_init(arr)                              \
    do {                                                \
        SA_Header *_h = sa_hdr(arr);                    \
        _h->cap = sizeof(arr) / sizeof((arr)[0]);       \
        _h->len = 0;                                    \
    } while (0)


/*
===============================================================================
Core operations
===============================================================================

    sa_at, sa_peek, and sa_pop are expression-style checked access macros.

    On error they call the installed handler. These handlers are intended
    to abort or otherwise not return. If a handler returns, the value
    produced by the expression is unspecified and must not be relied on.
*/

// Pushes val and returns a pointer to the inserted element.
// Returns NULL only if the overflow handler returns.
#define sa_push(arr, val)                                                 \
    (sa_len(arr) < sa_cap(arr)                                            \
        ? (((arr))[sa_len(arr)] = (val), &((arr))[sa_len(arr)++])         \
        : (sa_overflow_abort(sa_cap(arr)), NULL))

#define sa_pop(arr)                                                       \
    (sa_len(arr) > 0                                                      \
        ? ((arr)[--sa_len(arr)])                                          \
        : (sa_underflow_abort(), (arr)[0]))

#define sa_peek(arr)                                                      \
    (sa_len(arr) > 0                                                      \
        ? ((arr)[sa_len(arr) - 1])                                        \
        : (sa_underflow_abort(), (arr)[0]))

//  sa_at() --> accessing out of bounds triggers overflow/bounds error
#define sa_at(arr, i)                                                     \
    (((i) < sa_len(arr))                                                  \
        ? ((arr)[(i)])                                                    \
        : (sa_overflow_abort(sa_cap(arr)), (arr)[0]))


/*
===============================================================================
Generic append
===============================================================================
*/

#define sa_append(dst, src, count)                                        \
do {                                                                      \
    SA_Header *_h = sa_hdr(dst);                                          \
    size_t _n = (count);                                                  \
                                                                          \
    if (_n > _h->cap - _h->len) {                                         \
        sa_overflow_abort(sa_cap(dst));                                   \
    }                                                                     \
    else {                                                                \
        memcpy((dst) + _h->len, (src), sizeof((dst)[0]) * _n);            \
        _h->len += _n;                                                    \
    }                                                                     \
} while (0)


/*
===============================================================================
String stack arrays
===============================================================================
*/

#define stack_string(name, N) stack_array(name, char, N)

#define ss_field(name, N) sa_field(name, char, N)
#define ss_field_init(arr) sa_field_init(arr)

#define ss_hdr(str) sa_hdr(str)
#define ss_len(str) sa_len(str)
#define ss_cap(str) sa_cap(str)

/*
    String helpers operate on stack_string / ss_field buffers only.
*/

#ifdef __cplusplus
extern "C" {
#endif

void ss_clear(char *str);
void ss_append_n(char *dst, const char *src, size_t n);
void ss_append(char *dst, const char *src);
void ss_pushc(char *str, char c);
void ss_appendf(char *dst, const char *fmt, ...);
void ss_sprintf(char *dst, const char *fmt, ...);


typedef void (*sa_error_overflow_fn)(size_t);
typedef void (*sa_error_underflow_fn)(void);

/*
    sa_at, sa_peek, and sa_pop are expression-style checked access macros.

    On error they call the installed handler. These handlers are intended
    to abort or otherwise not return. If a handler returns, the value
    produced by the expression is unspecified and must not be relied on.
*/
// capacity/bounds violations on upper extent
void sa_overflow_abort(size_t);
// empty-stack access violations
void sa_underflow_abort(void);

sa_error_overflow_fn sa_set_overflow_handler(sa_error_overflow_fn handler);
sa_error_underflow_fn sa_set_underflow_handler(sa_error_underflow_fn handler);

#ifdef __cplusplus
}
#endif
#endif
