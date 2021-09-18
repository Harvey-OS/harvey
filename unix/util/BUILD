. $NXM/buildconfig
9c wrkey.c
9l -o ../xbin/wrkey wrkey.o
go build mksys.go
cp mksys $NXM/xbin
9c ../sys/src/cmd/aux/data2s.c 
9l -o ../xbin/data2s data2s.o
9c ../sys/src/cmd/aux/mklatinkbd.c 
9l -o ../xbin/mklatinkbd mklatinkbd.o
9c pcc.c
9l -o ../xbin/pcc pcc.o
(cd cpp && mk clean)
(cd cpp && mk install)
