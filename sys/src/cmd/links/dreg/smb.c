#include "links.h"

struct smb_connection_info {
	int list;
	int cl;
	int ntext;
	char text[1];
};

void smb_got_data(struct connection *c);
void smb_got_text(struct connection *c);
void end_smb_connection(struct connection *c);

#define READ_SIZE	4096

void smb_func(struct connection *c)
{
	int po[2];
	int pe[2];
	unsigned char *host, *user, *pass, *port, *data, *share, *dir;
	unsigned char *p;
	int r;
	struct smb_connection_info *si;
	if (!(si = mem_alloc(sizeof(struct smb_connection_info) + 2))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	memset(si, 0, sizeof(struct smb_connection_info));
	c->info = si;
	host = get_host_name(c->url);
	if (!host) {
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	if (!(user = get_user_name(c->url))) user = stracpy((unsigned char *)"");
	if (!(pass = get_pass(c->url))) pass = stracpy((unsigned char *)"");
	if (!(port = get_port_str(c->url))) port = stracpy((unsigned char *)"");
	if (!(data = get_url_data(c->url))) data = stracpy((unsigned char *)"");
	if ((p = (unsigned char *)strchr(( char *)data, '/'))) share = memacpy(data, p - data), dir = p + 1;
	else if (*data) {
		struct cache_entry *e;
		if (get_cache_entry(c->url, &e)) {
			mem_free(host);
			mem_free(port);
			mem_free(user);
			mem_free(pass);
			setcstate(c, S_OUT_OF_MEM);
			abort_connection(c);
			return;
		}
		c->cache = e;
		if (c->cache->redirect) mem_free(c->cache->redirect);
		c->cache->redirect = stracpy(c->url);
		c->cache->redirect_get = 1;
		add_to_strn(&c->cache->redirect, (unsigned char *)"/");
		c->cache->incomplete = 0;
		mem_free(host);
		mem_free(port);
		mem_free(user);
		mem_free(pass);
		setcstate(c, S_OK);
		abort_connection(c);
		return;
	} else share = stracpy((unsigned char *)""), dir = (unsigned char *)"";
	if (!*share) si->list = 1;
	else if (!*dir || dir[strlen(( char *)dir) - 1] == '/' || dir[strlen(( char *)dir) - 1] == '\\') si->list = 2;
	if (c_pipe(po)) {
		mem_free(host);
		mem_free(port);
		mem_free(user);
		mem_free(pass);
		mem_free(share);
		setcstate(c, -errno);
		abort_connection(c);
		return;
	}
	if (c_pipe(pe)) {
		mem_free(host);
		mem_free(port);
		mem_free(user);
		mem_free(pass);
		mem_free(share);
		close(po[0]);
		close(po[1]);
		setcstate(c, -errno);
		abort_connection(c);
		return;
	}
	c->from = 0;
	r = fork();
	if (r == -1) {
		mem_free(host);
		mem_free(port);
		mem_free(user);
		mem_free(pass);
		mem_free(share);
		close(po[0]);
		close(po[1]);
		close(pe[0]);
		close(pe[1]);
		setcstate(c, -errno);
		retry_connection(c);
		return;
	}
	if (!r) {
		int n;
		unsigned char *v[32];
		close_fork_tty();
		close(1);
		dup2(po[1], 1);
		close(2);
		dup2(pe[1], 2);
		close(0);
		open("/dev/null", O_RDONLY);
		close(po[0]);
		close(pe[0]);
		n = 0;
		v[n++] = (unsigned char *)"smbclient";
		if (!*share) {
			v[n++] = (unsigned char *)"-L";
			v[n++] = host;
		} else {
			unsigned char *s = stracpy((unsigned char *)"//");
			add_to_strn(&s, host);
			add_to_strn(&s, (unsigned char *)"/");
			add_to_strn(&s, (unsigned char *)share);
			v[n++] = s;
			if (*pass && !*user) {
				v[n++] = pass;
			}
		}
		v[n++] = (unsigned char *)"-N";
		v[n++] = (unsigned char *)"-E";
		if (*port) {
			v[n++] = (unsigned char *)"-p";
			v[n++] = port;
		}
		if (*user) {
			v[n++] =(unsigned char *) "-U";
			if (!*pass) {
				v[n++] = user;
			} else {
				unsigned char *s = stracpy(user);
				add_to_strn(&s, (unsigned char *)"%");
				add_to_strn(&s, pass);
				v[n++] = pass;
			}
		}
		if (*share) {
			if (!*dir || dir[strlen(( char *)dir) - 1] == '/' || dir[strlen(( char *)dir) - 1] == '\\') {
				if (dir) {
					v[n++] = (unsigned char *)"-D";
					v[n++] = dir;
				}
				v[n++] = (unsigned char *)"-c";
				v[n++] = (unsigned char *)"ls";
			} else {
				unsigned char *ss;
				unsigned char *s = stracpy((unsigned char *)"get \"");
				add_to_strn(&s, dir);
				add_to_strn(&s, (unsigned char *)"\" -");
				while ((ss =  (unsigned char *)strchr(( char *)s, '/'))) *ss = '\\';
				v[n++] = (unsigned char *)"-c";
				v[n++] = s;
			}
		}
		v[n++] = NULL;
		execvp("smbclient", (char **)v);
		fprintf(stderr, "smbclient not found in $PATH");
		_exit(1);
	}
	c->pid = r;
	mem_free(host);
	mem_free(port);
	mem_free(user);
	mem_free(pass);
	mem_free(share);
	c->sock1 = po[0];
	c->sock2 = pe[0];
	close(po[1]);
	close(pe[1]);
	set_handlers(po[0], (void (*)(void *))smb_got_data, NULL, NULL, c);
	set_handlers(pe[0], (void (*)(void *))smb_got_text, NULL, NULL, c);
	setcstate(c, S_CONN);
}

void smb_read_text(struct connection *c, int sock)
{
	int r;
	struct smb_connection_info *si = c->info;
	si = mem_realloc(si, sizeof(struct smb_connection_info) + si->ntext + READ_SIZE + 2);
	if (!si) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	c->info = si;
	r = read(sock, si->text + si->ntext, READ_SIZE);
	if (r == -1) {
		setcstate(c, -errno);
		retry_connection(c);
		return;
	}
	if (r == 0) {
		if (!si->cl) {
			si->cl = 1;
			set_handlers(c->sock2, NULL, NULL, NULL, NULL);
			return;
		}
		end_smb_connection(c);
		return;
	}
	if (!c->from) setcstate(c, S_GETH);
	si->ntext += r;
}

void smb_got_data(struct connection *c)
{
	struct smb_connection_info *si = c->info;
	struct cache_entry *e;
	char buffer[READ_SIZE];
	int r;
	if (si->list) {
		smb_read_text(c, c->sock1);
		return;
	}
	r = read(c->sock1, buffer, READ_SIZE);
	if (r == -1) {
		setcstate(c, -errno);
		retry_connection(c);
		return;
	}
	if (r == 0) {
		if (!si->cl) {
			si->cl = 1;
			set_handlers(c->sock1, NULL, NULL, NULL, NULL);
			return;
		}
		end_smb_connection(c);
		return;
	}
	setcstate(c, S_TRANS);
	if (get_cache_entry(c->url, &e)) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	c->cache = e;
	c->received += r;
	if (add_fragment(c->cache, c->from, (unsigned char *)buffer, r) == 1) c->tries = 0;
	c->from += r;
}

void smb_got_text(struct connection *c)
{
	smb_read_text(c, c->sock2);
}

void end_smb_connection(struct connection *c)
{
	struct smb_connection_info *si = c->info;
	struct cache_entry *e;
	if (get_cache_entry(c->url, &e)) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	c->cache = e;
	if (!c->from) {
		if (si->ntext && si->text[si->ntext - 1] != '\n') si->text[si->ntext++] = '\n';
		si->text[si->ntext] = 0;
		if ((strstr(( char *)si->text, "NT_STATUS_FILE_IS_A_DIRECTORY") || strstr(( char *)si->text, "NT_STATUS_ACCESS_DENIED") || strstr(( char *)si->text, "ERRbadfile")) && *c->url && c->url[strlen(( char *)c->url) - 1] != '/' && c->url[strlen(( char *)c->url) - 1] != '\\') {
			if (c->cache->redirect) mem_free(c->cache->redirect);
			c->cache->redirect = stracpy(c->url);
			c->cache->redirect_get = 1;
			add_to_strn(&c->cache->redirect, (unsigned char *)"/");
			c->cache->incomplete = 0;
		} else {
			unsigned char *ls, *le, *le2;
			unsigned char *ud;
			unsigned char *t = init_str();
			int l = 0;
			int type = 0;
			int pos = 0;
			add_to_str(&t, &l, (unsigned char *)"<html><head><title>/");
			ud = stracpy(get_url_data(c->url));
			if (strchr(( char *)ud, POST_CHAR)) *strchr(( char *)ud, POST_CHAR) = 0;
			add_to_str(&t, &l, ud);
			mem_free(ud);
			add_to_str(&t, &l, (unsigned char *)"</title></head><body><pre>");
			ls = (unsigned char *)si->text;
			while ((le =  (unsigned char *)strchr(( char *)ls, '\n'))) {
				unsigned char *lx;
				le2 =  (unsigned char *)strchr(( char *)ls, '\r');
				if (!le2 || le2 > le) le2 = le;
				lx = memacpy(ls, le2 - ls);
				if (si->list == 1) {
					unsigned char *ll, *lll;
					if (!*lx) type = 0;
					if (strstr(( char *)lx, "Sharename") && strstr(( char *)lx, "Type")) {
						if (strstr(( char *)lx, "Type")) pos = (unsigned char *)strstr(( char *)lx, "Type") - lx;
						else pos = 0;
						type = 1;
						goto af;
					}
					if (strstr(( char *)lx, "Server") && strstr(( char *)lx, "Comment")) {
						type = 2;
						goto af;
					}
					if (strstr(( char *)lx, "Workgroup") && strstr(( char *)lx, "Master")) {
						pos = (unsigned char *)strstr(( char *)lx, "Master") - lx;
						type = 3;
						goto af;
					}
					if (!type) goto af;
					for (ll = lx; *ll; ll++) if (!WHITECHAR(*ll) && *ll != '-') goto np;
					goto af;
					np:
					for (ll = lx; *ll; ll++) if (!WHITECHAR(*ll)) break;
					for (lll = ll; *lll/* && lll[1]*/; lll++) if (WHITECHAR(*lll)/* && WHITECHAR(lll[1])*/) break;
					if (type == 1) {
						unsigned char *llll;
						if (!strstr(( char *)lll, "Disk")) goto af;
						if (pos && pos < strlen(( char *)lx) && WHITECHAR(*(llll = lx + pos - 1)) && llll > ll) {
							while (llll > ll && WHITECHAR(*llll)) llll--;
							if (!WHITECHAR(*llll)) lll = llll + 1;
						}
						add_bytes_to_str(&t, &l, lx, ll - lx);
						add_to_str(&t, &l, (unsigned char *)"<a href=\"");
						add_bytes_to_str(&t, &l, ll, lll - ll);
						add_to_str(&t, &l, (unsigned char *)"/\">");
						add_bytes_to_str(&t, &l, ll, lll - ll);
						add_to_str(&t, &l, (unsigned char *)"</a>");
						add_to_str(&t, &l, lll);
					} else if (type == 2) {
						sss:
						add_bytes_to_str(&t, &l, lx, ll - lx);
						add_to_str(&t, &l, (unsigned char *)"<a href=\"smb://");
						add_bytes_to_str(&t, &l, ll, lll - ll);
						add_to_str(&t, &l, (unsigned char *)"/\">");
						add_bytes_to_str(&t, &l, ll, lll - ll);
						add_to_str(&t, &l, (unsigned char *)"</a>");
						add_to_str(&t, &l, lll);
					} else if (type == 3) {
						if (pos < strlen(( char *)lx) && pos && WHITECHAR(lx[pos - 1]) && !WHITECHAR(lx[pos])) ll = lx + pos;
						else for (ll = lll; *ll; ll++) if (!WHITECHAR(*ll)) break;
						for (lll = ll; *lll; lll++) if (WHITECHAR(*lll)) break;
						goto sss;
					} else goto af;
				} else if (si->list == 2) {
					if (strstr(( char *)lx, "NT_STATUS")) {
						le[1] = 0;
						goto af;
					}
					if (le2 - ls >= 5 && ls[0] == ' ' && ls[1] == ' ' && ls[2] != ' ') {
						int dir;
						unsigned char *pp;
						unsigned char *p = ls + 3;
						while (p + 2 <= le2) {
							if (p[0] == ' ' && p[1] == ' ') goto o;
							p++;
						}
						goto af;
						o:
						dir = 0;
						pp = p;
						while (pp < le2 && *pp == ' ') pp++;
						while (pp < le2 && *pp != ' ') {
							if (*pp == 'D') {
								dir = 1;
								break;
							}
							pp++;
						}
						add_to_str(&t, &l, (unsigned char *)"  <a href=\"");
						add_bytes_to_str(&t, &l, ls + 2, p - (ls + 2));
						if (dir) add_chr_to_str(&t, &l, '/');
						add_to_str(&t, &l, (unsigned char *)"\">");
						add_bytes_to_str(&t, &l, ls + 2, p - (ls + 2));
						add_to_str(&t, &l, (unsigned char *)"</a>");
						add_bytes_to_str(&t, &l, p, le - p);
					} else goto af;
				} else af: add_bytes_to_str(&t, &l, ls, le2 - ls);
				add_chr_to_str(&t, &l, '\n');
				ls = le + 1;
				mem_free(lx);
			}
			/*add_to_str(&t, &l, si->text);*/
			add_fragment(c->cache, 0, t, l);
			c->from += l;
			truncate_entry(c->cache, l, 1);
			c->cache->incomplete = 0;
			mem_free(t);
			if (!c->cache->head) c->cache->head = stracpy((unsigned char *)"\r\n");
			add_to_strn(&c->cache->head, (unsigned char *)"Content-Type: text/html\r\n");
		}
	} else {
		truncate_entry(c->cache, c->from, 1);
		c->cache->incomplete = 0;
	}
	close_socket(&c->sock1);
	close_socket(&c->sock2);
	setcstate(c, S_OK);
	abort_connection(c);
	return;
}
