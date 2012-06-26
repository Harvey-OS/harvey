. $NXM/buildconfig
#9c ../sys/src/cmd/auth/wrkey.c  readnvram.c ../sys/src/libauthsrv/nvcsum.c
#9l -o ../xbin/wrkey wrkey.o  readnvram.o nvcsum.o
go build mksys.go
cp mksys $NXM/xbin
9c ../sys/src/cmd/aux/data2s.c 
9l -o ../xbin/data2s data2s.o
9c ../sys/src/cmd/aux/mklatinkbd.c 
9l -o ../xbin/mklatinkbd mklatinkbd.o
9c pcc.c
9l -o ../xbin/pcc pcc.o
(cd cpp && mk install)
