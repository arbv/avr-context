# Description

This library provides data type (`avr_context_t`), functions (`avr_getcontext()`, `avr_setcontext()`, `avr_makecontext()`, `avr_swapcontext()`), and macros (`AVR_SAVE_CONTEXT`, `AVR_RESTORE_CONTEXT`, `AVR_SAVE_CONTEXT_GLOBAL_POINTER`, `AVR_RESTORE_CONTEXT_GLOBAL_POINTER`) that implement context switching between multiple threads of execution on an AVR micro-controller.

It is safe to say that this library contains implementations (or, rather, substitutes) for `getcontext()`, `setcontext()`, `makecontext()`, and `swapcontext()` which are available on the UNIX-like systems.

As many Arduino boards are powered by AVR micro-controllers, this library can be used on these boards as well.

# Application Programming Interface

## Data Types

```
avr_context_t
```

The `avr_context_t` represents a machine context. It should be treated as an opaque data type.


## Functions

The four functions `avr_getcontext()`, `avr_setcontext()`, `avr_swapcontext()`, and `avr_makecontext()` provide facility for context switching between multiple threads of execution.

These functions provide more or less direct substitutes to the functions `getcontext()`, `setcontext()`, `swapcontext()`, and `makecontext()` which used to be a part of the POSIX standard. It is recommended to read about them before using the functions from this library.

There are multiple important differences between the functions from this library and their equivalents from the POSIX standard:

1. These functions do not perform sanity checking of their input parameters. This is a deliberate decision. They are barely useful on their own, in most cases, they used as a basis for implementing higher-level abstractions. Thus, it is better to perform sanity checking at the higher levels. Moreover, these functions tend to appear in the hot spots of an application, where additional sanity checks lead to extra wasted CPU cycles. This might be not acceptable behaviour when programming MCUs.

2. As the direct consequence of the above: there is no error reporting. These functions will gladly accept whatever you pass to them. If you have passed to them something unintentionally, that could lead to undefined behaviour (e.g. it could lead to a mess in an MCU's memory).

The two paragraphs above mean that these functions built around the GIGO principle (Garbage In, Garbage Out). One could call them *unsafe*, but I prefer to call them *sharp*.

```
void avr_getcontext(avr_context_t *cp);

void avr_setcontext(const avr_context_t *cp);
```

The function `avr_getcontext()` initialises the structure pointed at by `cp` to the currently active context.

The function `avr_setcontext()` restores the context from the structure pointed at by `cp`.  The context should have been obtained by a call to `avr_getcontext()`, `avr_swapcontext()` or `avr_makecontext()`. In the case, when the context has been obtained by a call to the `avr_getcontext()` or `avr_swapcontext()` program execution continues as if the call has just returned. *The function `avr_setcontext()` never returns.*

```
void avr_swapcontext(avr_context_t *oucp, const avr_context_t *cp);

void avr_makecontext(avr_context_t *cp,
                     void *stackp, const size_t stack_size,
                     const avr_context_t *successor_cp,
                     void (*funcp)(void *), void *funcargp);
```

The function `avr_swapcontext()` saves the current context in the structure pointed to by `oucp` and then activates the context pointed to by `cp` as one operation. It may return later when context pointed to by `oucp` gets activated. Calling `avr_swapcontext()` with `cp` and `oucp` pointing to the same structure leads to undefined behaviour.

The function `avr_makecontext()` modifies the context obtained by a call to `avr_getcontext()` and pointed to by `cp` in such a way that upon activation the function `funcp` gets called with the `funcargp` value passed as its argument. When this function returns, the successor context `successor_cp` gets activated. Thus, the successor context **must** be a valid context before the activation of the context pointed to by `cp`.

Before invoking the `avr_makecontext()`, the caller must allocate a new stack for the modifiable context and pass pointer to it (`stackp`) and the size of the memory region (`stack_size`).

## Macros

```
#define AVR_SAVE_CONTEXT(presave_code, load_address_to_Z_code)

#define AVR_RESTORE_CONTEXT(load_address_to_Z_code)
```

`AVR_SAVE_CONTEXT` and `AVR_RESTORE_CONTEXT` macros provide the generic facility for saving and restoring an AVR CPU context.

Using them directly needed only in rare cases, please consider using the `*context()` functions described above. These macros may be useful when implementing Interrupt System Routines, though.

It should be noted that `*context()` functions implemented on top of `AVR_SAVE_CONTEXT` and `AVR_RESTORE_CONTEXT`.

The code in the macros expects that the pointer register `Z` (`R31:R30`) contains the address of an `avr_context_t` structure. Additionally to that, the code expects to find the return address on top of the stack (like after the `CALL` family of instructions, or during an interrupt handling).

The argument named `load_address_to_Z_code` should be a string constant which contains assembly instructions. These instructions should load the address of an `avr_context_t` structure to the pointer register `Z`. Before executing this code, the original values in the register `Z` preserved.

The argument named `presave_code` should be a string constant which contains assembly instructions which get executed right after preserving the `SREG` register value. If, for example, you want to disable interrupts before saving the context, it is the right place to do it.

Please note that, in general, after executing the code in `presave_code`, `load_address_to_Z_code`, one should restore the original values of the general-purpose registers, the stack pointer, and, in most cases, the status register (`SREG`).

One could have noted that using these macros is quite cumbersome, but this is a very low-level code and in some cases, it is rather hard (or impossible) to provide a reasonable interface for the low-level functionality.

I want to stress it one more time: if in doubt please use `avr_getcontext()`, `avr_setcontext()`, `avr_swapcontext()`, `avr_makecontext()` instead.

```
#define AVR_SAVE_CONTEXT_GLOBAL_POINTER(presave_code, global_context_pointer)

#define AVR_RESTORE_CONTEXT_GLOBAL_POINTER(global_context_pointer)
```

`AVR_RESTORE_CONTEXT_GLOBAL_POINTER` and `AVR_SAVE_CONTEXT_GLOBAL_POINTER` macros provide the generic facility for saving/restoring an AVR CPU context to/from a structure via a global pointer variable.

Please make sure that the pointer defined as a volatile pointer. If you use C++, wrap the pointer declaration in `extern "C" { .. }` to avoid name mangling.

Example pointer definition:

```
#ifdef __cplusplus
extern "C" {
#endif
avr_context_t *volatile avr_current_ctx;
#ifdef __cplusplus
}
#endif
```

As these macros implemented on top of the `AVR_SAVE_CONTEXT` and `AVR_RESTORE_CONTEXT`, please make sure that you understand how they work.

# Usage

This library consists of two files: a *header file with declarations* (`avrcontext.h`) and a *header file with definitions* (`avrcontext_impl.h`). No header file includes any other header files.  The *header file with definitions* should be used *only once* across the project.

This organisation believed to provide extra flexibility without demanding any particular project structure. It is especially convenient when used with Arduino sketches or one-file projects.

For example, you can dedicate one of the C or C++ source files in the project (e.g. `avrcontext.c`) to the definitions of `avr_getcontext()`, `avr_setcontext()`, `avr_swapcontext()`, and `avr_makecontext()`. In this case you can put the following content into that file (assuming that the library is in the `avr-context` directory):

```
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>

#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"
```

If it a one-file project, then putting the text above into the include section of the file would be enough. If it is an Arduino sketch, then it could be even simpler than that (assuming that the library is in the `avr-context` directory that is inside the directory of the sketch):

```
#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"
```

In the case when a project consists of multiple source files and the functionality of this library required in more than one of them, you can put the following into the include sections of the files (please notice the absence of `avrcontext_impl.h`):

```
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>

#include "avr-context/avrcontext.h"
```

***

*Made in Ukraine*

