none:VQ:
	echo usage: mk all, install, installall, cmd, cmd.install, lib, lib.install

all:V:
	mk lib.all
	mk cmd.all
	mk 9src.all

lib.%:V:
	cd lib
	mk $stem

lib.clean:V:
	cd lib
	rm -f .mk.$objtype
	mk clean

lib.nuke:V:
	cd lib
	rm -f .mk.$objtype
	mk nuke

cmd.%:V:
	cd cmd
	mk $stem

9src.%:V:
	cd 9src
	mk $stem

&:V:
	mk lib.$stem
	mk cmd.$stem
	mk 9src.$stem
