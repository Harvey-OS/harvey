sh $NXM/buildconfig
rm *.o
(cd ../cc; $PLAN9/bin/yacc -D1 -d cc.y)
9c -g -I . -I ../cc ../cc/*.c *.c && 9l -o 6c *.o
cp 6c $NXM/xbin

