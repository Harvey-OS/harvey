1i\
\#include \<setjmp.h\> \
jmp_buf jmp9998, jmp32; int lab31=0;
s/goto lab31 ; */{lab31=1; return;}/
s/goto lab32/longjmp(jmp32,1)/
s/goto lab9998/longjmp(jmp9998,1)/g
s/lab31://
s/lab32://
s/hack1 \(\) ;/if(setjmp(jmp9998)==1) goto lab9998;if(setjmp(jmp32)==0)for(;;)/
s/hack2 \(\)/break/
/^void mainbody/,$s/while \( true/while (lab31==0/
