	bin.init(0, OREAD);
	seq = 0;
	nf = 0;
	f = nil;
	while((s=bin.rdline('\n')) != nil){
		s[bin.linelen()-1] = 0;
		(n, tf) = findfile(s);
		if(n == 0)
			errors("no files match input", s);
		for(i=0; i<n; i++)
			tf[i].seq = seq++;
		f = realloc(f, (n+nf)*sizeof(*f));
		if(f == nil)
			rerror("out of memory");
		memmove(f+nf, tf, n*sizeof(f[0]));
		nf += n;
		free(tf);
	}

	/* sort by file name */
	qsort(f, nf, sizeof f[0], ncmp);

	/* convert to character positions if necessary*/
	for(i=0; i<nf; i++){
		tf = &f[i];
		tf->ok = 1;
		/* see if it's easy */
		s = tf->addr;
		if(*s=='#'){
			s++;
			n = 0;
			while(*s && '0'<=*s && *s<='9'){
				n = n*10+(*s-'0');
				s++;
			}
			tf->q0 = n;
			if(*s == 0){
				tf->q1 = n;
				continue;
			}
			if(*s != ',')
				goto Hard;
			s++;
			n = 0;
			while(*s && '0'<=*s && *s<='9'){
				n = n*10+(*s-'0');
				s++;
			}
			tf->q1 = n;
			if(*s == 0)
				continue;
		}
	Hard:
		id = tf->id;
		sprint(buf, "/mnt/8½/%d/addr", id);
		afd = open(buf, ORDWR);
		if(afd < 0)
			rerror(buf);
		sprint(buf, "/mnt/8½/%d/ctl", id);
		cfd = open(buf, ORDWR);
		if(cfd < 0)
			rerror(buf);
		if(write(cfd, "addr=dot\n", 9) != 9)
			rerror("setting address to dot");
		if(write(afd, tf->addr, strlen(tf->addr)) != strlen(tf->addr)){
			fprint(2, "%s: %s:%s is invalid address\n", prog, tf->name, tf->addr);
			tf->ok = 0;
			close(afd);
			close(cfd);
			continue;
		}
		seek(afd, 0, 0);
		if(read(afd, buf, sizeof buf) != 2*12)
			rerror("reading address");
		close(afd);
		close(cfd);
		tf->q0 = atoi(buf);
		tf->q1 = atoi(buf+12);
	}
