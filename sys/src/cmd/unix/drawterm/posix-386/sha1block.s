# 1 "<stdin>"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "<stdin>"
.text

.p2align 2,0x90

.globl __sha1block
__sha1block:
# 110 "<stdin>"
 pushl %ebp
 subl $((48+80*4)), %esp

 mov %ebx, ((48+80*4)-36-(80*4))(%esp)
 mov %esi, ((48+80*4)-40-(80*4))(%esp)
 mov %edi, ((48+80*4)-44-(80*4))(%esp)

 movl ((48+80*4)+8)(%esp), %eax
 addl ((48+80*4)+12)(%esp), %eax
 movl %eax, ((48+80*4)-32-(80*4))(%esp)

 leal (((48+80*4)-4-(80*4))+15*4)(%esp), %edi
 movl %edi, ((48+80*4)-16-(80*4))(%esp)
 leal (((48+80*4)-4-(80*4))+40*4)(%esp), %edx
 movl %edx, ((48+80*4)-20-(80*4))(%esp)
 leal (((48+80*4)-4-(80*4))+60*4)(%esp), %ecx
 movl %ecx, ((48+80*4)-24-(80*4))(%esp)
 leal (((48+80*4)-4-(80*4))+80*4)(%esp), %edi
 movl %edi, ((48+80*4)-28-(80*4))(%esp)

0:
 leal ((48+80*4)-4-(80*4))(%esp), %ebp

 movl ((48+80*4)+16)(%esp), %edi
 movl (%edi),%eax
 movl 4(%edi),%ebx
 movl %ebx, ((48+80*4)-8-(80*4))(%esp)
 movl 8(%edi), %ecx
 movl 12(%edi), %edx
 movl 16(%edi), %esi

 movl ((48+80*4)+8)(%esp), %ebx

1:
 movl 0(%ebx), %edi; bswap %edi; movl %edi, 0(%ebp); leal 0x5a827999(%edi,%esi,1), %esi; movl %eax, %edi; roll $5,%edi; addl %edi,%esi; movl %ecx, %edi; xorl %edx, %edi; andl ((48+80*4)-8-(80*4))(%esp), %edi; xorl %edx, %edi; addl %edi,%esi; rorl $2,((48+80*4)-8-(80*4))(%esp);
 movl %esi,((48+80*4)-12-(80*4))(%esp)
 movl 4(%ebx), %edi; bswap %edi; movl %edi, 4(%ebp); leal 0x5a827999(%edi,%edx,1), %edx; movl %esi, %edi; roll $5,%edi; addl %edi,%edx; movl ((48+80*4)-8-(80*4))(%esp), %edi; xorl %ecx, %edi; andl %eax, %edi; xorl %ecx, %edi; addl %edi,%edx; rorl $2,%eax;
 movl ((48+80*4)-8-(80*4))(%esp),%esi
 movl 8(%ebx), %edi; bswap %edi; movl %edi, 8(%ebp); leal 0x5a827999(%edi,%ecx,1), %ecx; movl %edx, %edi; roll $5,%edi; addl %edi,%ecx; movl %eax, %edi; xorl %esi, %edi; andl ((48+80*4)-12-(80*4))(%esp), %edi; xorl %esi, %edi; addl %edi,%ecx; rorl $2,((48+80*4)-12-(80*4))(%esp);
 movl 12(%ebx), %edi; bswap %edi; movl %edi, 12(%ebp); leal 0x5a827999(%edi,%esi,1), %esi; movl %ecx, %edi; roll $5,%edi; addl %edi,%esi; movl ((48+80*4)-12-(80*4))(%esp), %edi; xorl %eax, %edi; andl %edx, %edi; xorl %eax, %edi; addl %edi,%esi; rorl $2,%edx;
 movl %esi,((48+80*4)-8-(80*4))(%esp)
 movl 16(%ebx), %edi; bswap %edi; movl %edi, 16(%ebp); leal 0x5a827999(%edi,%eax,1), %eax; movl %esi, %edi; roll $5,%edi; addl %edi,%eax; movl %edx, %edi; xorl ((48+80*4)-12-(80*4))(%esp), %edi; andl %ecx, %edi; xorl ((48+80*4)-12-(80*4))(%esp), %edi; addl %edi,%eax; rorl $2,%ecx;
 movl ((48+80*4)-12-(80*4))(%esp),%esi

 addl $20, %ebx
 addl $20, %ebp
 cmpl ((48+80*4)-16-(80*4))(%esp), %ebp
 jb 1b

 movl 0(%ebx), %edi; bswap %edi; movl %edi, 0(%ebp); leal 0x5a827999(%edi,%esi,1), %esi; movl %eax, %edi; roll $5,%edi; addl %edi,%esi; movl %ecx, %edi; xorl %edx, %edi; andl ((48+80*4)-8-(80*4))(%esp), %edi; xorl %edx, %edi; addl %edi,%esi; rorl $2,((48+80*4)-8-(80*4))(%esp);
 addl $4, %ebx
 MOVL %ebx, ((48+80*4)+8)(%esp)
 MOVL ((48+80*4)-8-(80*4))(%esp),%ebx

 movl (4 -64)(%ebp), %edi; xorl (4 -56)(%ebp), %edi; xorl (4 -32)(%ebp), %edi; xorl (4 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 4(%ebp); leal 0x5a827999(%edi, %edx, 1), %edx; movl %esi, %edi; roll $5, %edi; addl %edi, %edx; movl %ebx, %edi; xorl %ecx, %edi; andl %eax, %edi; xorl %ecx, %edi; addl %edi, %edx; rorl $2, %eax;
 movl (8 -64)(%ebp), %edi; xorl (8 -56)(%ebp), %edi; xorl (8 -32)(%ebp), %edi; xorl (8 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 8(%ebp); leal 0x5a827999(%edi, %ecx, 1), %ecx; movl %edx, %edi; roll $5, %edi; addl %edi, %ecx; movl %eax, %edi; xorl %ebx, %edi; andl %esi, %edi; xorl %ebx, %edi; addl %edi, %ecx; rorl $2, %esi;
 movl (12 -64)(%ebp), %edi; xorl (12 -56)(%ebp), %edi; xorl (12 -32)(%ebp), %edi; xorl (12 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 12(%ebp); leal 0x5a827999(%edi, %ebx, 1), %ebx; movl %ecx, %edi; roll $5, %edi; addl %edi, %ebx; movl %esi, %edi; xorl %eax, %edi; andl %edx, %edi; xorl %eax, %edi; addl %edi, %ebx; rorl $2, %edx;
 movl (16 -64)(%ebp), %edi; xorl (16 -56)(%ebp), %edi; xorl (16 -32)(%ebp), %edi; xorl (16 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 16(%ebp); leal 0x5a827999(%edi, %eax, 1), %eax; movl %ebx, %edi; roll $5, %edi; addl %edi, %eax; movl %edx, %edi; xorl %esi, %edi; andl %ecx, %edi; xorl %esi, %edi; addl %edi, %eax; rorl $2, %ecx;

 addl $20, %ebp

2:
 movl (0 -64)(%ebp), %edi; xorl (0 -56)(%ebp), %edi; xorl (0 -32)(%ebp), %edi; xorl (0 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 0(%ebp); leal 0x6ed9eba1(%edi, %esi, 1), %esi; movl %eax, %edi; roll $5, %edi; addl %edi, %esi; movl %ebx, %edi; xorl %ecx, %edi; xorl %edx, %edi; addl %edi, %esi; rorl $2, %ebx;
 movl (4 -64)(%ebp), %edi; xorl (4 -56)(%ebp), %edi; xorl (4 -32)(%ebp), %edi; xorl (4 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 4(%ebp); leal 0x6ed9eba1(%edi, %edx, 1), %edx; movl %esi, %edi; roll $5, %edi; addl %edi, %edx; movl %eax, %edi; xorl %ebx, %edi; xorl %ecx, %edi; addl %edi, %edx; rorl $2, %eax;
 movl (8 -64)(%ebp), %edi; xorl (8 -56)(%ebp), %edi; xorl (8 -32)(%ebp), %edi; xorl (8 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 8(%ebp); leal 0x6ed9eba1(%edi, %ecx, 1), %ecx; movl %edx, %edi; roll $5, %edi; addl %edi, %ecx; movl %esi, %edi; xorl %eax, %edi; xorl %ebx, %edi; addl %edi, %ecx; rorl $2, %esi;
 movl (12 -64)(%ebp), %edi; xorl (12 -56)(%ebp), %edi; xorl (12 -32)(%ebp), %edi; xorl (12 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 12(%ebp); leal 0x6ed9eba1(%edi, %ebx, 1), %ebx; movl %ecx, %edi; roll $5, %edi; addl %edi, %ebx; movl %edx, %edi; xorl %esi, %edi; xorl %eax, %edi; addl %edi, %ebx; rorl $2, %edx;
 movl (16 -64)(%ebp), %edi; xorl (16 -56)(%ebp), %edi; xorl (16 -32)(%ebp), %edi; xorl (16 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 16(%ebp); leal 0x6ed9eba1(%edi, %eax, 1), %eax; movl %ebx, %edi; roll $5, %edi; addl %edi, %eax; movl %ecx, %edi; xorl %edx, %edi; xorl %esi, %edi; addl %edi, %eax; rorl $2, %ecx;

 addl $20,%ebp
 cmpl ((48+80*4)-20-(80*4))(%esp), %ebp
 jb 2b

3:
 movl (0 -64)(%ebp), %edi; xorl (0 -56)(%ebp), %edi; xorl (0 -32)(%ebp), %edi; xorl (0 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 0(%ebp); leal 0x8f1bbcdc(%edi, %esi, 1), %esi; movl %eax, %edi; roll $5, %edi; addl %edi, %esi; movl %ebx, %edi; xorl %ecx, %edi; xorl %ebx, %edx; andl %edx, %edi; xorl %ebx, %edi; xorl %ebx, %edx; addl %edi, %esi; rorl $2, %ebx;
 movl (4 -64)(%ebp), %edi; xorl (4 -56)(%ebp), %edi; xorl (4 -32)(%ebp), %edi; xorl (4 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 4(%ebp); leal 0x8f1bbcdc(%edi, %edx, 1), %edx; movl %esi, %edi; roll $5, %edi; addl %edi, %edx; movl %eax, %edi; xorl %ebx, %edi; xorl %eax, %ecx; andl %ecx, %edi; xorl %eax, %edi; xorl %eax, %ecx; addl %edi, %edx; rorl $2, %eax;
 movl (8 -64)(%ebp), %edi; xorl (8 -56)(%ebp), %edi; xorl (8 -32)(%ebp), %edi; xorl (8 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 8(%ebp); leal 0x8f1bbcdc(%edi, %ecx, 1), %ecx; movl %edx, %edi; roll $5, %edi; addl %edi, %ecx; movl %esi, %edi; xorl %eax, %edi; xorl %esi, %ebx; andl %ebx, %edi; xorl %esi, %edi; xorl %esi, %ebx; addl %edi, %ecx; rorl $2, %esi;
 movl (12 -64)(%ebp), %edi; xorl (12 -56)(%ebp), %edi; xorl (12 -32)(%ebp), %edi; xorl (12 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 12(%ebp); leal 0x8f1bbcdc(%edi, %ebx, 1), %ebx; movl %ecx, %edi; roll $5, %edi; addl %edi, %ebx; movl %edx, %edi; xorl %esi, %edi; xorl %edx, %eax; andl %eax, %edi; xorl %edx, %edi; xorl %edx, %eax; addl %edi, %ebx; rorl $2, %edx;
 movl (16 -64)(%ebp), %edi; xorl (16 -56)(%ebp), %edi; xorl (16 -32)(%ebp), %edi; xorl (16 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 16(%ebp); leal 0x8f1bbcdc(%edi, %eax, 1), %eax; movl %ebx, %edi; roll $5, %edi; addl %edi, %eax; movl %ecx, %edi; xorl %edx, %edi; xorl %ecx, %esi; andl %esi, %edi; xorl %ecx, %edi; xorl %ecx, %esi; addl %edi, %eax; rorl $2, %ecx;

 addl $20, %ebp
 cmpl ((48+80*4)-24-(80*4))(%esp), %ebp
 jb 3b

4:
 movl (0 -64)(%ebp), %edi; xorl (0 -56)(%ebp), %edi; xorl (0 -32)(%ebp), %edi; xorl (0 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 0(%ebp); leal 0xca62c1d6(%edi, %esi, 1), %esi; movl %eax, %edi; roll $5, %edi; addl %edi, %esi; movl %ebx, %edi; xorl %ecx, %edi; xorl %edx, %edi; addl %edi, %esi; rorl $2, %ebx;
 movl (4 -64)(%ebp), %edi; xorl (4 -56)(%ebp), %edi; xorl (4 -32)(%ebp), %edi; xorl (4 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 4(%ebp); leal 0xca62c1d6(%edi, %edx, 1), %edx; movl %esi, %edi; roll $5, %edi; addl %edi, %edx; movl %eax, %edi; xorl %ebx, %edi; xorl %ecx, %edi; addl %edi, %edx; rorl $2, %eax;
 movl (8 -64)(%ebp), %edi; xorl (8 -56)(%ebp), %edi; xorl (8 -32)(%ebp), %edi; xorl (8 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 8(%ebp); leal 0xca62c1d6(%edi, %ecx, 1), %ecx; movl %edx, %edi; roll $5, %edi; addl %edi, %ecx; movl %esi, %edi; xorl %eax, %edi; xorl %ebx, %edi; addl %edi, %ecx; rorl $2, %esi;
 movl (12 -64)(%ebp), %edi; xorl (12 -56)(%ebp), %edi; xorl (12 -32)(%ebp), %edi; xorl (12 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 12(%ebp); leal 0xca62c1d6(%edi, %ebx, 1), %ebx; movl %ecx, %edi; roll $5, %edi; addl %edi, %ebx; movl %edx, %edi; xorl %esi, %edi; xorl %eax, %edi; addl %edi, %ebx; rorl $2, %edx;
 movl (16 -64)(%ebp), %edi; xorl (16 -56)(%ebp), %edi; xorl (16 -32)(%ebp), %edi; xorl (16 -12)(%ebp), %edi; roll $1, %edi; movl %edi, 16(%ebp); leal 0xca62c1d6(%edi, %eax, 1), %eax; movl %ebx, %edi; roll $5, %edi; addl %edi, %eax; movl %ecx, %edi; xorl %edx, %edi; xorl %esi, %edi; addl %edi, %eax; rorl $2, %ecx;

 addl $20, %ebp
 cmpl ((48+80*4)-28-(80*4))(%esp), %ebp
 jb 4b

 movl ((48+80*4)+16)(%esp), %edi
 addl %eax, 0(%edi)
 addl %ebx, 4(%edi)
 addl %ecx, 8(%edi)
 addl %edx, 12(%edi)
 addl %esi, 16(%edi)

 movl ((48+80*4)-32-(80*4))(%esp), %edi
 cmpl %edi, ((48+80*4)+8)(%esp)
 jb 0b


 mov ((48+80*4)-36-(80*4))(%esp), %ebx
 mov ((48+80*4)-40-(80*4))(%esp), %esi
 mov ((48+80*4)-44-(80*4))(%esp), %edi
 addl $((48+80*4)), %esp
 popl %ebp
 ret
