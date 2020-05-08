/*
  Author: Artem Boldariev <artem@boldariev.com>
  The software distributed under the terms of the MIT/Expat license.

  See LICENSE.txt for license details.
*/

#ifndef AVRCORO_H
#define AVRCORO_H

#ifdef __AVR__

/* Coroutine state codes */
typedef enum avr_coro_state_t_ {
    AVR_CORO_SUSPENDED = 0,
    AVR_CORO_RUNNING,
    AVR_CORO_DEAD,
    AVR_CORO_ILLEGAL,
} avr_coro_state_t;

/* Coroutine definition */
typedef struct avr_coro_t_ {
    char status;
    avr_context_t ret;
    avr_context_t exec;
    void *data;
    void *funcp;
} avr_coro_t;

/* Coroutine function type */
typedef void *(*avr_coro_func_t)(avr_coro_t *, void *);

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */
/*
There are four functions that implement the asymmetric stackful
coroutine facility: avr_coro_init(), avr_coro_resume(),
avr_coro_yield(), avr_coro_state().

A coroutine is represented by the "avr_coro_t"
opaque data type. A coroutine can be in one of the following states.

1. Suspended
2. Running
3. Dead

All of the functions return 0 on success or 1 on failure with one
notable exception: avr_coro_state() returns either a state of a
coroutine (represented as a member of the "avr_coro_state_t" data
type), or "AVR_CORO_ILLEGAL" status on failure.

Failure status usually means that a wrong argument has been passed to
the function.

The function avr_coro_init() initialises a coroutine, represented by a
structure pointed at by "coro." The coroutine gets initialised in the
suspended state. Upon resumption, the function "func" gets called with
two arguments applied: a pointer to the avr_coro_t structure itself
("self"), an argument passed on at first resumption.

The function avr_coro_resume() resumes execution of a coroutine,
represented by a structure pointed at by "coro." The coroutine might
return control to the invoker by calling the function
avr_coro_yield(). It accepts a currently running coroutine represented
by a structure pointed at by "self."

Both of the functions accept pointer to a pointer as the last argument
("data"). The coroutine and its invoker can exchange data using this
last argument. When the coroutine gets resumed using avr_coro_resume()
for the first time, a value of the pointer, to which "data" points,
gets passed as the second argument to the coroutine function.

After the call to the avr_coro_resume() returns, the pointer to which
"data" points during the call, gets initialised to the value,
correspondingly passed as the last argument of avr_coro_yield().

Symmetrically, after the call to the avr_coro_yield() returns, the
pointer to which "data" points during the call, gets initialised to
the value correspondingly passed as the last argument of
avr_coro_resume().

When the coroutine's function returns (and, thus, becomes dead), the
returned value initialises the pointer to which "data" points during
the corresponding avr_coro_resume() call.

If there is no need to exchange data between the coroutine and its
invoker, one can pass the "NULL" value as the "data" argument to
avr_coro_resume()/avr_coro_yield().

The function avr_coro_state() returns the current status of a
coroutine:

1. If the coroutine is suspended, the function returns
"AVR_CORO_SUSPENDED".

2. If the coroutine is running, the function returns
"AVR_CORO_RUNNING". This value can be obtained only from within the
context of the currently running coroutine.

3. If the coroutine is dead, the function returns "AVR_CORO_DEAD".

The value "AVR_CORO_ILLEGAL" gets returned in the case of error
(e.g. the "NULL" value was passed instead of a pointer to a
coroutine).
*/
extern int avr_coro_init(avr_coro_t *coro,
                         void *stackp, const size_t stack_size,
                         avr_coro_func_t func);
extern int avr_coro_resume(avr_coro_t *coro, void **data);
extern int avr_coro_yield(avr_coro_t *self, void **data);
extern avr_coro_state_t avr_coro_state(const avr_coro_t *coro);
#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* __AVR__ */
#endif /* AVRCORO_H */

