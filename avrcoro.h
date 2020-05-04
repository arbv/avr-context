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
    AVR_CORO_SLEEPING = 0,
    AVR_CORO_RUNNING,
    AVR_CORO_DEAD,
    AVR_CORO_ILLEGAL,
} avr_coro_state_t;

typedef struct avr_coro_t_ {
    char status;
    avr_context_t ret;
    avr_context_t exec;
    void *data;
    void *funcp;
} avr_coro_t;

typedef void *(*avr_coro_func_t)(avr_coro_t *, void *);

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */
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

