/*
This example demonstrates how avr_makecontext() and avr_swapcontext()
could be used.

It also might give some clues about how coroutines could be implemented.

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

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avrcontext_arduino.h>

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

