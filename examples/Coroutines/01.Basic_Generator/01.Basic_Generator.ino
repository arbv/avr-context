/*
This example demonstrates how an asymmetric coroutine might be used as
a generator.  Every time the coroutine gets invoked it generates the
next number in sequence (0...N) and returns it back to the invoker.

One of the important points of this example is to demonstrate how a
value from the coroutine can be passed back to the invoker.

When being uploaded to an Arduino board, this sketch print numbers in
increasing order via serial port every one second, e.g.:

0
1
2
3
4
5
...

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avrcontext_arduino.h>

#define STACK_SIZE 64

static avr_coro_t int_generator;
static uint8_t stack[STACK_SIZE];

static void *gen_func(avr_coro_t *self, void *)
{
    int n = 0;
    for (;;)
    {
        // Return control back to the invoker.
        // Also pass to it the current number.
        void *data = (void*)&n;
        avr_coro_yield(self, &data);
        n++;
    }
    return NULL; // unreachable
}

static void gen_init(void)
{
    avr_coro_init(&int_generator,
                  (void*)&stack[0], sizeof(stack),
                  gen_func);
}

static int gen_next(void)
{
    int *data = NULL;
    avr_coro_resume(&int_generator, (void **)&data);
    return *data; // Return the value we have got from the coroutine.
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    gen_init();
}

void loop() {
    Serial.println(gen_next());
    delay(1000);
}

