DIRS=kpathsea kpathutil dvipsk web2c local tpic makeindex

%:V:
	for(i in $DIRS) @{
		cd $i
		mk $MKFLAGS $target
	}

all:V:
