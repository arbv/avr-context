# Description

This library provides a low-level facility for context switching between multiple threads of execution and contains an implementation of asymmetric stackful coroutines on an AVR micro-controller.

The low level context switching facility consists of a data type (`avr_context_t`), functions (`avr_getcontext()`, `avr_setcontext()`, `avr_makecontext()`, `avr_swapcontext()`), and macros (`AVR_SAVE_CONTEXT`, `AVR_RESTORE_CONTEXT`, `AVR_SAVE_CONTEXT_GLOBAL_POINTER`, `AVR_RESTORE_CONTEXT_GLOBAL_POINTER`). It is safe to say that this facility provifes implementations (or, rather, substitutes) for `getcontext()`, `setcontext()`, `makecontext()`, and `swapcontext()` which are available on the UNIX-like systems.

The asymmetric stackful coroutines facility consists of a data type (`avr_corot_t`), and four functions (`avr_coro_init()`, `avr_coro_resume()`, `avr_coro_yield()`, `avr_coro_state()`). This functionality is implemented on top of the context switching facility.

One can use the provided functionality in many creative ways. For example, on top of this one can implement:

* cooperative and preemptive multitasking;
* complex error recovery mechanisms;
* profiling.

As many Arduino boards are powered by AVR micro-controllers, this library can be used on them as well. Moreover, this project organised in such a way, that it can be used as an Arduino library. Newer `megaAVR` based boards (e.g. Arduino Nano Every) should work, but were not tested.

# Application Programming Interface

## Context Switching

### Data Types

```
avr_context_t
```

The `avr_context_t` represents a machine context. It should be treated as an opaque data type.


### Functions

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

typedef void (*avr_context_func_t)(void *);

void avr_makecontext(avr_context_t *cp,
                     void *stackp, const size_t stack_size,
                     const avr_context_t *successor_cp,
                     avr_context_func_t funcp, void *funcargp);
```

The function `avr_swapcontext()` saves the current context in the structure pointed to by `oucp` and then activates the context pointed to by `cp` as one operation. It may return later when context pointed to by `oucp` gets activated.

The function `avr_makecontext()` modifies the context obtained by a call to `avr_getcontext()` and pointed to by `cp` in such a way that upon activation the function `funcp` gets called with the `funcargp` value passed as its argument. When this function returns, the successor context `successor_cp` gets activated. Thus, the successor context **must** be a valid context before the activation of the context pointed to by `cp`.

Before invoking the `avr_makecontext()`, the caller must allocate a new stack for the modifiable context and pass pointer to it (`stackp`) and the size of the memory region (`stack_size`).

### Macros

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

## Coroutines

There are four functions that implement the asymmetric stackful coroutine facility: `avr_coro_init()`, `avr_coro_resume()`, `avr_coro_yield()`, `avr_coro_state()`. They are implemented on top of the context switching facility.

A coroutine is represented by the `avr_coro_t` opaque data type. A coroutine can be in one of the following states.

1. Suspended
2. Running
3. Dead

All of the functions return `0` on success or `1` on failure with one notable exception: `avr_coro_state()` returns either a state of a coroutine (represented as a member of the `avr_coro_state_t` data type), or `AVR_CORO_ILLEGAL` status on failure.

Failure status usually means that a wrong argument has been passed to the function.

### Data Types

```
avr_coro_t
```

An opaque data type which represents a coroutine.

### Functions

```
typedef void *(*avr_coro_func_t)(avr_coro_t *, void *);

int avr_coro_init(avr_coro_t *coro,
                  void *stackp, const size_t stack_size,
                  avr_coro_func_t func);
```
The function `avr_coro_init()` initialises a coroutine, represented by a structure pointed at by `coro`. The coroutine gets initialised in the suspended state. Upon resumption, the function `func` gets called with two arguments applied: a pointer to the `avr_coro_t` structure itself (`self`), an argument passed on at first resumption.


```
int avr_coro_resume(avr_coro_t *coro, void **data);
int avr_coro_yield(avr_coro_t *self, void **data);
```
The function `avr_coro_resume()` resumes execution of a coroutine, represented by a structure pointed at by `coro.` The coroutine might return control to the invoker by calling the function `avr_coro_yield()`. It accepts a currently running coroutine represented by a structure pointed at by `self`.

Both of the functions accept pointer to a pointer as the last argument (`data`). The coroutine and its invoker can exchange data using this last argument. When the coroutine gets resumed using `avr_coro_resume()` for the first time, a value of the pointer, to which `data` points, gets passed as the second argument to the coroutine function.

After the call to the avr_coro_resume() returns, the pointer to which `data` points during the call, gets initialised to the value, correspondingly passed as the last argument of `avr_coro_yield()`.

Symmetrically, after the call to the `avr_coro_yield()` returns, the pointer to which `data` points during the call, gets initialised to the value correspondingly passed as the last argument of `avr_coro_resume()`.

When the coroutine's function returns (and, thus, becomes dead), the returned value initialises the pointer to which `data` points during the corresponding `avr_coro_resume()` call.

If there is no need to exchange data between the coroutine and its invoker, one can pass the `NULL` value as the `data` argument to `avr_coro_resume()`/`avr_coro_yield()`.


```
typedef enum avr_coro_state_t_ {
    AVR_CORO_SUSPENDED = 0,
    AVR_CORO_RUNNING,
    AVR_CORO_DEAD,
    AVR_CORO_ILLEGAL,
} avr_coro_state_t;

avr_coro_state_t avr_coro_state(const avr_coro_t *coro);

```

The function `avr_coro_state()` returns the current state of a coroutine:

1. If the coroutine is suspended, the function returns `AVR_CORO_SUSPENDED`.
2. If the coroutine is running, the function returns `AVR_CORO_RUNNING`. This value can be obtained only from within the context of the currently running coroutine.
3. If the coroutine is dead, the function returns `AVR_CORO_DEAD`.

The value `AVR_CORO_ILLEGAL` gets returned in the case of error (e.g. the `NULL` value was passed instead of a pointer to a coroutine).

# Usage

## Arduino

If you happen to use an Arduino board, then all you need to do is to install the contents of this repository [as a library](https://www.arduino.cc/en/guide/libraries). Then add the following code somewhere at the beginning of your sketch:

```
#include <avrcontext_arduino.h>

```

If you want to keep your project self contained, you may want to use the library in a way, described below.

## Generic

The core of the library consists of four files: two *header files with declarations* (`avrcontext.h`, `avrcoro.h`) and two *header files with definitions* (`avrcontext_impl.h`, `avrcoro_impl.h`). No header file includes any other header files.  The *header files with definitions* should be used *only once* across the project.

The files `avrcontext_arduino.c` and `avrcontext_arduino.h` are there to make it possible to use this code as an Arduino library. If you do not rely on Arduino platform when developing your project, you probably do not need these files.

This organisation of the project provides extra flexibility without demanding any particular structure from projects which use this library. It is especially convenient if you want to keep your projects self-contained.

Please keep in mind that coroutines facility depends on context switching facility.

For example, you can dedicate one of the C or C++ source files in the project (e.g. `avrcontext.c`) to the definitions of the functions which implement the functionality of the library. In this case, you can put the following content into that file (assuming that the library is in the `avr-context` directory):

```
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>

/* context switching */
#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"
/* coroutines */
#include "avr-context/avrcoro.h"
#include "avr-context/avrcoro_impl.h"
```

If it a one-file project, then putting the text above into the include section of the file would be enough. If it is an Arduino sketch, then it could be even simpler than that (assuming that the library is in the `avr-context` directory that is inside the directory of the sketch):

```
/* context switching */
#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"
/* coroutines */
#include "avr-context/avrcoro.h"
#include "avr-context/avrcoro_impl.h"
```

In the case when a project consists of multiple source files and the functionality of this library required in more than one of them, you can put the following into the include sections of the files (please notice the absence of `avrcontext_impl.h` and `avrcoro_impl.h`):

```
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>

#include "avr-context/avrcontext.h"
#include "avr-context/avrcoro.h" /* if you need coroutines */
```


# Examples

The examples below may be used as a tutorial. Every example is a complete Arduino sketch. It was decided to use Arduino boards for examples because the boards have somewhat standardised application programming interface and hardware.

Nevertheless, this library can be used on its own on AVR controllers: it does not contain any Arduino-specific code. Hopefully, the examples are easy to adapt to any AVR based hardware.

## Context Switching

### [GOTO via Context Switching](./examples/Context_Switching/01.GOTO_via_Context_Switching/01.GOTO_via_Context_Switching.ino)

This example demonstrates how `avr_getcontext()` and `avr_setcontext()` could be used to emulate the `GOTO` operator.

When being uploaded to an Arduino board, this sketch produces the following output via serial port every one second:

```
Hello from start()!
```

Please notice that:

1. There are no explicit loops.
2. Execution never reaches the end of the `setup()` function.
3. As the consequence of the previous point: `loop()` function never gets called.

### [Non-Linear Program Execution](./examples/Context_Switching/02.Non-Linear_Program_Execution/02.Non-Linear_Program_Execution.ino)

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

### [Symmetric Stackful Coroutines](./examples/Context_Switching/03.Symmetric_Stackful_Coroutines/03.Symmetric_Stackful_Coroutines.ino)

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

### [Preemptive Task Switching](./examples/Context_Switching/04.Preemptive_Task_Switching/04.Preemptive_Task_Switching.ino)

This example shows how preemptive task switching can be implemented on top of `avr_getcontext()`, `avr_makecontext()`, `AVR_SAVE_CONTEXT_GLOBAL_POINTER()`,`AVR_RESTORE_CONTEXT_GLOBAL_POINTER()` and a hardware timer, which generates interrupts.

The main idea is quite simple. There are two tasks, both run indefinitely: one enables the built-in LED, the other one disables it. System timer, which is implemented on top of watchdog running in interrupt mode, ticks every one second and switches the tasks in Round Robin fashion during the tick. Thus, the LED does not remain enabled or disabled for more than one second.

One notable interesting point here is that we convert the initial execution context of an MCU into a switchable task. This allows `loop()` function to work as expected. We use it to enable the LED. To do so we allocate the first element of the `tasks` array for the initial MCU execution context, we modify the global variables used for task switching in such a way that during the first interrupt tick the initial execution context gets saved into the first element of the `tasks` array.

Please keep in mind that our intention here is to show how task switching can be performed in a preemptive task executive, not to implement an RTOS in one Arduino sketch. In a real RTOS system timer would tick at a much higher rate, and on every tick, it would perform much more complicated scheduling code.

Nevertheless, if you are brave enough to write your own RTOS, this tiny sketch might be a good start.

## Coroutines

### [Basic Generator](./examples/Coroutines/01.Basic_Generator/01.Basic_Generator.ino)

This example demonstrates how an asymmetric coroutine might be used as a generator.  Every time the coroutine gets invoked it generates the next number in sequence (`0...N`) and returns it back to the invoker.

One of the important points of this example is to demonstrate how a value from the coroutine can be passed back to the invoker.

When being uploaded to an Arduino board, this sketch print numbers in increasing order via serial port every one second, e.g.:


```
0
1
2
3
4
5
...

```

### [Symmetric Coroutines via Asymmetric Ones](./examples/Coroutines/02.Symmetric_Coroutines_via_Asymmetric_Ones/02.Symmetric_Coroutines_via_Asymmetric_Ones.ino)

This example demonstrates how one could implement symmetric coroutines on top of the asymmetric ones.

The most important difference between the symmetric and asymmetric coroutines is that the asymmetric ones cannot pass control to the other coroutine directly: they can return control only back to the coroutine invoker. Nevertheless, they are no less powerful.

To bypass the above-mentioned limitation, a coroutine, which yields control, passes the information about the coroutine which it wants to activate to a dispatching loop. The loop, in turn, activates the next coroutine.

It is a version of the other example, which was implemented directly on top of `avr_makecontext()`, `avr_swapcontext()`.

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

### [Producer-Consumer](./examples/Coroutines/03.Producer-Consumer/03.Producer-Consumer.ino)

This example demonstrates how asymmetric coroutines can be used to solve a well known "producer-consumer" problem.

Coroutines communicate using the `send()` and `receive()` functions. `send()` passes a value to the consumer and resumes it, `receive()` gets value from the producer and returns control back to it. These actions are repeated forever.

It is a consumer-driven design because the program starts by calling the consumer. When the consumer needs an item, it resumes the producer, which runs until it has an item to give to the consume, and then pauses until the consumer restarts it again.

When being uploaded to an Arduino board, this sketch produces a pair of messages via serial port every one second in the following format:

```
Produced: 0
Consumed: 0
Produced: 1
Consumed: 1
Produced: 2
Consumed: 2
Produced: 3
Consumed: 3
Produced: 4
Produced: 4
...
```
### [Producer-Filter-Consumer](./examples/Coroutines/04.Producer-Filter-Consumer/04.Producer-Filter-Consumer.ino)

This example demonstrates how asymmetric coroutines can be used to solve the "producer-filter-consumer" problem (one can also call the filter as "mediator").

Coroutines communicate using the `send()` and `receive()` functions. The function `send()` passes a value to the receiver and returns control back to the invoker, the function `receive()` gets value from the sender and resumes it.

It is a consumer-driven design because the program starts by calling a consumer. The consumer receives a value from a filter and thus resumes it. Then the filter receives the value from a producer (again, by resuming it) and doubles it. After that, the filter sends the doubled value to the consumer. By doing so it resumes the consumer. Then the whole process repeats.

When being uploaded to an Arduino board, this sketch produces a triplet of messages via serial port every one second in the following format.

```
Produced: 0
Filtered: 0
Produced: 0
Produced: 1
Filtered: 2
Produced: 2
Filtered: 4
Consumed: 4
Produced: 3
Filtered: 6
Consumed: 6
Produced: 4
Filtered: 8
Consumed: 8
Produced: 5
Filtered: 10
Consumed: 10
...
```

# References

1. [Richard Barry - Multitasking on an AVR, 2004](https://xivilization.net/~marek/binaries/multitasking.pdf)
2. [FreeRTOS Implementation Building Blocks: The AVR Context](https://www.freertos.org/implementation/a00015.html)
3. [The Open Group Base Specifications Issue 6: getcontext, setcontext - get and set current user context](https://pubs.opengroup.org/onlinepubs/009695399/functions/getcontext.html)
4. [The Open Group Base Specifications Issue 6: makecontext, swapcontext - manipulate user contexts](https://pubs.opengroup.org/onlinepubs/009695399/functions/makecontext.html)
5. [Ana Lucia De Moura, Roberto Ierusalimschy - Revisiting Coroutines, ACM Transactions on Programming Languages and Systems 31(2), July 2004](http://www.inf.puc-rio.br/~roberto/docs/MCC15-04.pdf)

# Copyright

Copyright (c) 2020 [Artem Boldariev](https://chaoticlab.io/).

The software distributed under the terms of the MIT/Expat license.

See [LICENSE.txt](./LICENSE.txt) for license details.

***

*Made in Ukraine*

