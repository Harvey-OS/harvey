all:V:
	9fs $fileserver >[2=1]
	cp /adm/*key* /n/$fileserver/adm >[2=1]
