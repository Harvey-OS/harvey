O=blocked namefiles names.1127 names.112 names.1121 names.1122 names.1123 names.1125 names.1125-ih names.1126 names.1127 names.113 names.113-ho names.1134 names.1135 names.1135-ih

all:V:
	9fs $fileserver
	for(i in $O)
		mk $i

%:	/n/$fileserver/mail/lib/%
	cp $prereq $target
