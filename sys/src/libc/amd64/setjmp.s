.text

.globl longjmp
longjmp:
        movq    %rdi, %r9
        movq    0(%r9), %rbx
        movq    8(%r9), %r12
        movq    16(%r9), %r13
        movq    24(%r9), %r14
        movq    32(%r9), %r15
        movq    40(%r9), %rbp
        movq    48(%r9), %rsp
        movl    %esi, %eax
        test    %eax, %eax      /* if val != 0          */
        jnz     1f              /*      return val      */
        incl    %eax            /* else return 1        */
1:
        movq    56(%r9), %r8  /* return to caller of setjmp */

        movq	64(%r9), %rdi /* 1st function argument */
        movq	72(%r9), %rsi /* 2nd function argument */

        jmp     *%r8 

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
