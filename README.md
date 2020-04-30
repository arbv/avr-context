# Description

This library provides a low-level facility for context switching between multiple threads of execution on an AVR micro-controller. The facility consists of a data type (`avr_context_t`), functions (`avr_getcontext()`, `avr_setcontext()`, `avr_makecontext()`, `avr_swapcontext()`), and macros (`AVR_SAVE_CONTEXT`, `AVR_RESTORE_CONTEXT`, `AVR_SAVE_CONTEXT_GLOBAL_POINTER`, `AVR_RESTORE_CONTEXT_GLOBAL_POINTER`).

It is safe to say that this library contains implementations (or, rather, substitutes) for `getcontext()`, `setcontext()`, `makecontext()`, and `swapcontext()` which are available on the UNIX-like systems.

One can use the provided functionality in many creative ways. For example, on top of this one can implement:

* coroutines and fibers;
* cooperative and preemptive multitasking;
* complex error recovery mechanisms;
* profiling.

As many Arduino boards are powered by AVR micro-controllers, this library can be used on them as well. Moreover, this project organised in such a way, that it can be used as an Arduino library. Newer `megaAVR` based boards (e.g. Arduino Nano Every) should work, but were not tested.

# Application Programming Interface

## Data Types

```
avr_context_t
```

The `avr_context_t` represents a machine context. It should be treated as an opaque data type.


## Functions

The four functions `avr_getcontext()`, `avr_setcontext()`, `avr_swapcontext()`, and `avr_makecontext()` provide facility for context switching between multiple threads of execution.

These functions provide more or less direct substitutes to the functions `getcontext()`, `setcontext()`, `swapcontext()`, and `makecontext()` which used to be a part of the POSIX standard. It is recommended to read about them before using the functions from this library[[1]](https://pubs.opengroup.org/onlinepubs/009695399/functions/getcontext.html)[[2]](https://pubs.opengroup.org/onlinepubs/009695399/functions/makecontext.html)[[3]](http://man7.org/linux/man-pages/man3/setcontext.3.html)[[4]](http://man7.org/linux/man-pages/man3/makecontext.3.html).

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

The function `avr_swapcontext()` saves the current context in the structure pointed to by `oucp` and then activates the context pointed to by `cp` as one operation. It may return later when context pointed to by `oucp` gets activated.

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

The code in the macros expects that the pointer register `Z` (`R31:R30`) contains the address of an `avr_context_t` structure. Additionally to that, the code expects to find the return address on top of the stack (like after the `CALL` family of instructions, or during an interrupt handling). In fact, the macros are barely useful anywhere else except the naked interrupt system routines.

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

## Arduino

If you happen to use an Arduino board, then all you need to do is to install the contents of this repository [as a library](https://www.arduino.cc/en/guide/libraries). Then add the following code somewhere at the beginning of your sketch:

```
#include <avrcontext_arduino.h>

```

If you want to keep your project self contained, you may want to use the library in a way, described below.

## Generic

The core of the library consists of two files: a *header file with declarations* (`avrcontext.h`) and a *header file with definitions* (`avrcontext_impl.h`). No header file includes any other header files.  The *header file with definitions* should be used *only once* across the project. The files `avrcontext_arduino.c` and `avrcontext_arduino.h` are there to make it possible to use this code as an Arduino library. If you do not rely on Arduino platform when developing your project, you probably do not need these files.

This organisation of the project provides extra flexibility without demanding any particular structure from projects which use this library. It is especially convenient if you want to keep your projects self-contained.

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


# Examples

Every example below is a complete Arduino sketch. It was decided to use Arduino boards for examples because the boards have somewhat standardised application programming interface and hardware.

Nevertheless, this library can be used on its own on AVR controllers: it does not contain any Arduino-specific code. Hopefully, the examples are easy to adapt to any AVR based hardware.

## [GOTO via Context Switching](./examples/Low_Level/01.GOTO_via_Context_Switching/01.GOTO_via_Context_Switching.ino)

This example demonstrates how `avr_getcontext()` and `avr_setcontext()` could be used to emulate the `GOTO` operator.

When being uploaded to an Arduino board, this sketch produces the following output via serial port every one second:

```
Hello from start()!
```

Please notice that:

1. There are no explicit loops.
2. Execution never reaches the end of the `setup()` function.
3. As the consequence of the previous point: `loop()` function never gets called.

### [Non-Linear Program Execution](./examples/Low_Level/02.Non-Linear_Program_Execution/02.Non-Linear_Program_Execution.ino)

This example demonstrates how `avr_makecontext()` and `avr_swapcontext()` could be used to achieve non-linear execution of a program.

It also might give some clues about how coroutines could be implemented.

When being uploaded to an Arduino board, this sketch produces the following output via serial port every one second:

```
loop() start
start f2
start f1
finish f2
finish f1
loop() end
```

### [Symmetric Stackful Coroutines](./examples/Low_Level/03.Symmetric_Stackful_Coroutines/03.Symmetric_Stackful_Coroutines.ino)

This example demonstrates how `avr_makecontext()` and `avr_swapcontext()` could be used to implement coroutines.

When being uploaded to an Arduino board, this sketch produces the following (or very similar) output via serial port every two seconds:

```
Starting coroutines...

Coroutine 0 counts i=0 (&i=0x18A)
Coroutine 1 counts i=0 (&i=0x20A)
Coroutine 0 counts i=1 (&i=0x18A)
Coroutine 1 counts i=1 (&i=0x20A)
Coroutine 0 counts i=2 (&i=0x18A)
Coroutine 1 counts i=2 (&i=0x20A)
Coroutine 0 counts i=3 (&i=0x18A)
Coroutine 1 counts i=3 (&i=0x20A)
Coroutine 0 counts i=4 (&i=0x18A)
Coroutine 1 counts i=4 (&i=0x20A)
Done.
```

### [Preemptive Task Switching](./examples/Low_Level/04.Preemptive_Task_Switching/04.Preemptive_Task_Switching.ino)

This example shows how preemptive task switching can be implemented on top of `avr_getcontext()`, `avr_makecontext()`, `AVR_SAVE_CONTEXT_GLOBAL_POINTER()`,`AVR_RESTORE_CONTEXT_GLOBAL_POINTER()` and a hardware timer, which generates interrupts.

The main idea is quite simple. There are two tasks, both run indefinitely: one enables the built-in LED, the other one disables it. System timer, which is implemented on top of watchdog running in interrupt mode, ticks every one second and switches the tasks in Round Robin fashion during the tick. Thus, the LED does not remain enabled or disabled for more than one second.

One notable interesting point here is that we convert the initial execution context of an MCU into a switchable task. This allows `loop()` function to work as expected. We use it to enable the LED. To do so we allocate the first element of the `tasks` array for the initial MCU execution context, we modify the global variables used for task switching in such a way that during the first interrupt tick the initial execution context gets saved into the first element of the `tasks` array.

Please keep in mind that our intention here is to show how task switching can be performed in a preemptive task executive, not to implement an RTOS in one Arduino sketch. In a real RTOS system timer would tick at a much higher rate, and on every tick, it would perform much more complicated scheduling code.

Nevertheless, if you are brave enough to write your own RTOS, this tiny sketch might be a good start.

# Copyright

Copyright (c) 2020 [Artem Boldariev](https://chaoticlab.io/).

The software distributed under the terms of the MIT/Expat license.

See [LICENSE.txt](./LICENSE.txt) for license details.

***

*Made in Ukraine*

