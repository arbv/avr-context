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


# Examples

Every example below is a complete Arduino sketch. It was decided to use Arduino boards for examples because the boards have somewhat standardised application programming interface and hardware.

Nevertheless, this library can be used on its own on AVR controllers: it does not contain any Arduino-specific code. Hopefully, the examples are easy to adapt to any AVR based hardware.

Again, in every example, we assume that the context switching library located in the `avr-context` directory that is inside the directory of the sketch.

## Unconditional Jump via Context Switching

```
/*
This example demonstrates how avr_getcontext() and avr_setcontext()
could be used to emulate the GOTO operator.

When being uploaded to an Arduino board, this sketch produces the
following output via serial port every one second:

Hello from start()!

Please notice that:

1) There are no explicit loops.
2) Execution never reaches the end of the setup() function.
3) As the consequence of the previous point: loop() function never gets called.
*/

#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"

void setup(void)
{
    avr_context_t ctx;
    Serial.begin(9600);
    while (!Serial);

    avr_getcontext(&ctx); // save the current execution context
    Serial.println(F("Hello from start()!"));
    delay(1000);
    avr_setcontext(&ctx); // go to the previously saved context
    // unreachable
    Serial.println(F("returning from start()..."));
}

void loop(void) // never gets called
{
    Serial.println(F("loop()"));
}

```

## Non-Linear Program Execution

```
/*
This example demonstrates how avr_makecontext() and avr_swapcontext()
could be used to achieve non-linear execution of a program.

It also might give some clues about how coroutines could be implmented.

When being uploaded to an Arduino board, this sketch produces the
following output via serial port every one second:

loop() start
start f2
start f1
finish f2
finish f1
loop() end

This example was found on the OpenGroup site and adapted for Arduino.

https://pubs.opengroup.org/onlinepubs/009695399/functions/swapcontext.html
*/

#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"

static avr_context_t ctx[3];

static void f1(void *)
{
    Serial.println(F("start f1"));
    avr_swapcontext(&ctx[1], &ctx[2]);
    Serial.println(F("finish f1"));
}

static void f2(void *)

{
    Serial.println(F("start f2"));
    avr_swapcontext(&ctx[2], &ctx[1]);
    Serial.println(F("finish f2"));
}

static void opengroup_example(void)
{
    char st1[128];
    char st2[128];
    avr_getcontext(&ctx[1]);
    avr_makecontext(&ctx[1], &st1[0], sizeof(st1), &ctx[0], f1, 0);

    avr_getcontext(&ctx[2]);
    avr_makecontext(&ctx[2], &st2[0], sizeof(st2), &ctx[1], f2, 0);

    avr_swapcontext(&ctx[0], &ctx[2]);
}

void setup(void)
{
    Serial.begin(9600);
    while (!Serial);
}

void loop(void)
{
    Serial.println(F("loop() start"));
    opengroup_example();
    delay(1000);
    Serial.println(F("loop() end"));
}

```

## Low-Level Stackful Co-Routines

```
/*
This example demonstrates how avr_makecontext() and avr_swapcontext()
could be used to implement co-routines.

When being uploaded to an Arduino board, this sketch produces the
following (or very similar) output via serial port every two seconds:

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

The original example was found here:

http://courses.cs.vt.edu/~cs5204/fall12-gback/examples/threads/coroutines.c
*/

#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"

#define STACK_SIZE 128

static uint8_t stack[2][STACK_SIZE];       // a stack for each coroutine
static avr_context_t coroutine_state[2];   // container to remember context
static int arguments[2];                   // coroutine arguments

// switch current coroutine (0 -> 1 -> 0 -> 1 ...)
static void yield_to_next(void)
{
    static size_t current = 0;

    size_t prev = current;
    size_t next = 1 - current;

    current = next;
    avr_swapcontext(&coroutine_state[prev], &coroutine_state[next]);
}

static void coroutine(void *data)
{
    const int coroutine_number = *((int *)data);
    for (size_t i = 0; i < 5; i++)
    {
        Serial.print(F("Coroutine "));
        Serial.print(coroutine_number);
        Serial.print(F(" counts i="));
        Serial.print(i);
        Serial.print(F(" (&i=0x"));
        Serial.print((uintptr_t)&i, HEX);
        Serial.println(F(")"));
        yield_to_next();
    }
}

static void coroutines_example(void)
{
    avr_context_t return_to_main;
    // set up
    for (size_t i = 0; i < 2; i++)
    {
        // initialize avr_context_t
        avr_getcontext(&coroutine_state[i]);
        arguments[i] = i;
        avr_makecontext(&coroutine_state[i],
                        (void *)stack[i], STACK_SIZE, // set up per-context stack
                        &return_to_main, // when done, resume 'return_to_main' context
                        coroutine, //let context[i] perform a call to coroutine(i) when swapped to
                        &arguments[i]);

    }

    Serial.println(F("Starting coroutines...\n"));
    avr_swapcontext(&return_to_main, &coroutine_state[0]);
    Serial.println(F("Done.\n"));
}

void setup(void)
{
    Serial.begin(9600);
    while (!Serial);
}

void loop(void)
{
    delay(2000);
    coroutines_example();
}

```

## Preemptive Task Switching

```
/*
This example shows how preemptive task switching can be implemented on
top of avr_getcontext(), avr_makecontext(), AVR_SAVE_CONTEXT_GLOBAL_POINTER(),
AVR_RESTORE_CONTEXT_GLOBAL_POINTER() and a hardware timer, which
generates interrupts.

The main idea is quite simple. There are two tasks, both run
indefinitely: one enables the built-in LED, the other one disables
it. System timer, which is implemented on top of watchdog running in
interrupt mode, ticks every one second and switches the tasks in Round
Robin fashion during the tick. Thus, the LED does not remain enabled
or disabled for more than one second.

One notable interesting point here is that we convert the initial
execution context of an MCU into a switchable task. This allows loop()
function to work as expected. We use it to enable the LED. To do so we
allocate the first element of the 'tasks' array for the initial MCU
execution context, we modify the global variables used for task
switching in such a way that during the first interrupt tick the
initial execution context gets saved into the first element of the 'tasks'
array.

Please keep in mind that our intention here is to show how task
switching can be performed in a preemptive task executive, not to
implement an RTOS in one Arduino sketch. In a real RTOS system timer
would tick at a much higher rate, and on every tick, it would perform
much more complicated scheduling code.

Nevertheless, if you are brave enough to write your own RTOS, this
tiny sketch might be a good start.
*/

#include <avr/wdt.h>
#include <avr/sleep.h>

#include "avr-context/avrcontext.h"
#include "avr-context/avrcontext_impl.h"

//// Global variables

extern "C" {
avr_context_t *volatile current_task_ctx; // current task context
}
static size_t current_task_num; // current task index
static avr_context_t dummy_ctx; // never going to be used
static avr_context_t tasks[2]; // contexts for tasks
static uint8_t disabler_stack[128]; // stack for the disabler_task

// This function is the second task body.
// It tries to disable the built-in LED (forever).
static void disabler_task(void *)
{
    for (;;)
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

// This function starts system timer. We use the watchdog timer
// because it is unused by default on Arduino boards.
void start_system_timer(void)
{
    cli(); // disable interrupts
    MCUSR &= ~(1<<WDRF);
    wdt_reset(); // reset watchdog timer
    // configure WD timer
    WDTCSR |= 1 << WDCE | 1 << WDE; // enable WD timer configuration mode
    WDTCSR = 0; // reset WD timer
    wdt_enable(WDTO_1S); // configure period
    WDTCSR |= 1 << WDIE; // use WD timer in interrupt mode
    sei(); // enable interrupts
}

void setup(void)
{
    // Enable builtin LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Initialise dummy context.
    //
    // Actually, we could avoid initialising it. If our intention was
    // to implement a real thread manager/operating system we could
    // create a context which upon activation after task completion
    // would run scheduler to choose the next task.
    //
    // Our tasks run code in endless loops, so this context never gets
    // activated.
    //
    // Our tasks run code in endless loops, so this context never gets
    // activated.
     avr_getcontext(&dummy_ctx);

    // Initialise the first task.
    //
    // Convert the currently running code into a first task.  When the
    // system timer ticks for the first time, the current execution
    // context is going to be saved into tasks[0]. See the
    // switch_task() function for the actual task switching code.

    // To put it simply: we hijack the current execution context and
    // make it schedulable.
    current_task_num = 0;
    current_task_ctx = &tasks[0];

    // Initialise the second task.
    //
    // This task starts execution on the first system timer tick.
    avr_getcontext(&tasks[1]);
    avr_makecontext(&tasks[1],
                    (void*)&disabler_stack[0], sizeof(disabler_stack),
                    &dummy_ctx,
                    disabler_task, NULL);
    // start scheduling
    start_system_timer();
    // after returning from this function
    // loop() gets executed (as usual).
}

// This code tries to enable built-in LED (forever).
void loop(void)
{
    digitalWrite(LED_BUILTIN, HIGH);
}


static void switch_task(void)
{
    current_task_num = current_task_num == 0 ? 1 : 0;
    current_task_ctx = &tasks[current_task_num];
}

// System Timer Interrupt System Routine.
//
// Please keep in mind that ISR_NAKED attribute is important, because
// we have to save the current task execution context without changing
// it.
ISR(WDT_vect, ISR_NAKED)
{
    // save the context of the current task
    AVR_SAVE_CONTEXT_GLOBAL_POINTER(
        "cli\n", // disable interrupts during task switching
        current_task_ctx);
    switch_task(); // switch to the other task.
    WDTCSR |= 1 << WDIE; // re-enable watchdog timer interrupts to avoid reset
    // restore the context of the task to which we have just switched.
    AVR_RESTORE_CONTEXT_GLOBAL_POINTER(current_task_ctx);
    asm volatile("reti\n"); // return from the interrupt and activate the restored context.
}

```

# Copyright

Copyright (c) 2020 [Artem Boldariev](https://chaoticlab.io/).

The software distributed under the terms of the MIT/Expat license.

See [LICENSE.txt](./LICENSE.txt) for license details.

***

*Made in Ukraine*

