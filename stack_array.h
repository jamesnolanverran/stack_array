#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if defined(__cplusplus)
#define SA_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#else
#define SA_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#endif

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
    SA_STATIC_ASSERT((N) > 0, "stack_array size must be greater than zero"); \
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
    SA_STATIC_ASSERT((N) > 0, "sa_field size must be greater than zero"); \
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

//  sa_at() --> accessing out of bounds triggers overflow error
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
    if (_h->len + _n > _h->cap) {                                         \
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


#define ss_clear(str)                                                 \
do {                                                                  \
    sa_clear(str);                                                    \
    (str)[0] = '\0';                                                  \
} while (0)


/*
===============================================================================
String append (strlen based)
===============================================================================
*/

#define ss_append(dst, src)                                               \
do {                                                                      \
    SA_Header *_h = sa_hdr(dst);                                          \
    size_t _n = strlen(src);                                              \
                                                                          \
    if (_h->len + _n + 1 > _h->cap) {                                     \
        sa_overflow_abort(sa_cap(dst));                                   \
    }                                                                     \
    else {                                                                \
        memcpy((dst) + _h->len, (src), _n);                               \
        _h->len += _n;                                                    \
        (dst)[_h->len] = '\0';                                            \
    }                                                                     \
} while (0)

#define ss_append_n(dst, src, n)                                          \
do {                                                                      \
    SA_Header *_h = sa_hdr(dst);                                          \
    size_t _n = (n);                                                      \
                                                                          \
    if (_h->len + _n + 1 > _h->cap) {                                     \
        sa_overflow_abort(sa_cap(dst));                                   \
    }                                                                     \
    else {                                                                \
        memcpy((dst) + _h->len, (src), _n);                               \
        _h->len += _n;                                                    \
        (dst)[_h->len] = '\0';                                            \
    }                                                                     \
} while (0)


#define ss_append_lit(dst, lit)                                           \
do {                                                                      \
    SA_Header *_h = sa_hdr(dst);                                          \
    size_t _n = sizeof(lit) - 1;                                          \
                                                                          \
    if (_h->len + _n + 1 > _h->cap) {                                     \
        sa_overflow_abort(sa_cap(dst));                                   \
    }                                                                     \
    else {                                                                \
        memcpy((dst) + _h->len, (lit), _n);                               \
        _h->len += _n;                                                    \
        (dst)[_h->len] = '\0';                                            \
    }                                                                     \
} while (0)


/*
===============================================================================
Formatted append
===============================================================================
*/

#define ss_appendf(dst, fmt, ...)                                         \
do {                                                                      \
    SA_Header *_h = sa_hdr(dst);                                          \
    int _needed = snprintf(NULL, 0, fmt __VA_OPT__(,) __VA_ARGS__);       \
                                                                          \
    if (_needed < 0) {                                                    \
        sa_overflow_abort(sa_cap(dst));                                   \
    }                                                                     \
    else {                                                                \
        size_t _n = (size_t)_needed;                                      \
                                                                          \
        if (_h->len + _n + 1 > _h->cap) {                                 \
            sa_overflow_abort(sa_cap(dst));                               \
        }                                                                 \
        else {                                                            \
            snprintf((dst) + _h->len, _h->cap - _h->len,                  \
                     fmt __VA_OPT__(,) __VA_ARGS__);                      \
            _h->len += _n;                                                \
        }                                                                 \
    }                                                                     \
} while (0)

#define ss_sprintf(dst, fmt, ...)                                         \
do {                                                                      \
    SA_Header *_h = sa_hdr(dst);                                          \
    int _needed = snprintf(NULL, 0, fmt __VA_OPT__(,) __VA_ARGS__);       \
                                                                          \
    if (_needed < 0) {                                                    \
        sa_overflow_abort(sa_cap(dst));                                   \
    }                                                                     \
    else {                                                                \
        size_t _n = (size_t)_needed;                                      \
                                                                          \
        if (_n + 1 > _h->cap) {                                           \
            sa_overflow_abort(sa_cap(dst));                               \
        }                                                                 \
        else {                                                            \
            snprintf((dst), _h->cap, fmt __VA_OPT__(,) __VA_ARGS__);      \
            _h->len = _n;                                                 \
        }                                                                 \
    }                                                                     \
} while (0)

/*
===============================================================================
Character push
===============================================================================
*/

#define ss_pushc(str, c)                                                  \
do {                                                                      \
    SA_Header *_h = sa_hdr(str);                                          \
                                                                          \
    if (_h->len + 2 > _h->cap) {                                          \
        sa_overflow_abort(_h->cap);                                       \
    }                                                                     \
    else {                                                                \
        (str)[_h->len++] = (c);                                           \
        (str)[_h->len] = '\0';                                            \
    }                                                                     \
} while (0)

#ifdef __cplusplus
extern "C" {
#endif

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
