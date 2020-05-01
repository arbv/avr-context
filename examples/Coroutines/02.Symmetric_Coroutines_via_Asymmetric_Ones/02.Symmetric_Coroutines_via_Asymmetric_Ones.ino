/*
This example demonstrates how one could implement symmetric coroutines
on top of the asymmetric ones.

The most important difference between the symmetric and asymmetric
coroutines is that the asymmetric ones cannot pass control to the
other coroutine directly: they can return control only back to the
coroutine invoker. Nevertheless, they are no less powerful.

To bypass the above-mentioned limitation, a coroutine, which yields
control, passes the information about the coroutine which it wants to
activate to a dispatching loop. The loop, in turn, activates the next
coroutine.

It is a version of the other example, which was implemented directly
on top of avr_makecontext(), avr_swapcontext().

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
static avr_coro_t coroutine_state[2];      // container to remember context

// switch current coroutine (0 -> 1 -> 0 -> 1 ...)
static void yield_to_next(avr_coro_t *self, const size_t current_num)
{
    // Suspends the current coroutine and passes
    // the number of the next coroutine to the dispatcher thread.
    size_t yield_to = current_num == 1 ? 0 : 1;
    void *data = &yield_to;
    avr_coro_yield(self, &data);
}

static void *coroutine(avr_coro_t *self, size_t *data)
{
    const size_t coroutine_number = *data;
    for (size_t i = 0; i < 5; i++)
    {
        Serial.print(F("Coroutine "));
        Serial.print(coroutine_number);
        Serial.print(F(" counts i="));
        Serial.print(i);
        Serial.print(F(" (&i=0x"));
        Serial.print((uintptr_t)&i, HEX);
        Serial.println(F(")"));
        yield_to_next(self, coroutine_number);
    }
    return NULL;
}

static void coroutines_example(void)
{
    // set up
    for (size_t i = 0; i < 2; i++)
    {
        // initialise coroutines
        avr_coro_init(&coroutine_state[i],
                        (void *)stack[i], STACK_SIZE, // set up per-coroutine stack
                        (avr_coro_func_t)coroutine //let coroutine_state[i] perform a call to coroutine(i) when swapped to
                        );

    }
    Serial.println(F("Starting coroutines...\n"));
    // Dispatcher loop
    size_t yield_to = 0; // start from the first
    for (;;)
    {
        void *data = (void *)&yield_to; // initialise data which gets passed to the coroutine
        avr_coro_resume(&coroutine_state[yield_to], &data);
        // (Please keep in mind that at this point the value in data has changed.
        // We are going to use it at the end of the loop.)

        // Stop when a coroutine has died (the function returned).
        // Please notice that in this case only one of the coroutines
        // reaches the point when its function returns.
        if (avr_coro_status(&coroutine_state[yield_to]) == AVR_CORO_DEAD)
        {
            break;
        }
        // Fetch the number of the next coroutine.
        yield_to = *((int *)data);
    }
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

