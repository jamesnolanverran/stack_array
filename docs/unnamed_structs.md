# Unnamed Struct Types, Anonymous Members, and how `stack_array` Works

C lets you write code like this:

```c
struct { int x; } foo;
```

At first glance, this can look like some strange hybrid of a struct definition and a variable declaration. That is basically what it is. The important detail is that this is **not** the same thing as an anonymous struct member inside another struct, and that distinction matters.

## A named object of an unnamed struct type

In this declaration:

```c
struct { int x; } foo;
```

the type is:

```c
struct { int x; }
```

and the object is:

```c
foo
```

So this is directly analogous to:

```c
int foo;
```

The only difference is that instead of `int`, the type specifier is an unnamed struct type.

This means the declaration is best understood as:

* define a struct type with no tag
* declare one object of that type

The struct type is unnamed, but the object is not.

## This is different from a tagged struct

With a tagged struct:

```c
struct Point { int x; };
struct Point p;
```

the reusable type name is:

```c
struct Point
```

That tag lets you refer to the same type later.

With the unnamed form:

```c
struct { int x; } p;
```

there is no reusable tag. The type exists, but only inline in that declaration.

## This is also different from a typedef

A typedef gives a reusable name to a type:

```c
typedef struct {
    int x;
} Point;
```

Now `Point` is a typedef name for that unnamed struct type.

So these three forms are doing different things:

```c
struct Point { int x; };      // tagged struct type
typedef struct { int x; } P;  // unnamed struct type + typedef name
struct { int x; } p;          // unnamed struct type + object
```

## A subtle but important rule: each unnamed struct type is distinct

This surprises people the first time they notice it.

These two declarations do **not** produce the same type:

```c
struct { int id; } a;
struct { int id; } b;
```

Even though the text is identical, `a` and `b` have different types.

That means unnamed struct types are not interchangeable just because their members happen to match. Each appearance creates a fresh type.

That is why tags and typedefs are not just cosmetic. They are how you make a type reusable.

## Why this matters for functions

If you create separate unnamed struct types, you cannot conveniently write one normal function interface that treats them as the same struct type.

That is one reason unnamed struct types are usually used for:

* one-off local storage
* internal helper objects
* macro-generated implementation details

They are less useful when you want:

* a shared public type
* multiple objects of exactly the same type
* reusable function signatures based on that type

## The other feature: anonymous struct members

Now compare this:

```c
struct Outer {
    int id;
    struct {
        int x;
        int y;
    };
};
```

This is a different language feature.

Here, the inner struct is an **unnamed member** of `struct Outer`. Its members are promoted so you can write:

```c
struct Outer o = {0};
o.x = 1;
o.y = 2;
```

without naming the inner member first.

This is what people often mean by an **anonymous struct member**. It is different from:

```c
struct { int x; } foo;
```

The first is a struct declaration with an unnamed member inside another struct. The second is just a named object of an unnamed struct type.

These two forms are easy to confuse because both involve a struct without a name, but they are not the same thing.

## Why the `stack_array` pattern works

The `stack_array` technique uses a declaration like this:

```c
struct {
    SA_Header hdr;
    T data[N];
} SA_name = { { N, 0 }, { 0 } };
```

This creates one storage object containing:

* a header
* the actual array data

laid out contiguously in memory.

Then the code takes the address of the `data` member and works with that:

```c
T *name = SA_name.data;
```

At that point, functions are not dealing with the unnamed struct type anymore. They are dealing with:

```c
T *
```

or for strings:

```c
char *
```

That completely sidesteps the awkwardness of passing unnamed struct types through function interfaces.

The enclosing unnamed struct is only there to package metadata and storage together in one object. The public-facing API operates on the member pointer.

## Why this avoids the function problem

This is the key trick.

The library is **not** passing around the struct object itself. It is passing around a member of that object:

* `SA_name.data` is an array member
* that array decays to a pointer in normal expressions
* helper functions take that pointer
* header metadata is recovered by stepping backward in memory

So the API stays simple:

```c
char *str
int *arr
```

while the hidden metadata remains attached in memory. (see [Hidden Metadata in Front of an Array](hidden_metadata.md))

## The field/member version uses the same idea

The same pattern also explains why embedded struct fields work cleanly.

For example:

```c
struct Label {
    int id;
    ss_field(name, 32);
};
```

After initialization, you can use:

```c
label.name
l->name
char *name = label.name;
```

because the library API is operating on the field pointer, not on the enclosing struct type.

---

The `stack_array` trick works well because it combines two ideas:

* use an unnamed struct type as a private storage wrapper
* expose only the array member pointer to the API

That gives you:

* hidden metadata
* contiguous layout
* simple call sites
* no need to pass around a visible wrapper struct
