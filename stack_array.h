#ifndef STACK_ARRAY_H
#define STACK_ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__cplusplus)
    #define SA_ALIGNOF(T) alignof(T)
    #define SA_ALIGNAS(A) alignas(A)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define SA_ALIGNOF(T) _Alignof(T)
    #define SA_ALIGNAS(A) _Alignas(A)
#elif defined(_MSC_VER)
    #define SA_ALIGNOF(T) __alignof(T)
    #define SA_ALIGNAS(A) __declspec(align(A))
#elif defined(__GNUC__) || defined(__clang__)
    #define SA_ALIGNOF(T) __alignof__(T)
    #define SA_ALIGNAS(A) __attribute__((aligned(A)))
#else
    #define SA_ALIGNOF(T) offsetof(struct { char __c; T __member; }, __member)
    #define SA_ALIGNAS(A)
#endif

#define SA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define SA_ROUND_UP(x, a) ((((x) + (a) - 1) / (a)) * (a))

#define SA_META_GUARD ((size_t)0xA5A5A5A5A5A5A5A5ULL)
#define SA_META_WORDS 2
#define SA_META_ALIGN(T) SA_MAX(SA_ALIGNOF(size_t), SA_ALIGNOF(T))
#define SA_META_SIZE(T) SA_ROUND_UP(sizeof(size_t) * SA_META_WORDS, SA_META_ALIGN(T))

typedef struct SA_Header {
    size_t cap;
    size_t len;
} SA_Header;

/*
===============================================================================
Internal helpers
===============================================================================
*/
#if !defined(SA_DEBUG) && !defined(NDEBUG)
    #define SA_DEBUG
#endif

#if defined(__cplusplus)
extern "C" {
#endif
void sa_invalid_abort(void);
#if defined(__cplusplus)
}
#endif

static inline int sa__is_size_t_aligned(const void *ptr)
{
    size_t align = SA_ALIGNOF(size_t);
    if (align <= 1) {
        return 1;
    }
    if ((align & (align - 1)) == 0) {
        return (((uintptr_t)ptr) & (align - 1)) == 0;
    }
    return (((uintptr_t)ptr) % align) == 0;
}

#if defined(SA_DEBUG)
    static inline void sa__assert_size_t_aligned(const void *ptr)
    {
        if (!sa__is_size_t_aligned(ptr)) {
            sa_invalid_abort();
        }
    }
#else
    static inline void sa__assert_size_t_aligned(const void *ptr)
    {
        (void)ptr;
    }
#endif

// offset must be initialized via stack_array or sa_field_init before use
static inline size_t sa__get_offset(void *arr) {
    unsigned char *data = (unsigned char *)arr;
    size_t *guard_ptr = (size_t *)(data - sizeof(size_t));
    sa__assert_size_t_aligned(guard_ptr);
    size_t guard = *guard_ptr;
    if (guard != SA_META_GUARD) {
        sa_invalid_abort();
    }
    size_t *off_ptr = guard_ptr - 1;
    size_t off = *off_ptr;
    if (off < sizeof(SA_Header)) {
        sa_invalid_abort();
    }
    return off;
}

#define sa_hdr(arr) ((SA_Header *)((char *)(arr) - sa__get_offset(arr)))
#define sa_len(arr) (sa_hdr((arr))->len)
#define sa_cap(arr) (sa_hdr((arr))->cap)

#define sa_clear(arr) (sa_hdr((arr))->len = 0)


/*
===============================================================================
Stack array declaration
===============================================================================
*/

#define stack_array(name, T, N)                                              \
    struct SA_##name {                                                       \
        SA_Header hdr;                                                       \
        SA_ALIGNAS(SA_META_ALIGN(T)) unsigned char sa_meta_##name[SA_META_SIZE(T)]; \
        T data[N];                                                           \
    } SA_##name = {0};                                                       \
    T *name = SA_##name.data;                                                \
    do {                                                                     \
        SA_##name.hdr.cap = (N);                                             \
        SA_##name.hdr.len = 0;                                               \
        size_t _off =                                                        \
            offsetof(struct SA_##name, data) -                               \
            offsetof(struct SA_##name, hdr);                                 \
        unsigned char *_meta_end =                                           \
            SA_##name.sa_meta_##name + SA_META_SIZE(T);                      \
        size_t *_guard_ptr = (size_t *)(_meta_end - sizeof(size_t));         \
        sa__assert_size_t_aligned(_guard_ptr);                               \
        size_t *_off_ptr = _guard_ptr - 1;                                   \
        *_guard_ptr = SA_META_GUARD;                                         \
        *_off_ptr = _off;                                                    \
    } while (0)

/*
===============================================================================
Struct field helpers
===============================================================================
*/
// inline fixed arrays
#define sa_field(name, T, N)                                               \
    SA_Header hdr_##name;                                                  \
    SA_ALIGNAS(SA_META_ALIGN(T)) unsigned char sa_meta_##name[SA_META_SIZE(T)]; \
    T name[N]

#define sa_field_init(parent_type, obj, field)                    \
    do {                                                          \
        size_t _off =                                             \
            offsetof(parent_type, field) -                        \
            offsetof(parent_type, hdr_##field);                   \
        unsigned char *_meta = (unsigned char *)(obj).sa_meta_##field;      \
        size_t _meta_size = sizeof(((parent_type *)0)->sa_meta_##field);    \
        unsigned char *_meta_end = _meta + _meta_size;                      \
        size_t *_guard_ptr = (size_t *)(_meta_end - sizeof(size_t));        \
        sa__assert_size_t_aligned(_guard_ptr);                              \
        size_t *_off_ptr = _guard_ptr - 1;                                  \
        *_guard_ptr = SA_META_GUARD;                                        \
        *_off_ptr = _off;                                                   \
        SA_Header *_h =                                           \
            (SA_Header *)((char *)((obj).field) - _off);          \
                                                                  \
        _h->cap = sizeof((obj).field) / sizeof((obj).field[0]);   \
        _h->len = 0;                                              \
    } while (0)

/*
===============================================================================
Core operations
===============================================================================

    sa_at, sa_peek, and sa_pop are expression-style checked access macros.

    On error they call the installed handler. These handlers are intended
    to abort or otherwise not return. If a handler returns, the value
    produced by the expression is unspecified and must not be relied on.

    note: these macros take simple l-values 
        they may evaluate more than once
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
        sa_overflow_abort(_h->cap);                                   \
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
#define ss_field_init(parent_type, obj, field) sa_field_init(parent_type, obj, field)

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
typedef void (*sa_error_invalid_fn)(void);

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
sa_error_invalid_fn sa_set_invalid_handler(sa_error_invalid_fn handler);

#ifdef __cplusplus
}
#endif
#endif
