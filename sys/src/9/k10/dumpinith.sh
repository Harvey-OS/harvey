#!/home/elbing/source/plan9port/bin/rc

echo 'unsigned char initcode[]={' >> init.h
xd -1x init.out | sed -e 's/^[0-9a-f]+ //' -e 's/ ([0-9a-f][0-9a-f])/0x\1,/g' >> init.h
echo '};' >> init.h

