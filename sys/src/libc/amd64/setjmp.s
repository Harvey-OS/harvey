.text

.globl longjmp
longjmp:
        movl    16(%esp), %eax   /* r */
        cmpl    $0, %eax
        jne     ok              /* ansi: "longjmp(0) => longjmp(1)" */
        movl    $1, %eax                /* bless their pointed heads */
ok:
        movq    8(%rsp), %rdx           /* l */
        movq    0(%rdx), %rsp           /* restore sp */
        movq    8(%rdx), %rbx           /* put return pc on the stack */
        movq    %rbx, 0(%rsp)
        movq    16(%rdx), %rbx           /* restore bx */
        movq    20(%rdx), %rsi          /* restore si */
        movq    24(%rdx), %rdi          /* restore di */
        movq    28(%rdx), %rbp  /* restore bp */
        ret

.globl setjmp
setjmp:
        movq    8(%rsp), %rdx           /* l */
        movq    %rsp, 0(%rdx)           /* store sp */
        movq    0(%rsp), %rax           /* store return pc */
        movq    %rax, 8(%rdx)
        movq    %rbx, 16(%rdx)           /* store bx */
        movq    %rsi, 20(%rdx)          /* store si */
        movq    %rdi, 24(%rdx)          /* store di */
        movq    %rbp, 28(%rdx)  /* store bp */
        movq    $0, %rax                        /* return 0 */
        ret
