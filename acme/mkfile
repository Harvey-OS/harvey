</$objtype/mkfile

none:VQ:
	echo mk all, install, clean, nuke, installall, update

all install clean nuke installall update:V:
	@{cd bin/source; mk $target}
	@{cd mail/src; mk $target}
	@{cd news/src; mk $target}
	@{cd wiki/src; mk $target}
