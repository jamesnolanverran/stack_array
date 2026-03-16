#include "stack_array.h"

#include <stdio.h>
#include <stdlib.h>

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
