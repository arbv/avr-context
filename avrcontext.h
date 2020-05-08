/*
  Author: Artem Boldariev <artem@boldariev.com>
  The software distributed under the terms of the MIT/Expat license.

  See LICENSE.txt for license details.
*/

#ifndef AVRCONTEXT_H
#define AVRCONTEXT_H

#ifdef __AVR__

/* AVR machine context definition. Please keep the corresponding
 * routines/macros synchronised with this definition. */
typedef struct avr_context_t_ {
    uint8_t sreg;
    uint8_t r[32];
    union {
        struct {
            uint8_t low;
            uint8_t high;
        } part;
        void *ptr;
    } pc;
    union {
        struct {
            uint8_t low;
            uint8_t high;
        } part;
        void *ptr;
    } sp;
} avr_context_t;

typedef void (*avr_context_func_t)(void *);

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */
/*
The four functions avr_getcontext(), avr_setcontext(),
avr_swapcontext(), and avr_makecontext() provide facility for context
switching between multiple threads of execution.

These functions provide more or less direct substitutes to the
functions getcontext(), setcontext(), swapcontext(), and makecontext()
which used to be a part of the POSIX standard. It is recommended to
read about them before using the functions from this library:

https://pubs.opengroup.org/onlinepubs/009695399/functions/getcontext.html
https://pubs.opengroup.org/onlinepubs/009695399/functions/makecontext.html

http://man7.org/linux/man-pages/man3/setcontext.3.html
http://man7.org/linux/man-pages/man3/makecontext.3.html

There are multiple important differences between the functions from
this library and their equivalents from the POSIX standard:

a) These functions do not perform sanity checking of their input
parameters. This is a deliberate decision. They are barely useful on
their own, in most cases, they used as a basis for implementing
higher-level abstractions. Thus, it is better to perform sanity
checking at the higher levels. Moreover, these functions tend to
appear in the hot spots of the application, where additional sanity
checks lead to extra wasted CPU cycles. This might be not acceptable
behaviour when programming MCUs.

b) As the direct consequence of the above: there is no error
reporting. These functions will gladly accept whatever you pass to
them. If you have passed to them something unintentionally, that could
lead to undefined behaviour (e.g. it could lead to a mess in an MCU's
memory).

The two paragraphs above mean that these functions built around the
GIGO principle (Garbage In, Garbage Out). One could call them
"unsafe", but I prefer to call them "sharp".
*/
/*
The function avr_getcontext() initialises the structure pointed at by
'cp' to the currently active context.

The function avr_setcontext() restores the context from the structure
pointed at by 'cp.'  The context should have been obtained by a call
to avr_getcontext(), avr_swapcontext() or avr_makecontext(). In the
case, when the context has been obtained by a call to the
avr_getcontext() or avr_swapcontext() program execution continues as
if the call has just returned. The function avr_setcontext() never
returns.

The function avr_swapcontext() saves the current context in the
structure pointed to by 'cp' and then activates the context pointed to
by 'cp' as one operation. It may return later when context pointed to
by 'oucp' gets activated.

The function avr_makecontext() modifies the context obtained by a call
to avr_getcontext() and pointed to by 'oucp' in such a way that upon
activation the function 'funcp' gets called with the 'funcargp' value
passed as its argument. When this function returns, the successor
context 'successor_cp' gets activated. Thus, the successor context
MUST be a valid context before the activation of the context pointed
to by 'cp.'

Before invoking the avr_makecontext(), the caller must allocate a new
stack for the modifiable context and pass pointer to it (stackp) and
the size of the memory region (stack_size).
*/
extern void avr_getcontext(avr_context_t *cp);
extern void avr_setcontext(const avr_context_t *cp);
extern void avr_swapcontext(avr_context_t *oucp, const avr_context_t *cp);
extern void avr_makecontext(avr_context_t *cp,
                            void *stackp, const size_t stack_size,
                            const avr_context_t *successor_cp,
                            avr_context_func_t funcp, void *funcargp);

#ifdef __cplusplus
}
#endif /*__cplusplus */

/*
It is highly unlikely to understand the following code without:
a) basic understanding of AVR assembly language;
b) reading about avr-gcc's ABI beforehand (https://gcc.gnu.org/wiki/avr-gcc).
*/

/* Some utility macros, most of them define offsets to simplify working on the
   machine context structure from within assembly code. */
#define AVR_CONTEXT_ASMCONST(name, value)\
    __asm__(".equ " #name "," #value "\n");

#define AVR_CONTEXT_OFFSET_PC_L 33
AVR_CONTEXT_ASMCONST(AVR_CONTEXT_OFFSET_PC_L, 33)

#define AVR_CONTEXT_OFFSET_PC_H 34
AVR_CONTEXT_ASMCONST(AVR_CONTEXT_OFFSET_PC_H, 34)

#define AVR_CONTEXT_OFFSET_SP_L 35
AVR_CONTEXT_ASMCONST(AVR_CONTEXT_OFFSET_SP_L, 35)

#define AVR_CONTEXT_OFFSET_SP_H 36
AVR_CONTEXT_ASMCONST(AVR_CONTEXT_OFFSET_SP_H, 36)

#define AVR_CONTEXT_BACK_OFFSET_R26 9
AVR_CONTEXT_ASMCONST(AVR_CONTEXT_BACK_OFFSET_R26, 9)

/*
AVR_SAVE_CONTEXT and AVR_RESTORE_CONTEXT macros provide the generic
facility for saving/restoring an AVR CPU context.

Using them directly needed only in rare cases, please consider using
*context() functions described above. These macros may be useful when
implementing Interrupt System Routines, though.

Please keep in mind that *context() functions implemented on top of
AVR_SAVE_CONTEXT and AVR_RESTORE_CONTEXT.

The code in the macros expects that the pointer register Z (R31:R30)
contains the address of an avr_context_t structure. Additionally to
that, the code expects to find the return address on top of the stack
(like after the CALL family of instructions, or during an interrupt
handling). In fact, the macros are barely useful anywhere else except
the naked interrupt system routines.

The argument named 'load_address_to_Z_code' should be a string
constant which contains assembly instructions. These instructions
should load the address of an avr_context_t to Z. Before executing
this code, the original values in register Z preserved.

The argument named 'presave_code' should be a string constant which
contains assembly instructions which get executed right after
preserving the SREG register value. If, for example, you want to
disable interrupts before saving the context, it is the right place to
do it.

Please note that, in general, after executing the code in
'presave_code', 'load_address_to_Z_code', one should restore the
original values of the general-purpose registers, the stack pointer,
and, in most cases, the status register.

One could have noted that using these macros is quite cumbersome, but
this is a very low-level code and in some cases, it is rather hard (or
impossible) to provide a reasonable interface for the low-level
functionality.

I want to stress it one more time: if in doubt please use
avr_getcontext()/avr_setcontext()/avr_swapcontext()/avr_makecontext().
 */
#define AVR_SAVE_CONTEXT(presave_code, load_address_to_Z_code)          \
    __asm__ __volatile__(                                                       \
        /* push Z*/                                                     \
        "push r30\n"                                                    \
        "push r31\n"                                                    \
        /* Save SREG value using R0 as a temporary register. */         \
        "in r30, __SREG__\n"                                            \
        "\n" presave_code "\n"                                          \
        "push r0\n"                                                     \
        /* Push SREG value. */                                          \
        "push r30\n"                                                    \
        /* Load address of a context pointer structure to Z */          \
        "\n" load_address_to_Z_code "\n"                                \
        /* save SREG to the context structure */                        \
        "pop r0\n"                                                      \
        "st Z+, r0\n"                                                   \
        /* Restore initial R0 value. */                                 \
        "pop r0 \n"                                                     \
        /* Save general purpose register values. */                     \
        "st z+, r0\n"                                                   \
        "st z+, r1\n"                                                   \
        "st z+, r2\n"                                                   \
        "st z+, r3\n"                                                   \
        "st z+, r4\n"                                                   \
        "st z+, r5\n"                                                   \
        "st z+, r6\n"                                                   \
        "st z+, r7\n"                                                   \
        "st z+, r8\n"                                                   \
        "st z+, r9\n"                                                   \
        "st z+, r10\n"                                                  \
        "st z+, r11\n"                                                  \
        "st z+, r12\n"                                                  \
        "st z+, r13\n"                                                  \
        "st z+, r14\n"                                                  \
        "st z+, r15\n"                                                  \
        "st z+, r16\n"                                                  \
        "st z+, r17\n"                                                  \
        "st z+, r18\n"                                                  \
        "st z+, r19\n"                                                  \
        "st z+, r20\n"                                                  \
        "st z+, r21\n"                                                  \
        "st z+, r22\n"                                                  \
        "st z+, r23\n"                                                  \
        "st z+, r24\n"                                                  \
        "st z+, r25\n"                                                  \
        "st z+, r26\n"                                                  \
        "st z+, r27\n"                                                  \
        "st z+, r28\n"                                                  \
        "st z+, r29\n"                                                  \
        /* Switch to other index register (Z to Y) as its has been saved at this point */ \
        "mov r28, r30\n"                                                \
        "mov r29, r31\n"                                                \
        /* Restore and save values of registers 30 and 31 (Z) */        \
        "pop r31\n"                                                     \
        "pop r30\n"                                                     \
        "st y+, r30\n"                                                  \
        "st y+, r31\n"                                                  \
        /* Pop and save the return address */                           \
        "pop r30\n" /* high part */                                     \
        "pop r31\n" /* low part */                                      \
        "st y+, r31\n"                                                  \
        "st y+, r30\n"                                                  \
        /* Save the stack pointer to the structure. */                  \
        "in r26, __SP_L__\n"                                            \
        "in r27, __SP_H__\n"                                            \
        "st y+, r26\n"                                                  \
        "st y, r27\n"                                                   \
        /* Push the return address back at the top of the stack. */     \
        "push r31\n" /* low part */                                     \
        "push r30\n" /* high part */                                    \
        /* At this point the context is saved, but registers */         \
        /* 26, 27, 28, 29, 30, and 31 are clobbered. */                 \
        /* In some cases we may not need to restore them, */            \
        /* but let's remain on the clean side and restore their values. */ \
        /* We have to do that because we provide a generic solution. */ \
        "mov r30, r28\n" /* Switch from Y pointer register to Z */      \
        "mov r31, r29\n"                                                \
        /* go to the offset of R26 in the context structure */          \
        "in r28, __SREG__\n" /* save SREG */                            \
        "sbiw r30, AVR_CONTEXT_BACK_OFFSET_R26\n"                       \
        "out __SREG__, r28\n" /* restore SREG */                        \
        /* Restore registers 26-29 */                                   \
        "ld r26, Z+\n"                                                  \
        "ld r27, Z+\n"                                                  \
        "ld r28, Z+\n"                                                  \
        "ld r29, Z+\n"                                                  \
        /* save R28, R29 (Y) on the stack */                            \
        "push r28\n"                                                    \
        "push r29\n"                                                    \
        /* switch to other index register (z to y) and read r30 and r31 */ \
        "mov r28, r30\n"                                                \
        "mov r29, r31\n"                                                \
        "ld r30, Y+\n"                                                  \
        "ld r31, Y\n"                                                   \
        /* Restore R28, R29 (Y) from the stack */                       \
        "pop r29\n"                                                     \
        "pop r28\n")

#define AVR_RESTORE_CONTEXT(load_address_to_Z_code)                     \
    __asm__ __volatile__(                                                       \
        /* load address of a context structure pointer to Z */          \
        "\n"                                                            \
        load_address_to_Z_code                                          \
        "\n"                                                            \
        /* Go to the end of the context structure and */                \
        /* start restoring it from there. */                            \
        "adiw r30, AVR_CONTEXT_OFFSET_SP_H\n"                           \
        /* Restore the saved stack pointer. */                          \
        "ld r0, Z\n"                                                    \
        "out __SP_H__, r0\n"                                            \
        "ld r0, -Z\n"                                                   \
        "out __SP_L__, r0\n"                                            \
        /* Put the saved return address (PC) back on the top of the stack. */ \
        "ld r1, -Z\n" /* high part */                                   \
        "ld r0, -Z\n" /* low part */                                    \
        "push r0\n"                                                     \
        "push r1\n"                                                     \
        /* Temporarily switch pointer from Z to Y,*/                    \
        /* restore r31, r30 (Z) and put them on top of the stack. */    \
        "mov r28, r30\n"                                                \
        "mov r29, r31\n"                                                \
        "ld r31, -Y\n"                                                  \
        "ld r30, -Y\n"                                                  \
        "push r31\n"                                                    \
        "push r30\n"                                                    \
        /* Switch back from Y to Z. */                                  \
        "mov r30, r28\n"                                                \
        "mov r31, r29\n"                                                \
        /* Restore other general purpose registers. */                  \
        "ld r29, -Z\n"                                                  \
        "ld r28, -Z\n"                                                  \
        "ld r27, -Z\n"                                                  \
        "ld r26, -Z\n"                                                  \
        "ld r25, -Z\n"                                                  \
        "ld r24, -Z\n"                                                  \
        "ld r23, -Z\n"                                                  \
        "ld r22, -Z\n"                                                  \
        "ld r21, -Z\n"                                                  \
        "ld r20, -Z\n"                                                  \
        "ld r19, -Z\n"                                                  \
        "ld r18, -Z\n"                                                  \
        "ld r17, -Z\n"                                                  \
        "ld r16, -Z\n"                                                  \
        "ld r15, -Z\n"                                                  \
        "ld r14, -Z\n"                                                  \
        "ld r13, -Z\n"                                                  \
        "ld r12, -Z\n"                                                  \
        "ld r11, -Z\n"                                                  \
        "ld r10, -Z\n"                                                  \
        "ld r9, -Z\n"                                                   \
        "ld r8, -Z\n"                                                   \
        "ld r7, -Z\n"                                                   \
        "ld r6, -Z\n"                                                   \
        "ld r5, -Z\n"                                                   \
        "ld r4, -Z\n"                                                   \
        "ld r3, -Z\n"                                                   \
        "ld r2, -Z\n"                                                   \
        "ld r1, -Z\n"                                                   \
        "ld r0, -Z\n"                                                   \
        /* Restore SREG */                                              \
        "push r0\n"                                                     \
        "ld r0, -Z\n"                                                   \
        "out __SREG__, r0\n"                                            \
        "pop r0\n"                                                      \
        /* Restore r31, r30 (Z) from the stack. */                      \
        "pop r30\n"                                                     \
        "pop r31\n")

/*
AVR_RESTORE_CONTEXT_GLOBAL_POINTER and AVR_SAVE_CONTEXT_GLOBAL_POINTER
macros provide the generic facility for saving/restoring an AVR CPU
context to/from a structure via a global pointer variable.

Please make sure that the pointer defined as a volatile pointer. If
you use C++, wrap the pointer declaration in 'extern "C" { .. }' to
avoid name mangling.

Example pointer definition:

#ifdef __cplusplus
extern "C" {
#endif
avr_context_t *volatile avr_current_ctx;
#ifdef __cplusplus
}
#endif

As these macros implemented on top of the AVR_SAVE_CONTEXT and
AVR_RESTORE_CONTEXT, please make sure that you understand how they
work.
*/
#define AVR_SAVE_CONTEXT_GLOBAL_POINTER(presave_code, global_context_pointer) \
    AVR_SAVE_CONTEXT(                                                   \
        presave_code,                                                  \
        "lds ZL, "#global_context_pointer"\n"                           \
        "lds ZH, "#global_context_pointer" + 1\n")

#define AVR_RESTORE_CONTEXT_GLOBAL_POINTER(global_context_pointer)  \
    AVR_RESTORE_CONTEXT(                                            \
        "lds ZL, " #global_context_pointer "\n"                     \
        "lds ZH, " #global_context_pointer " + 1\n")

#endif /* __AVR__ */

#endif /* AVRCONTEXT_H */

