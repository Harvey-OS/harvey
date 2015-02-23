set -e

CC=gcc 
CFLAGS='-O0 -static -fplan9-extensions -ffreestanding -fno-builtin -Wall -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wuninitialized -Wmaybe-uninitialized -I/amd64/include -I/sys/include -I .'

awk -f ../mk/parse -- -mksystab /sys/src/libc/9syscall/sys.h k8cpu > systab.c
$CC $CFLAGS -c systab.c

xxd -i init.out > init.h

gcc -O0 -static -fplan9-extensions -ffreestanding -fno-builtin -Wall -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wuninitialized -Wmaybe-uninitialized -I/amd64/include -I/sys/include -I . -c *.c
gcc -O0 -static -fplan9-extensions -ffreestanding -fno-builtin -Wall -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wuninitialized -Wmaybe-uninitialized -I/amd64/include -I/sys/include -I . -c ../386/*.c
gcc -O0 -static -fplan9-extensions -ffreestanding -fno-builtin -Wall -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wuninitialized -Wmaybe-uninitialized -I/amd64/include -I/sys/include -I . -c ../ip/*.c
gcc -O0 -static -fplan9-extensions -ffreestanding -fno-builtin -Wall -Wno-missing-braces -Wno-parentheses -Wno-unknown-pragmas -Wuninitialized -Wmaybe-uninitialized -I/amd64/include -I/sys/include -I . -c ../port/*.c
