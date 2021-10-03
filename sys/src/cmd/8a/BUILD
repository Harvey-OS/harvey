mkdir -p ../../../../linux_amd64/bin
$PLAN9/bin/yacc -D1 -d a.y
9c -Wno-maybe-uninitialized -g -I . *.c
9l -o 8a *.o
cp 8a ../../../../linux_amd64/bin
