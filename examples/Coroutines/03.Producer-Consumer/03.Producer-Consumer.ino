/*
This example demonstrates how asymmetric coroutines can be used to
solve a well known "producer-consumer" problem.

Coroutines communicate using the send() and receive() functions.
send() passes a value to the consumer and resumes it, receive() gets
value from the producer and returns control back to it. These actions
are repeated indefinitely.

It is a consumer-driven design because the program starts by calling
the consumer. When the consumer needs an item, it resumes the
producer, which runs until it has an item to give to the consumer, and
then pauses until the consumer restarts it again.

When being uploaded to an Arduino board, this sketch produces a pair
of messages via serial port every one second in the following format:

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

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avrcontext_arduino.h>

#define STACK_SIZE 128

static avr_coro_t producer, consumer;
static uint8_t producer_stack[STACK_SIZE], consumer_stack[STACK_SIZE];

static void send(avr_coro_t *to, int value)
{
    void *data = &value;
    avr_coro_resume(to, &data);
}

static int receive(avr_coro_t *self)
{
    int *data = NULL;
    avr_coro_yield(self, (void **)&data);
    return *data;
}

static void *producer_func(avr_coro_t *, void *)
{
    int num = 0;
    for (;;)
    {
        Serial.print(F("Produced: "));
        Serial.println(num);
        send(&consumer, num);
        num++;
        delay(1000);
    }
    return NULL;
}

static void *consumer_func(avr_coro_t *self, void *)
{
    for (;;)
    {
        int received = receive(self);
        Serial.print(F("Consumed: "));
        Serial.println(received);
    }
    return NULL;
}


void setup()
{
    avr_coro_init(&producer,
                  &producer_stack[0], STACK_SIZE,
                  producer_func);
    avr_coro_init(&consumer,
                  &consumer_stack[0], STACK_SIZE,
                  consumer_func);
    Serial.begin(9600);
    while(!Serial)
        ;
    avr_coro_resume(&consumer, NULL);
    avr_coro_resume(&producer, NULL);
    // unreachable
}

void loop()
{
    // unreachable
}

