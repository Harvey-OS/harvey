/* file.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#ifdef FS_UNIX_RIGHTS
void setrwx(int m, unsigned char *p)
{
	if(m & S_IRUSR) p[0] = 'r';
	if(m & S_IWUSR) p[1] = 'w';
	if(m & S_IXUSR) p[2] = 'x';
}

void setst(int m, unsigned char *p)
{
#ifdef S_ISUID
	if (m & S_ISUID) {
		p[2] = 'S';
		if (m & S_IXUSR) p[2] = 's';
	}
#endif
#ifdef S_ISGID
	if (m & S_ISGID) {
		p[5] = 'S';
		if (m & S_IXGRP) p[5] = 's';
	}
#endif
#ifdef S_ISVTX
	if (m & S_ISVTX) {
		p[8] = 'T';
		if (m & S_IXOTH) p[8] = 't';
	}
#endif
}
#endif

void stat_mode(unsigned char **p, int *l, struct stat *stp)
{
	unsigned char c = '?';
	if (stp) {
		if (0) ;
#ifdef S_ISBLK
		else if (S_ISBLK(stp->st_mode)) c = 'b';
#endif
#ifdef S_ISCHR
		else if (S_ISCHR(stp->st_mode)) c = 'c';
#endif
		else if (S_ISDIR(stp->st_mode)) c = 'd';
		else if (S_ISREG(stp->st_mode)) c = '-';
#ifdef S_ISFIFO
		else if (S_ISFIFO(stp->st_mode)) c = 'p';
#endif
#ifdef S_ISLNK
		else if (S_ISLNK(stp->st_mode)) c = 'l';
#endif
#ifdef S_ISSOCK
		else if (S_ISSOCK(stp->st_mode)) c = 's';
#endif
#ifdef S_ISNWK
		else if (S_ISNWK(stp->st_mode)) c = 'n';
#endif
	}
	add_chr_to_str(p, l, c);
#ifdef FS_UNIX_RIGHTS
	{
		unsigned char rwx[10] = "---------";
		if (stp) {
			int mode = stp->st_mode;
			setrwx(mode << 0, &rwx[0]);
			setrwx(mode << 3, &rwx[3]);
			setrwx(mode << 6, &rwx[6]);
			setst(mode, rwx);
		}
		add_to_str(p, l, rwx);
	}
#endif
	add_chr_to_str(p, l, ' ');
}


void stat_links(unsigned char **p, int *l, struct stat *stp)
{
#ifdef FS_UNIX_HARDLINKS
	unsigned char lnk[64];
	if (!stp) add_to_str(p, l, (unsigned char *)"    ");
	else {
		sprintf(( char *)lnk, "%3d ", (int)stp->st_nlink);
		add_to_str(p, l, lnk);
	}
#endif
}

int last_uid = -1;
char last_user[64];

int last_gid = -1;
char last_group[64];

void stat_user(unsigned char **p, int *l, struct stat *stp, int g)
{
#ifdef FS_UNIX_USERS
	struct passwd *pwd;
	struct group *grp;
	int id;
	unsigned char *pp;
	int i;
	if (!stp) {
		add_to_str(p, l, (unsigned char *)"         ");
		return;
	}
	id = !g ? stp->st_uid : stp->st_gid;
	pp = (unsigned char *)(!g ? last_user : last_group);
	if (!g && id == last_uid && last_uid != -1) goto a;
	if (g && id == last_gid && last_gid != -1) goto a;
	if (!g) {
		if (!(pwd = getpwuid(id)) || !pwd->pw_name) sprintf(( char *)pp, "%d", id);
		else sprintf(( char *)pp, "%.8s", pwd->pw_name);
		last_uid = id;
	} else {
		if (!(grp = getgrgid(id)) || !grp->gr_name) sprintf(( char *)pp, "%d", id);
		else sprintf(( char *)pp, "%.8s", grp->gr_name);
		last_gid = id;
	}
	a:
	add_to_str(p, l, pp);
	for (i = strlen(( char *)pp); i < 8; i++) add_chr_to_str(p, l, ' ');
	add_chr_to_str(p, l, ' ');
#endif
}

void stat_size(unsigned char **p, int *l, struct stat *stp)
{
	unsigned char size[64];
	if (!stp) add_to_str(p, l, (unsigned char *)"         ");
	else {
		sprintf(( char *)size, "%8ld ", (long)stp->st_size);
		add_to_str(p, l, size);
	}
}

void stat_date(unsigned char **p, int *l, struct stat *stp)
{
	time_t current_time = time(NULL);
	time_t when;
	struct tm *when_local;
	unsigned char *fmt;
	unsigned char str[13];
	int wr;
	if (!stp) {
		add_to_str(p, l, (unsigned char *)"             ");
		return;
	}
	when = stp->st_mtime;
	when_local = localtime(&when);
	if (current_time > when + 6L * 30L * 24L * 60L * 60L || 
	    current_time < when - 60L * 60L) fmt = (unsigned char *)"%b %e  %Y";
	else fmt = (unsigned char*)"%b %e %H:%M";
#ifdef HAVE_STRFTIME
	wr = strftime((char *)str, 13, (char *)fmt, when_local);
#else
	wr = 0;
#endif
	while (wr < 12) str[wr++] = ' ';
	str[12] = 0;
	add_to_str(p, l, str);
	add_chr_to_str(p, l, ' ');
}

char *get_filename(char *url)
{
	char *p, *m;
	for (p = url + 7; *p && *p != 1; p++) ;
	if (!(m = mem_alloc(p - url - 7 + 1))) return NULL;
	memcpy(m, url + 7, p - url - 7),
	m[p - url - 7] = 0;
	return m;
}

struct dirs {
	unsigned char *s;
	unsigned char *f;
};

int comp_de(struct dirs *d1, struct dirs *d2)
{
	if (d1->f[0] == '.' && d1->f[1] == '.' && !d1->f[2]) return -1;
	if (d2->f[0] == '.' && d2->f[1] == '.' && !d2->f[2]) return 1;
	if (d1->s[0] == 'd' && d2->s[0] != 'd') return -1;
	if (d1->s[0] != 'd' && d2->s[0] == 'd') return 1;
	return strcmp(( char *)d1->f, ( char *)d2->f);
}

#include <dirent.h>
void file_func(struct connection *c)
{
	struct cache_entry *e;
	unsigned char *file, *name, *head;
	int fl;
	DIR *d;
	int h, r;
	struct stat stt;
	if (anonymous) {
		setcstate(c, S_BAD_URL);
		abort_connection(c);
		return;
	}
	if (!(name = (unsigned char*)get_filename(( char *)c->url))) {
		setcstate(c, S_OUT_OF_MEM); abort_connection(c); return;
	}
	if ((h = open((char *)name, O_RDONLY | O_NOCTTY)) == -1) {
		int er = errno;
		if ((d = opendir((char *)name))) goto dir;
		mem_free(name);
		setcstate(c, -er); abort_connection(c); return;
	}
	set_bin(h);
	if (fstat(h, &stt)) {
		mem_free(name);
		close(h);
		setcstate(c, -errno); abort_connection(c); return;
	}
	if (S_ISDIR(stt.st_mode)) {
		struct dirs *dir;
		int dirl;
		int i;
		struct dirent *de;
		d = opendir((char *)name);
		close(h);
		dir:
		dir = DUMMY, dirl = 0;
		if (name[0] && !dir_sep(name[strlen((char *)name) - 1])) {
			if (get_cache_entry(c->url, &e)) {
				mem_free(name);
				closedir(d);
				setcstate(c, S_OUT_OF_MEM); abort_connection(c); return;
			}
			c->cache = e;
			if (e->redirect) mem_free(e->redirect);
			e->redirect = stracpy(c->url);
			e->redirect_get = 1;
			add_to_strn(&e->redirect, (unsigned char *)"/");
			mem_free(name);
			closedir(d);
			goto end;
		}
		last_uid = -1;
		last_gid = -1;
		file = init_str();
		fl = 0;
		add_to_str(&file, &fl, (unsigned char *)"<html><head><title>");
		add_to_str(&file, &fl, name);
		add_to_str(&file, &fl, (unsigned char *)"</title></head><body><h2>Directory ");
		add_to_str(&file, &fl, name);
		add_to_str(&file, &fl, (unsigned char *)"</h2><pre>");
		while ((de = readdir(d))) {
			struct stat stt, *stp;
			unsigned char **p;
			int l;
			struct dirs *nd;
			unsigned char *n;
			if (!strcmp(( char *)de->d_name, ".")) continue;
			if (!(nd = mem_realloc(dir, (dirl + 1) * sizeof(struct dirs))))
				continue;
			dir = nd;
			dir[dirl].f = stracpy((unsigned char *)de->d_name);
			*(p = &dir[dirl++].s) = init_str();
			l = 0;
			n = stracpy(name);
			add_to_strn(&n, (unsigned char *)de->d_name);
#ifdef FS_UNIX_SOFTLINKS
			if (lstat((char *)n, &stt))
#else
			if (stat((char *)n, &stt))
#endif
			     stp = NULL;
			else stp = &stt;
			mem_free(n);
			stat_mode(p, &l, stp);
			stat_links(p, &l, stp);
			stat_user(p, &l, stp, 0);
			stat_user(p, &l, stp, 1);
			stat_size(p, &l, stp);
			stat_date(p, &l, stp);
		}
		closedir(d);
		if (dirl) qsort(dir, dirl, sizeof(struct dirs), (int(*)(const void *, const void *))comp_de);
		for (i = 0; i < dirl; i++) {
			unsigned char *lnk = NULL;
#ifdef FS_UNIX_SOFTLINKS
			if (dir[i].s[0] == 'l') {
				unsigned char *buf = NULL;
				int size = 0;
				int r;
				unsigned char *n = stracpy(name);
				add_to_strn(&n, dir[i].f);
				do {
					if (buf) mem_free(buf);
					size += ALLOC_GR;
					if (!(buf = mem_alloc(size))) goto xxx;
					r = readlink((char *)n, (char *)buf, size);
				} while (r == size);
				if (r == -1) goto yyy;
				buf[r] = 0;
				lnk = buf;
				goto xxx;
				yyy:
				mem_free(buf);
				xxx:
				mem_free(n);
			}
#endif
			/*add_to_str(&file, &fl, "   ");*/
			add_to_str(&file, &fl, dir[i].s);
			add_to_str(&file, &fl, (unsigned char *)"<a href=\"");
			add_to_str(&file, &fl, dir[i].f);
			if (dir[i].s[0] == 'd') add_to_str(&file, &fl, (unsigned char *)"/");
			else if (lnk) {
				struct stat st;
				unsigned char *n = stracpy(name);
				add_to_strn(&n, dir[i].f);
				if (!stat(( char *)n, &st)) if (S_ISDIR(st.st_mode)) add_to_str(&file, &fl, (unsigned char *)"/");
				mem_free(n);
			}
			add_to_str(&file, &fl, (unsigned char *)"\">");
			/*if (dir[i].s[0] == 'd') add_to_str(&file, &fl, (unsigned char *)"<font color=\"yellow\">");*/
			add_to_str(&file, &fl, dir[i].f);
			/*if (dir[i].s[0] == 'd') add_to_str(&file, &fl, (unsigned char *)"</font>");*/
			add_to_str(&file, &fl, (unsigned char *)"</a>");
			if (lnk) {
				add_to_str(&file, &fl, (unsigned char *)" -> ");
				add_to_str(&file, &fl, lnk);
				mem_free(lnk);
			}
			add_to_str(&file, &fl, (unsigned char *)"\n");
		}
		mem_free(name);
		for (i = 0; i < dirl; i++) mem_free(dir[i].s), mem_free(dir[i].f);
		mem_free(dir);
		add_to_str(&file, &fl, (unsigned char *)"</pre></body></html>\n");
		head = stracpy((unsigned char *)"\r\nContent-Type: text/html\r\n");
	} else if (!S_ISREG(stt.st_mode)) {
		mem_free(name);
		close(h);
		setcstate(c, S_FILE_TYPE); abort_connection(c); return;
	} else {
		mem_free(name);
		/* + 1 is there because of bug in Linux. Read returns -EACCES when
		   reading 0 bytes to invalid address */
		if (!(file = mem_alloc(stt.st_size + 1))) {
			close(h);
			setcstate(c, S_OUT_OF_MEM); abort_connection(c); return;
		}
		if ((r = read(h, file, stt.st_size)) != stt.st_size) {
			mem_free(file); close(h);
			setcstate(c, r == -1 ? -errno : S_FILE_ERROR);
			abort_connection(c); return;
		}
		close(h);
		fl = stt.st_size;
		head = stracpy((unsigned char *)"");
	}
	if (get_cache_entry(c->url, &e)) {
		mem_free(file);
		setcstate(c, S_OUT_OF_MEM); abort_connection(c); return;
	}
	if (e->head) mem_free(e->head);
	e->head = head;
	c->cache = e;
	add_fragment(e, 0, file, fl);
	truncate_entry(e, fl, 1);
	mem_free(file);
	end:
	c->cache->incomplete = 0;
	setcstate(c, S_OK);
	abort_connection(c);
}
