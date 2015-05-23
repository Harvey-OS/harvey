.text

.globl longjmp
longjmp:
        movq    0(%rdi), %rbx
        movq    8(%rdi), %r12
        movq    16(%rdi), %r13
        movq    24(%rdi), %r14
        movq    32(%rdi), %r15
        movq    40(%rdi), %rbp
        movq    48(%rdi), %rsp
        movl    %esi, %eax
        test    %eax, %eax      /* if val != 0          */
        jnz     1f              /*      return val      */
        incl    %eax            /* else return 1        */
1:
        movq    56(%rdi), %rdx  /* return to caller of setjmp */
        jmp     *%rdx

.globl setjmp
setjmp:
        movq    %rbx, 0(%rdi)
        movq    %r12, 8(%rdi)
        movq    %r13, 16(%rdi)
        movq    %r14, 24(%rdi)
        movq    %r15, 32(%rdi)
        movq    %rbp, 40(%rdi)
        popq    %rdx            /* return address */
        movq    %rsp, 48(%rdi)
        movq    %rdx, 56(%rdi)
        xorl    %eax, %eax      /* return 0 */
        jmp     *%rdx
