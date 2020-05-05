/*
This example demonstrates how asymmetric coroutines can be used to
solve the "producer-filter-consumer" problem (one can also call the
filter as "mediator").

Coroutines communicate using the send() and receive() functions. The
function send() passes a value to the receiver and returns control
back to the invoker, the function receive() gets value from the sender
and resumes it.

It is a consumer-driven design because the program starts by calling a
consumer. The consumer receives a value from a filter and thus resumes
it. Then the filter receives the value from a producer (again, by
resuming it) and doubles it. After that, the filter sends the doubled
value to the consumer. By doing so it resumes the consumer. Then the
whole process repeats.

When being uploaded to an Arduino board, this sketch produces a
triplet of messages via serial port every one second in the following
format.

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

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avrcontext_arduino.h>

#define STACK_SIZE 128

static avr_coro_t producer, consumer, filter;
static uint8_t producer_stack[STACK_SIZE], consumer_stack[STACK_SIZE], filter_stack[STACK_SIZE];

static void send(avr_coro_t *self, int value)
{
    void *data = &value;
    avr_coro_yield(self, &data);
}

static int receive(avr_coro_t *from)
{
    int *data = NULL;
    avr_coro_resume(from, (void **)&data);
    return *data;
}

static void *producer_func(avr_coro_t *self, void *)
{
    int num = 0;
    for (;;)
    {
        Serial.print(F("Produced: "));
        Serial.println(num);
        send(self, num);
        num++;
    }
    return NULL;
}

static void *consumer_func(avr_coro_t *, void *)
{
    for (;;)
    {
        const int received = receive(&filter);
        Serial.print(F("Consumed: "));
        Serial.println(received);
        delay(1000);
    }
    return NULL;
}

static void *filter_func(avr_coro_t *self, void *)
{
    for (;;)
    {
        const int received = receive(&producer);
        const int filtered = received * 2;
        Serial.print(F("Filtered: "));
        Serial.println(filtered);
        send(self, filtered);
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
    avr_coro_init(&filter,
                  &filter_stack[0], STACK_SIZE,
                  filter_func);
    Serial.begin(9600);
    while(!Serial)
        ;
    avr_coro_resume(&consumer, NULL);
    // unreachable
}

void loop()
{
    // unreachable
}

