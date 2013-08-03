ARCH=\
	bcm\
	kw\
	mtx\
	omap\
	pc\
	ppc\
	rb\
	teg2\

all:V:
	for(i in $ARCH)@{
		cd $i
		mk
	}
	# build pc boots last
	@{ cd pc; mk clean }
	@{ cd pcboot; mk }
pcboot:V:
	@{ cd pc; mk clean }
	@{ cd pcboot; mk }

installall install:V:
	for(i in $ARCH) @{
		cd $i
		mk $target
	}
	@{ cd pc; mk clean }
	@{ cd pcboot; mk install }
	@{ cd pc; mk clean }

clean:V:
	for(i in $ARCH pcboot) @{
		cd $i
		mk clean
	}
