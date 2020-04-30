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

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avrcontext_arduino.h>

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

