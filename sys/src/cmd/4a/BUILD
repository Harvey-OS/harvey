mkdir -p ../../../../linux_amd64/bin
$PLAN9/bin/yacc -D1 -d a.y
9c -O0 -Wno-maybe-uninitialized -g -I . *.c
9l -o 4a *.o
cp 4a ../../../../linux_amd64/bin
