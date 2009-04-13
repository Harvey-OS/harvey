# 1 "<stdin>"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "<stdin>"
# 116 "<stdin>"
 .text

 .p2align 2,0x90

 .globl __md5block
 __md5block:






 pushl %ebp
 subl $(20), %esp
 movl %ebx, (20 -8)(%esp)
 movl %esi, (20 -12)(%esp)
 movl %edi, (20 -16)(%esp)

 movl (20 +8)(%esp), %eax
 addl (20 +12)(%esp), %eax
 movl %eax, (20 -4)(%esp)

 movl (20 +8)(%esp), %ebp

0:
 movl (20 +16)(%esp), %esi
 movl (%esi), %eax
 movl 4(%esi), %ebx
 movl 8(%esi), %ecx
 movl 12(%esi), %edx

 movl %ecx, %edi; xorl %edx, %edi; andl %ebx, %edi; xorl %edx, %edi; leal 0xd76aa478(%eax, %edi, 1), %eax; addl 0*4(%ebp), %eax; roll $7, %eax; addl %ebx, %eax;
 movl %ebx, %edi; xorl %ecx, %edi; andl %eax, %edi; xorl %ecx, %edi; leal 0xe8c7b756(%edx, %edi, 1), %edx; addl 1*4(%ebp), %edx; roll $12, %edx; addl %eax, %edx;
 movl %eax, %edi; xorl %ebx, %edi; andl %edx, %edi; xorl %ebx, %edi; leal 0x242070db(%ecx, %edi, 1), %ecx; addl 2*4(%ebp), %ecx; roll $17, %ecx; addl %edx, %ecx;
 movl %edx, %edi; xorl %eax, %edi; andl %ecx, %edi; xorl %eax, %edi; leal 0xc1bdceee(%ebx, %edi, 1), %ebx; addl 3*4(%ebp), %ebx; roll $22, %ebx; addl %ecx, %ebx;

 movl %ecx, %edi; xorl %edx, %edi; andl %ebx, %edi; xorl %edx, %edi; leal 0xf57c0faf(%eax, %edi, 1), %eax; addl 4*4(%ebp), %eax; roll $7, %eax; addl %ebx, %eax;
 movl %ebx, %edi; xorl %ecx, %edi; andl %eax, %edi; xorl %ecx, %edi; leal 0x4787c62a(%edx, %edi, 1), %edx; addl 5*4(%ebp), %edx; roll $12, %edx; addl %eax, %edx;
 movl %eax, %edi; xorl %ebx, %edi; andl %edx, %edi; xorl %ebx, %edi; leal 0xa8304613(%ecx, %edi, 1), %ecx; addl 6*4(%ebp), %ecx; roll $17, %ecx; addl %edx, %ecx;
 movl %edx, %edi; xorl %eax, %edi; andl %ecx, %edi; xorl %eax, %edi; leal 0xfd469501(%ebx, %edi, 1), %ebx; addl 7*4(%ebp), %ebx; roll $22, %ebx; addl %ecx, %ebx;

 movl %ecx, %edi; xorl %edx, %edi; andl %ebx, %edi; xorl %edx, %edi; leal 0x698098d8(%eax, %edi, 1), %eax; addl 8*4(%ebp), %eax; roll $7, %eax; addl %ebx, %eax;
 movl %ebx, %edi; xorl %ecx, %edi; andl %eax, %edi; xorl %ecx, %edi; leal 0x8b44f7af(%edx, %edi, 1), %edx; addl 9*4(%ebp), %edx; roll $12, %edx; addl %eax, %edx;
 movl %eax, %edi; xorl %ebx, %edi; andl %edx, %edi; xorl %ebx, %edi; leal 0xffff5bb1(%ecx, %edi, 1), %ecx; addl 10*4(%ebp), %ecx; roll $17, %ecx; addl %edx, %ecx;
 movl %edx, %edi; xorl %eax, %edi; andl %ecx, %edi; xorl %eax, %edi; leal 0x895cd7be(%ebx, %edi, 1), %ebx; addl 11*4(%ebp), %ebx; roll $22, %ebx; addl %ecx, %ebx;

 movl %ecx, %edi; xorl %edx, %edi; andl %ebx, %edi; xorl %edx, %edi; leal 0x6b901122(%eax, %edi, 1), %eax; addl 12*4(%ebp), %eax; roll $7, %eax; addl %ebx, %eax;
 movl %ebx, %edi; xorl %ecx, %edi; andl %eax, %edi; xorl %ecx, %edi; leal 0xfd987193(%edx, %edi, 1), %edx; addl 13*4(%ebp), %edx; roll $12, %edx; addl %eax, %edx;
 movl %eax, %edi; xorl %ebx, %edi; andl %edx, %edi; xorl %ebx, %edi; leal 0xa679438e(%ecx, %edi, 1), %ecx; addl 14*4(%ebp), %ecx; roll $17, %ecx; addl %edx, %ecx;
 movl %edx, %edi; xorl %eax, %edi; andl %ecx, %edi; xorl %eax, %edi; leal 0x49b40821(%ebx, %edi, 1), %ebx; addl 15*4(%ebp), %ebx; roll $22, %ebx; addl %ecx, %ebx;


 movl %ebx, %edi; xorl %ecx, %edi; andl %edx, %edi; xorl %ecx, %edi; leal 0xf61e2562(%eax, %edi, 1), %eax; addl (1*4)(%ebp), %eax; roll $5, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; andl %ecx, %edi; xorl %ebx, %edi; leal 0xc040b340(%edx, %edi, 1), %edx; addl (6*4)(%ebp), %edx; roll $9, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; andl %ebx, %edi; xorl %eax, %edi; leal 0x265e5a51(%ecx, %edi, 1), %ecx; addl (11*4)(%ebp), %ecx; roll $14, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; andl %eax, %edi; xorl %edx, %edi; leal 0xe9b6c7aa(%ebx, %edi, 1), %ebx; addl (0*4)(%ebp), %ebx; roll $20, %ebx; addl %ecx,%ebx;

 movl %ebx, %edi; xorl %ecx, %edi; andl %edx, %edi; xorl %ecx, %edi; leal 0xd62f105d(%eax, %edi, 1), %eax; addl (5*4)(%ebp), %eax; roll $5, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; andl %ecx, %edi; xorl %ebx, %edi; leal 0x02441453(%edx, %edi, 1), %edx; addl (10*4)(%ebp), %edx; roll $9, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; andl %ebx, %edi; xorl %eax, %edi; leal 0xd8a1e681(%ecx, %edi, 1), %ecx; addl (15*4)(%ebp), %ecx; roll $14, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; andl %eax, %edi; xorl %edx, %edi; leal 0xe7d3fbc8(%ebx, %edi, 1), %ebx; addl (4*4)(%ebp), %ebx; roll $20, %ebx; addl %ecx,%ebx;

 movl %ebx, %edi; xorl %ecx, %edi; andl %edx, %edi; xorl %ecx, %edi; leal 0x21e1cde6(%eax, %edi, 1), %eax; addl (9*4)(%ebp), %eax; roll $5, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; andl %ecx, %edi; xorl %ebx, %edi; leal 0xc33707d6(%edx, %edi, 1), %edx; addl (14*4)(%ebp), %edx; roll $9, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; andl %ebx, %edi; xorl %eax, %edi; leal 0xf4d50d87(%ecx, %edi, 1), %ecx; addl (3*4)(%ebp), %ecx; roll $14, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; andl %eax, %edi; xorl %edx, %edi; leal 0x455a14ed(%ebx, %edi, 1), %ebx; addl (8*4)(%ebp), %ebx; roll $20, %ebx; addl %ecx,%ebx;

 movl %ebx, %edi; xorl %ecx, %edi; andl %edx, %edi; xorl %ecx, %edi; leal 0xa9e3e905(%eax, %edi, 1), %eax; addl (13*4)(%ebp), %eax; roll $5, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; andl %ecx, %edi; xorl %ebx, %edi; leal 0xfcefa3f8(%edx, %edi, 1), %edx; addl (2*4)(%ebp), %edx; roll $9, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; andl %ebx, %edi; xorl %eax, %edi; leal 0x676f02d9(%ecx, %edi, 1), %ecx; addl (7*4)(%ebp), %ecx; roll $14, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; andl %eax, %edi; xorl %edx, %edi; leal 0x8d2a4c8a(%ebx, %edi, 1), %ebx; addl (12*4)(%ebp), %ebx; roll $20, %ebx; addl %ecx,%ebx;


 movl %ebx, %edi; xorl %ecx, %edi; xorl %edx, %edi; leal 0xfffa3942(%eax, %edi, 1), %eax; addl (5*4)(%ebp), %eax; roll $4, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; xorl %ecx, %edi; leal 0x8771f681(%edx, %edi, 1), %edx; addl (8*4)(%ebp), %edx; roll $11, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; xorl %ebx, %edi; leal 0x6d9d6122(%ecx, %edi, 1), %ecx; addl (11*4)(%ebp), %ecx; roll $16, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; xorl %eax, %edi; leal 0xfde5380c(%ebx, %edi, 1), %ebx; addl (14*4)(%ebp), %ebx; roll $23, %ebx; addl %ecx,%ebx;

 movl %ebx, %edi; xorl %ecx, %edi; xorl %edx, %edi; leal 0xa4beea44(%eax, %edi, 1), %eax; addl (1*4)(%ebp), %eax; roll $4, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; xorl %ecx, %edi; leal 0x4bdecfa9(%edx, %edi, 1), %edx; addl (4*4)(%ebp), %edx; roll $11, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; xorl %ebx, %edi; leal 0xf6bb4b60(%ecx, %edi, 1), %ecx; addl (7*4)(%ebp), %ecx; roll $16, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; xorl %eax, %edi; leal 0xbebfbc70(%ebx, %edi, 1), %ebx; addl (10*4)(%ebp), %ebx; roll $23, %ebx; addl %ecx,%ebx;

 movl %ebx, %edi; xorl %ecx, %edi; xorl %edx, %edi; leal 0x289b7ec6(%eax, %edi, 1), %eax; addl (13*4)(%ebp), %eax; roll $4, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; xorl %ecx, %edi; leal 0xeaa127fa(%edx, %edi, 1), %edx; addl (0*4)(%ebp), %edx; roll $11, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; xorl %ebx, %edi; leal 0xd4ef3085(%ecx, %edi, 1), %ecx; addl (3*4)(%ebp), %ecx; roll $16, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; xorl %eax, %edi; leal 0x04881d05(%ebx, %edi, 1), %ebx; addl (6*4)(%ebp), %ebx; roll $23, %ebx; addl %ecx,%ebx;

 movl %ebx, %edi; xorl %ecx, %edi; xorl %edx, %edi; leal 0xd9d4d039(%eax, %edi, 1), %eax; addl (9*4)(%ebp), %eax; roll $4, %eax; addl %ebx,%eax;
 movl %eax, %edi; xorl %ebx, %edi; xorl %ecx, %edi; leal 0xe6db99e5(%edx, %edi, 1), %edx; addl (12*4)(%ebp), %edx; roll $11, %edx; addl %eax,%edx;
 movl %edx, %edi; xorl %eax, %edi; xorl %ebx, %edi; leal 0x1fa27cf8(%ecx, %edi, 1), %ecx; addl (15*4)(%ebp), %ecx; roll $16, %ecx; addl %edx,%ecx;
 movl %ecx, %edi; xorl %edx, %edi; xorl %eax, %edi; leal 0xc4ac5665(%ebx, %edi, 1), %ebx; addl (2*4)(%ebp), %ebx; roll $23, %ebx; addl %ecx,%ebx;


 movl %edx, %edi; xorl $-1, %edi; orl %ebx, %edi; xorl %ecx, %edi; leal 0xf4292244(%eax, %edi, 1), %eax; addl (0*4)(%ebp), %eax; roll $6, %eax; addl %ebx,%eax;
 movl %ecx, %edi; xorl $-1, %edi; orl %eax, %edi; xorl %ebx, %edi; leal 0x432aff97(%edx, %edi, 1), %edx; addl (7*4)(%ebp), %edx; roll $10, %edx; addl %eax,%edx;
 movl %ebx, %edi; xorl $-1, %edi; orl %edx, %edi; xorl %eax, %edi; leal 0xab9423a7(%ecx, %edi, 1), %ecx; addl (14*4)(%ebp), %ecx; roll $15, %ecx; addl %edx,%ecx;
 movl %eax, %edi; xorl $-1, %edi; orl %ecx, %edi; xorl %edx, %edi; leal 0xfc93a039(%ebx, %edi, 1), %ebx; addl (5*4)(%ebp), %ebx; roll $21, %ebx; addl %ecx,%ebx;

 movl %edx, %edi; xorl $-1, %edi; orl %ebx, %edi; xorl %ecx, %edi; leal 0x655b59c3(%eax, %edi, 1), %eax; addl (12*4)(%ebp), %eax; roll $6, %eax; addl %ebx,%eax;
 movl %ecx, %edi; xorl $-1, %edi; orl %eax, %edi; xorl %ebx, %edi; leal 0x8f0ccc92(%edx, %edi, 1), %edx; addl (3*4)(%ebp), %edx; roll $10, %edx; addl %eax,%edx;
 movl %ebx, %edi; xorl $-1, %edi; orl %edx, %edi; xorl %eax, %edi; leal 0xffeff47d(%ecx, %edi, 1), %ecx; addl (10*4)(%ebp), %ecx; roll $15, %ecx; addl %edx,%ecx;
 movl %eax, %edi; xorl $-1, %edi; orl %ecx, %edi; xorl %edx, %edi; leal 0x85845dd1(%ebx, %edi, 1), %ebx; addl (1*4)(%ebp), %ebx; roll $21, %ebx; addl %ecx,%ebx;

 movl %edx, %edi; xorl $-1, %edi; orl %ebx, %edi; xorl %ecx, %edi; leal 0x6fa87e4f(%eax, %edi, 1), %eax; addl (8*4)(%ebp), %eax; roll $6, %eax; addl %ebx,%eax;
 movl %ecx, %edi; xorl $-1, %edi; orl %eax, %edi; xorl %ebx, %edi; leal 0xfe2ce6e0(%edx, %edi, 1), %edx; addl (15*4)(%ebp), %edx; roll $10, %edx; addl %eax,%edx;
 movl %ebx, %edi; xorl $-1, %edi; orl %edx, %edi; xorl %eax, %edi; leal 0xa3014314(%ecx, %edi, 1), %ecx; addl (6*4)(%ebp), %ecx; roll $15, %ecx; addl %edx,%ecx;
 movl %eax, %edi; xorl $-1, %edi; orl %ecx, %edi; xorl %edx, %edi; leal 0x4e0811a1(%ebx, %edi, 1), %ebx; addl (13*4)(%ebp), %ebx; roll $21, %ebx; addl %ecx,%ebx;

 movl %edx, %edi; xorl $-1, %edi; orl %ebx, %edi; xorl %ecx, %edi; leal 0xf7537e82(%eax, %edi, 1), %eax; addl (4*4)(%ebp), %eax; roll $6, %eax; addl %ebx,%eax;
 movl %ecx, %edi; xorl $-1, %edi; orl %eax, %edi; xorl %ebx, %edi; leal 0xbd3af235(%edx, %edi, 1), %edx; addl (11*4)(%ebp), %edx; roll $10, %edx; addl %eax,%edx;
 movl %ebx, %edi; xorl $-1, %edi; orl %edx, %edi; xorl %eax, %edi; leal 0x2ad7d2bb(%ecx, %edi, 1), %ecx; addl (2*4)(%ebp), %ecx; roll $15, %ecx; addl %edx,%ecx;
 movl %eax, %edi; xorl $-1, %edi; orl %ecx, %edi; xorl %edx, %edi; leal 0xeb86d391(%ebx, %edi, 1), %ebx; addl (9*4)(%ebp), %ebx; roll $21, %ebx; addl %ecx,%ebx;

 addl $(16*4), %ebp
 movl (20 +16)(%esp), %edi
 addl %eax,0(%edi)
 addl %ebx,4(%edi)
 addl %ecx,8(%edi)
 addl %edx,12(%edi)

 movl (20 -4)(%esp), %edi
 cmpl %edi, %ebp
 jb 0b


 movl (20 -8)(%esp), %ebx
 movl (20 -12)(%esp), %esi
 movl (20 -16)(%esp), %edi
 addl $(20), %esp
 popl %ebp
 ret
