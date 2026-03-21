# stack_array

`stack_array` is a small C helper library for fixed-capacity arrays and strings that live on the stack.

The goal is to remove some of the repetitive, error-prone parts of normal C code. Length and capacity are tracked for you, bounds are checked, and string helpers keep the buffer null-terminated.

## What it provides

- fixed-capacity stack arrays with tracked length
- fixed-capacity stack strings with tracked length
- checked push/pop/peek/index operations
- checked append for arrays and strings
- ss_appendf and ss_sprintf for strings
- optional custom overflow / underflow handlers

This is not a heap-backed dynamic array. Capacity is fixed at declaration time.

## Why use it

Stack_array:

```c
stack_string(build_path, 512); 
ss_sprintf(build_path, "build/%s", rel_path);
```

Raw C:

```c
char build_path[512];
int needed = snprintf(NULL, 0, "build/%s", rel_path);

if (needed < 0 || (size_t)needed >= sizeof(build_path)) {
    fprintf(stderr, "error msg\n");
    abort();
}
snprintf(build_path, sizeof(build_path), "build/%s", rel_path)

```

---
## Example: stack array of ints

```c
#include "stack_array.h"

void example(void) {
    stack_array(values, int, 4);

    sa_push(values, 10);
    sa_push(values, 20);
    sa_push(values, 30);

    int x   = sa_pop(values);    // 30
    int top = sa_peek(values);   // 20

    // len/cap are tracked in the hidden header
    size_t len = sa_len(values); // 2
    size_t cap = sa_cap(values); // 4
}
```

## Example: append several values at once

```c
#include "stack_array.h"

void example(void) {
    stack_array(dst, int, 8);

    int src[] = { 1, 2, 3, 4 };
    sa_append(dst, src, 4);

    // dst now contains 1, 2, 3, 4
    // sa_len(dst) == 4
}
```

## Example: stack string

```c
#include "stack_array.h"

void example(void) {
    stack_string(msg, 32);

    ss_append(msg, "hello");
    ss_append(msg, " world");
    ss_pushc(msg, '!');
    ss_appendf(msg, " value=%d", 42);

    // msg is always null-terminated after successful string operations
    // ss_len(msg) matches strlen(msg)
}
```

## Example: overwrite a string with formatted output

```c
#include "stack_array.h"

void example(void) {
    stack_string(path, 64);

    // formatted writes only modify the buffer if the full result fits
    ss_sprintf(path, "/tmp/%s_%d.txt", "log", 7);

    // path == "/tmp/log_7.txt"
}
```

## Example: struct field

You can also embed these arrays directly inside a struct.

```c
#include "stack_array.h"

struct Label {
    int id;
    ss_field(name, 32);
};

void example(void) {
    struct Label label = {0};

    label.id = 7;
    ss_field_init(label, name); // init required to set capacity

    ss_append(label.name, "player");
    ss_pushc(label.name, '_');

    char *name = label.name; // derived pointer works normally after ss_field_init(...)
    ss_appendf(name, "%d", label.id);
}
```
The field lives inside the struct, and after initialization it can be used through a normal pointer.

## How it works (header lookup)

Each array reserves a fixed metadata prefix that sits immediately before the data buffer. The prefix always stores `cap` and `len`, and in debug builds an additional guard word is stored just before them for corruption detection. The prefix is padded as needed so that the data buffer still meets the element’s alignment requirement.

Because the layout is always `[optional padding][guard (debug only)][cap][len][data]`, recovering the header is trivial: subtract `sizeof(SA_Header)` from the array pointer (and, in debug builds, check the guard word). No extra indirection or offset decoding is required, so lookups are simple and cheap.

## API summary

### Arrays

- `stack_array(name, T, N)`
    
- `sa_len(arr)`
    
- `sa_cap(arr)`
    
- `sa_clear(arr)`
    
- `sa_push(arr, val)`
    
- `sa_pop(arr)`
    
- `sa_peek(arr)`
    
- `sa_at(arr, i)`
    
- `sa_append(dst, src, count)`
    

### Strings

- `stack_string(name, N)`
    
- `ss_len(str)`
    
- `ss_cap(str)`
    
- `ss_clear(str)`
    
- `ss_append(str, src)`
    
- `ss_append_n(str, src, n)`
    
- `ss_pushc(str, c)`
    
- `ss_appendf(str, fmt, ...)`
    
- `ss_sprintf(str, fmt, ...)`
    

### Struct fields

- `sa_field(name, T, N)`
    
- `sa_field_init(obj, field)`
    
- `ss_field(name, N)`
    
- `ss_field_init(obj, field)`
    

## Important rules

### 1. Array helpers only work on library-created buffers

Use the macros/functions only with buffers created by:

- `stack_array`
    
- `stack_string`
    
- `sa_field`
    
- `ss_field`
    

They are not general-purpose wrappers for arbitrary C arrays.

### 2. Struct fields must be initialized

Buffers declared with `sa_field(...)` or `ss_field(...)` must be initialized before use.

Example:

```c
struct Label {
    ss_field(name, 32);
};

void example(void) {
    struct Label label = {0};
    ss_field_init(label, name); // sets capacity
}
```
### 3. String capacity includes the trailing `'\0'`

For strings, capacity is the total buffer size, including the null terminator.

So:

```c
stack_string(str, 32);
```

means:

- up to 31 non-null characters
    
- plus the trailing `'\0'`
    

Also:

- `ss_len(str)` is the logical string length
    
- `ss_len(str)` matches `strlen(str)`
    
- the string helpers write a trailing `'\0'` after each successful modification
    

## Failure behavior

Overflow and underflow call installed handlers.

By default:

- overflow prints an error and aborts
    
- underflow prints an error and aborts
    

You can replace them:

```c
sa_set_overflow_handler(my_overflow_handler);
sa_set_underflow_handler(my_underflow_handler);
```

### Important detail

`sa_at`, `sa_peek`, and `sa_pop` are expression-style checked macros.

They are written with the expectation that the error handlers do not return. If a handler _does_ return, the resulting value of those expressions is unspecified and should not be relied on.

For `sa_push`, the return value is a pointer to the inserted element on success, and `NULL` only if the overflow handler returns.

## Notes and limitations

- The API relies heavily on macros.
    
- The helpers depend on a hidden-header layout and therefore only work with arrays declared using this library's macros.
    
- `sa_field(...)` / `ss_field(...)` require explicit initialization with `sa_field_init(...)` / `ss_field_init(...)`.
    
- Error handlers are intended to abort or otherwise not return.
    
- This is a fixed-capacity container. It does not grow dynamically.

## Requirements

- C99 or later
- Tested on MSVC, clang-cl, GCC, and Clang

## CMake

Build the library and run tests:

```
cmake -S . -B build  
cmake --build build  
ctest --test-dir build --output-on-failure
```

Use from another CMake project:

```
add_subdirectory(path/to/stack_array)  
target_link_libraries(my_target PRIVATE stack_array)
```

If you want to install the library and header:

```
cmake -S . -B build  
cmake --build build  
cmake --install build --prefix ./install
```


## License

MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy  
of this software and associated documentation files (the "Software"), to deal  
in the Software without restriction, including without limitation the rights  
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
copies of the Software, and to permit persons to whom the Software is  
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all  
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  
SOFTWARE.
