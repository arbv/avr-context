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

***
  Author: Artem Boldariev <artem@boldariev.com>
  This example code is in the public domain.

  See UNLICENSE.txt in the examples directory for license details.
*/

#include <avr/wdt.h>
#include <avr/sleep.h>

#include <avrcontext_arduino.h>

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

