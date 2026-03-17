# Hidden Metadata in Front of an Array

A nice part of `stack_array` is that the user-facing value looks like an ordinary pointer:

```c
char *str;  
int  *arr;
```

But length and capacity are still available to the library.

That only works because the metadata is stored immediately before the array data in memory.

## The basic layout

The core idea is to create one object that contains both:
- a header
- the actual array storage
    

For example:

```c
struct {  
    SA_Header hdr;  
    char data[32];  
} SA_name = { { 32, 0 }, { 0 } };
```

The memory layout looks like this:

+----------------+-----------------------------+  
| SA_Header hdr  |      char data[32]          |  
+----------------+-----------------------------+  
                 ^  
                 |  
               SA_name points here

The public variable is then just:

```c
char *name = SA_name.data;
```

So from the caller’s point of view, `name` is just a normal `char *`.

## Where the metadata lives

The important point is that `name` does **not** point to the beginning of the whole object. It points to the first element of the `data` member.

That means the header is still sitting just before it in memory.

So if `name` points here:

[ SA_Header ][ d ][ a ][ t ][ a ] ...  
              ^  
              name

then stepping back by one `SA_Header` gets you the metadata.

That is what this macro does:

```c
#define sa_hdr(arr) (((SA_Header *)(arr)) - 1)
```

## What that cast is doing

Suppose `arr` is a `char *` pointing at the first byte of `data`.

This expression:

```c
(SA_Header *)(arr)
```

reinterprets that address as a pointer to `SA_Header`.

Then:

```c
((SA_Header *)(arr)) - 1
```

moves backward by `sizeof(SA_Header)` bytes.

The result is a pointer to the header stored immediately before the array.

So once the library has `arr`, it can recover:
- `hdr->len`
- `hdr->cap`
    

without the caller having to pass them separately.

## Why this is useful

This buys a nicer call site.

Instead of something like:

```c
append(buf, &len, cap, src);
```

the user can write:

```c
ss_append(str, src);
```

The metadata is still there, but it is hidden in the storage layout instead of being threaded through every call.

That is the real convenience.

## Why the anonymous struct matters

The unnamed struct is what bundles the header and array into one contiguous object:

```c
struct {  
    SA_Header hdr;  
    T data[N];  
} SA_name;
```

Without that wrapper, there would be no guarantee that the header and data belong to the same object in the right layout.

The wrapper gives you:
- one allocation-free object
- contiguous metadata + storage
- a clean way to expose only the `data` pointer
    

The anonymous struct itself is mostly an implementation detail. What matters is the layout it creates.

## Why the API still uses ordinary pointers

The library does **not** pass around the wrapper struct type.

It passes around the member:

```c
SA_name.data
```

That member is an array, and in normal expressions it decays to a pointer:

```c
T *  
char *
```

So the public API can stay pointer-based, while still recovering hidden metadata internally.

That is why the design feels light at the call site. The wrapper exists, but the user mostly never sees it.

## Why embedded struct fields also work

The same idea applies to embedded fields.

For example:
```c
struct Label {  
    int id;  
    ss_field(name, 32);  
};
```

After initialization, `label.name` works with the normal helpers because it is still a buffer with the same header-then-data layout.

So these all work:

```c
ss_append(label.name, "hi");  
ss_append(l->name, " there");  
  
char *name = label.name;  
ss_append(name, "!");
```

The library is not operating on `struct Label`. It is operating on the field pointer, and recovering the hidden header from there.

The API looks like plain pointers, but those pointers only work because they originate from buffers with the expected hidden layout.

## Why this is nicer than visible bookkeeping

Most helper sets keep the bookkeeping visible:

- pass `len`
- pass `cap`
- update both manually
- keep buffer state in sync by convention
    
This design hides more of that state in the object itself.

So the benefit is:

- checked operations
- with very little call-site bookkeeping
