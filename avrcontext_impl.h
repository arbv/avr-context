/*
  Author: Artem Boldariev <artem@boldariev.com>
  The software distributed under the terms of the MIT/Expat license.

  See LICENSE.txt for license details.
*/

/*
This file contains definitions of the context manipulation functions.
It meant to be included after 'avrcontext.h'.
In general, you should include it only once across the project.
*/

#ifndef AVRCONTEXT_IMPL_H
#define AVRCONTEXT_IMPL_H

#ifdef __AVR__

void avr_getcontext(avr_context_t *cp) __attribute__ ((naked));
void avr_getcontext(avr_context_t *cp)
{
    (void)cp; /* to avoid compiler warnings */
    AVR_SAVE_CONTEXT(
        "",
        "mov r30, r24\n"
        "mov r31, r25\n");
    __asm__ __volatile__ ("ret\n");
}

void avr_setcontext(const avr_context_t *cp) __attribute__ ((naked));
void avr_setcontext(const avr_context_t *cp)
{
    (void)cp; /* to avoid compiler warnings */
    AVR_RESTORE_CONTEXT(
        "mov r30, r24\n"
        "mov r31, r25\n");
    __asm__ __volatile__ ("ret\n");
}


void avr_swapcontext(avr_context_t *oucp, const avr_context_t *ucp)  __attribute__ ((naked));
void avr_swapcontext(avr_context_t *oucp, const avr_context_t *ucp)
{
    (void)oucp; /* to avoid compiler warnings */
    (void)ucp;
    AVR_SAVE_CONTEXT(
        "",
        "mov r30, r24\n"
        "mov r31, r25\n");
    AVR_RESTORE_CONTEXT(
        "mov r30, r22\n"
        "mov r31, r23\n");
    __asm__ __volatile__ ("ret\n");
}

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus **/
static void avr_makecontext_callfunc(const avr_context_t *successor, void (*func)(void *), void *funcarg);
#ifdef __cplusplus
}
#endif /*__cplusplus */
static void avr_makecontext_callfunc(const avr_context_t *successor, void (*func)(void *), void *funcarg)
{
    func(funcarg);
    avr_setcontext(successor);
}

void avr_makecontext(avr_context_t *cp, void *stackp, const size_t stack_size, const avr_context_t *successor_cp, void (*funcp)(void *), void *funcargp)
{
    uint16_t addr;
    uint8_t *p = (uint8_t *)&addr;
    /* initialise stack pointer and program counter */
    cp->sp.ptr = ((uint8_t *)stackp + stack_size - 1);
    cp->pc.ptr = (void *)avr_makecontext_callfunc;
    /* initialise registers to pass arguments to avr_makecontext_callfunc */
    /* successor: registers 24,25; func registers 23, 22; funcarg: 21, 20. */
    addr = (uint16_t)successor_cp;
    cp->r[24] = p[0];
    cp->r[25] = p[1];
    addr = (uint16_t)funcp;
    cp->r[22] = p[0];
    cp->r[23] = p[1];
    addr = (uint16_t)funcargp;
    cp->r[20] = p[0];
    cp->r[21] = p[1];
}

#if __cplusplus >= 201103L
/*
See bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49171
*/
inline void avr_context_sanity_checks(void)
{
    avr_context_t test;
    static_assert(reinterpret_cast<uintptr_t>(&test) == reinterpret_cast<uintptr_t>(&test.sreg));
    static_assert(sizeof(avr_context_t) == 37);
    static_assert(reinterpret_cast<uintptr_t>(&test.sp.part.low) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_SP_L);
    static_assert(reinterpret_cast<uintptr_t>(&test.sp.part.high) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_SP_H);
    static_assert(reinterpret_cast<uintptr_t>(&test.pc.part.low) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_PC_L);
    static_assert(reinterpret_cast<uintptr_t>(&test.pc.part.high) - reinterpret_cast<uintptr_t>(&test) == AVR_CONTEXT_OFFSET_PC_H);
    static_assert(reinterpret_cast<uintptr_t>(&test.sp.part.high) - reinterpret_cast<uintptr_t>(&test.r[26]) == AVR_CONTEXT_BACK_OFFSET_R26);
}
#endif /* __cplusplus */

#endif /* __AVR__ */

#endif /* AVRCONTEXT_IMPL_H */

