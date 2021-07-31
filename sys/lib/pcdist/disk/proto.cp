386	d775 sys sys
	9load	555 sys sys
	init	555 sys sys
	ld.com	555 sys sys
	mbr	555 sys sys
	pbs	555 sys sys
	pbslba	555 sys sys
	bin	d775 sys sys
		aux	d555 sys sys
			fcall	555 sys sys
			mouse	555 sys sys
			pcmcia	555 sys sys
			vga	555 sys sys
		disk	d775 sys sys
			fdisk	555 sys sys
			format	555 sys sys
			kfs	555 sys sys
			kfscmd	555 sys sys
			mbr	555 sys sys
#			mkext	555 sys sys
			prep	555 sys sys
		ip	d555 sys sys
			ipconfig	555 sys sys
			ppp	555 sys sys
		ndb	d555 sys sys
# csquery and dnsquery could go
			cs	555 sys sys
# 			csquery	555 sys sys
			dns	555 sys sys
# 			dnsquery	555 sys sys
		wrap	d775 sys sys
			inst	555 sys sys
		9660srv	555 sys sys
# acme could go
#		acme	555 sys sys
		archfs	555 sys sys
# authfs could go
# 		authfs	555 sys sys
		awk	555 sys sys
		bargraph	555 sys sys 
		basename	555 sys sys
		bind	555 sys sys
		cat	555 sys sys
		chgrp	555 sys sys
		chmod	555 sys sys
		cleanname	555 sys sys
		cmp	555 sys sys
		cdsh	555 sys sys 
		cp	555 sys sys
# cpu could go
#		cpu	555 sys sys
		date	555 sys sys
		dd	555 sys sys
		dossrv 555 sys sys
		echo	555 sys sys
		ed	555 sys sys
# if cpu goes, exportfs could go
# 		exportfs	555 sys sys
		ext2srv	555 sys sys 
		grep	555 sys sys
		gunzip	555 sys sys
		hget	555 sys sys
		hoc	555 sys sys
		ls	555 sys sys
		mc	555 sys sys
		mount	555 sys sys
		mv	555 sys sys
#		netkey	555 sys sys
		pci	555 sys sys 
		ps	555 sys sys
		rc	555 sys sys
		read	555 sys sys
		rio	555 sys sys
		rm	555 sys sys
		sed	555 sys sys
# snoopy could go
# 		snoopy	555 sys sys
		sort	555 sys sys
		srv	555 sys sys
		stats	555 sys sys
		syscall	555 sys sys
		tailfsrv	555 sys sys 
		tee	555 sys sys
#		telnet	555 sys sys
		test	555 sys sys
		wc	555 sys sys
		xd	555 sys sys
adm	d555 adm adm
	timezone	d555 sys sys
		local	555 sys sys
lib	d777 sys sys
	font	d555 sys sys
		bit	d555 sys sys
			lucidasans	d555 sys sys
				lstr.12	444 sys sys
				typelatin1.7.font	444 sys sys
#			lucm	d555 sys sys
#				latin1.9	444 sys sys
#				latin1.9.font	444 sys sys
	namespace	444 sys sys
	ndb	d555 sys sys
		common	444 sys sys 
		local	444 sys sys 
	vgadb	666 sys sys	
fd	d555 sys sys
mnt	d777 sys sys
	wsys	d000 sys sys
	arch	d000 sys sys
	wrap	d000 sys sys
n	d777 sys sys
	a:	d000 sys sys
	c:	d000 sys sys
	9fat	d000 sys sys
	kfs	d000 sys sys
	kremvax	d000 sys sys
	dist	d000 sys sys	
rc	d555 sys sys
	bin	d775 sys sys
		dist	d775 sys sys 
			+	- sys sys 
		9fat:	555 sys sys
		a:	555 sys sys	 
		c:	555 sys sys
		cpurc	555 sys sys 
		dosmnt	555 sys sys
		kill	555 sys sys
		lc	555 sys sys
		mkdir	555 sys sys 
		pwd	555 sys sys 
		ramfs	555 sys sys 
		sleep	555 sys sys 
		unmount	555 sys sys 
		window	555 sys sys
	lib	d555 sys sys
		rcmain	444 sys sys
sys	d555 sys sys
	log	d555 sys sys
		dns	444 sys sys	
		timesync	444 sys sys	
tmp	d555 sys sys
usr	d555 sys sys
	glenda	d775 glenda glenda
		+	- glenda glenda	
