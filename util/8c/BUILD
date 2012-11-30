. $NXM/buildconfig

rm -f *.o

(cd ../cc; $PLAN9/bin/yacc -D1 -d cc.y)
9c -g -I . -I ../cc ../cc/*.c *.c && 9l -o 8c *.o
cp 8c $NXM/xbin

