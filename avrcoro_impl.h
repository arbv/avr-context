/*
  Author: Artem Boldariev <artem@boldariev.com>
  The software distributed under the terms of the MIT/Expat license.

  See LICENSE.txt for license details.
*/

#ifndef AVRCORO_IMPL_H
#define AVRCORO_IMPL_H

#ifdef __AVR__

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */
static void avr_coro_trampoline(avr_coro_t *coro);
#ifdef __cplusplus
}
#endif /*__cplusplus */

static void avr_coro_trampoline(avr_coro_t *coro)
{
    avr_coro_func_t funcp = (avr_coro_func_t)coro->funcp;
    void *ret = funcp(coro, coro->data);
    coro->data = ret;
    coro->status = (char)AVR_CORO_DEAD;
}

int avr_coro_init(avr_coro_t *coro,
                  void *stackp, const size_t stack_size,
                  avr_coro_func_t funcp)
{
    if (coro == NULL || stackp == NULL || stack_size == 0 || funcp == NULL)
    {
        return 1;
    }
    coro->status = (char)AVR_CORO_SUSPENDED;
    coro->funcp = (void *)funcp;
    avr_getcontext(&coro->exec);
    avr_makecontext(&coro->exec,
                    stackp, stack_size,
                    &coro->ret,
                    (void(*)(void *))avr_coro_trampoline, coro);
    return 0;
}

int avr_coro_resume(avr_coro_t *coro, void **data)
{
    if (coro == NULL || coro->status != (char)AVR_CORO_SUSPENDED)
    {
        return 1;
    }
    coro->status = (char)AVR_CORO_RUNNING;
    coro->data = data == NULL ? NULL : *data;
    avr_swapcontext(&coro->ret, &coro->exec);
    if (data != NULL)
    {
        *data = coro->data;
    }
    return 0;
}

int avr_coro_yield(avr_coro_t *self, void **data)
{
    if (self == NULL || self->status != (char)AVR_CORO_RUNNING)
    {
        return 1;
    }
    self->status = (char)AVR_CORO_SUSPENDED;
    self->data = data == NULL ? NULL : *data;
    avr_swapcontext(&self->exec, &self->ret);
    if (data != NULL)
    {
        *data = self->data;
    }
    return 0;
}

avr_coro_state_t avr_coro_state(const avr_coro_t *coro)
{
    return coro == NULL || coro->status < AVR_CORO_SUSPENDED || coro->status >= AVR_CORO_ILLEGAL ? AVR_CORO_ILLEGAL : (avr_coro_state_t)coro->status;
}

#endif /* __AVR__ */
#endif /* AVRCORO_IMPL_H */

