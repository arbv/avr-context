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

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avrcontext_arduino.h>

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

