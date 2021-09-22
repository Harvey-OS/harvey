/* http.c
 * HTTP protocol client implementation
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct http_connection_info {
	int bl_flags;
	int http10;
	int close;
	int length;
	int version;
	int chunk_remaining;
};

/* Returns a string pointer with value of the item.
 * The string must be destroyed after usage with mem_free.
 */
unsigned char *parse_http_header(unsigned char *head, unsigned char *item, unsigned char **ptr)
{
	unsigned char *i, *f, *g, *h;
	if (!head) return NULL;
	h = NULL;
	for (f = head; *f; f++) {
		if (*f != 10) continue;
		f++;
		for (i = item; *i && *f; i++, f++)
			if (upcase(*i) != upcase(*f)) goto cont;
		if (!*f) break;
		if (f[0] == ':') {
			while (f[1] == ' ') f++;
			for (g = ++f; *g >= ' '; g++) ;
			while (g > f && g[-1] == ' ') g--;
			if (h) mem_free(h);
			if ((h = mem_alloc(g - f + 1))) {
				memcpy(h, f, g - f);
				h[g - f] = 0;
				if (ptr) {
					*ptr = f;
					break;
				}
				return h;
			}
		}
		cont:;
		f--;
	}
	return h;
}

unsigned char *parse_header_param(unsigned char *x, unsigned char *e)
{
	int le = strlen((char *)e);
	int lp;
	unsigned char *y = x;
	a:
	if (!(y = (unsigned char *)strchr((char *)y, ';'))) return NULL;
	while (*y && (*y == ';' || *y <= ' ')) y++;
	if (strlen((char *)y) < le) return NULL;
	if (casecmp(y, e, le)) goto a;
	y += le;
	while (*y && (*y <= ' ' || *y == '=')) y++;
	if (!*y) return stracpy((unsigned char *)"");
	lp = 0;
	while (y[lp] >= ' ' && y[lp] != ';') lp++;
	return memacpy(y, lp);
}

int get_http_code(unsigned char *head, int *code, int *version)
{
	if (!head) return -1;
	while (head[0] == ' ') head++;
	if (upcase(head[0]) != 'H' || upcase(head[1]) != 'T' || upcase(head[2]) != 'T' ||
	    upcase(head[3]) != 'P') return -1;
	if (head[4] == '/' && head[5] >= '0' && head[5] <= '9'
	 && head[6] == '.' && head[7] >= '0' && head[7] <= '9' && head[8] <= ' ') {
		if (version) *version = (head[5] - '0') * 10 + head[7] - '0';
	} else if (version) *version = 0;
	for (head += 4; *head > ' '; head++) ;
	if (*head++ != ' ') return -1;
	if (head[0] < '1' || head [0] > '9' || head[1] < '0' || head[1] > '9' ||
	    head[2] < '0' || head [2] > '9') return -1;
	if (code) *code = (head[0]-'0')*100 + (head[1]-'0')*10 + head[2]-'0';
	return 0;
}

unsigned char *buggy_servers[] = { (unsigned char *)"mod_czech/3.1.0", (unsigned char *)"Purveyor", (unsigned char *)"Netscape-Enterprise", (unsigned char *)"Apache Coyote", NULL };

int check_http_server_bugs(unsigned char *url, struct http_connection_info *info, unsigned char *head)
{
	unsigned char *server, **s;
	if (!http_bugs.allow_blacklist || info->http10) return 0;
	if (!(server = parse_http_header(head, (unsigned char *)"Server", NULL))) return 0;
	for (s = buggy_servers; *s; s++) if (strstr((char *)server, (char *)*s)) goto bug;
	mem_free(server);
	return 0;
	bug:
	mem_free(server);
	if ((server = get_host_name(url))) {
		add_blacklist_entry(server, BL_HTTP10);
		mem_free(server);
		return 1;
	}
	return 0;
}

void http_end_request(struct connection *c, int notrunc)
{
	if (c->state == S_OK) {
		if (c->cache) {
			if (!notrunc) truncate_entry(c->cache, c->from, 1);
			c->cache->incomplete = 0;
		}
	}
	if (c->info && !((struct http_connection_info *)c->info)->close
#ifdef HAVE_SSL
	&& (!c->ssl) /* We won't keep alive ssl connections */
#endif
	&& (!http_bugs.bug_post_no_keepalive || !strchr((char *)c->url, POST_CHAR))) {
		add_keepalive_socket(c, HTTP_KEEPALIVE_TIMEOUT);
	} else abort_connection(c);
}

void http_send_header(struct connection *);

/* connect via http over tcp */
void
http_func(struct connection *c)
{
	/* setcstate(c, S_CONN); */
	/* set_timeout(c); */
	if (get_keepalive_socket(c)) {
		int p;

		if ((p = get_port(c->url)) == -1) {
			setcstate(c, S_INTERNAL);
			abort_connection(c);
			return;
		}
		make_connection(c, p, &c->sock1, http_send_header);
	} else
		http_send_header(c);
}

void proxy_func(struct connection *c)
{
	http_func(c);
}

void http_get_header(struct connection *);

void http_send_header(struct connection *c)
{
	static unsigned char *accept_charset = NULL;
	struct http_connection_info *info;
	int http10 = http_bugs.http10;
	struct cache_entry *e = NULL;
	unsigned char *hdr;
	unsigned char *h, *u, *uu, *sp;
	int l = 0;
	unsigned char *post;
	unsigned char *host;

	find_in_cache(c->url, &c->cache);

	host = upcase(c->url[0]) != 'P' ? c->url : get_url_data(c->url);
	set_timeout(c);
	if (!(info = mem_alloc(sizeof(struct http_connection_info)))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	memset(info, 0, sizeof(struct http_connection_info));
	c->info = info;
	if ((h = get_host_name(host))) {
		info->bl_flags = get_blacklist_flags(h);
		mem_free(h);
	}
	if (info->bl_flags & BL_HTTP10) http10 = 1;
	info->http10 = http10;
	post = (unsigned char *)strchr((char *)c->url, POST_CHAR);
	if (post) post++;
	if (!(hdr = init_str())) {
		setcstate(c, S_OUT_OF_MEM);
		http_end_request(c, 0);
		return;
	}
	if (!post) add_to_str(&hdr, &l, (unsigned char *)"GET ");
	else {
		add_to_str(&hdr, &l, (unsigned char *)"POST ");
		c->unrestartable = 2;
	}
	if (upcase(c->url[0]) != 'P') add_to_str(&hdr, &l, (unsigned char *)"/");
	if (!(u = get_url_data(c->url))) {
		setcstate(c, S_BAD_URL);
		http_end_request(c, 0);
		return;
	}
	if (!post) uu = stracpy(u);
	else uu = memacpy(u, post - u - 1);
	a:
	for (sp = uu; *sp; sp++) if (*sp <= ' ') {
		unsigned char *nu = mem_alloc(strlen((char *)uu) + 3);
		if (!nu) break;
		memcpy(nu, uu, sp - uu);
		sprintf((char *)(nu + (sp - uu)), "%%%02X", (int)*sp);
		strcat((char *)nu, (char *)sp + 1);
		mem_free(uu);
		uu = nu;
		goto a;
	} else if (*sp == '\\') *sp = '/';
	add_to_str(&hdr, &l, uu);
	mem_free(uu);
	if (!http10) add_to_str(&hdr, &l, (unsigned char *)" HTTP/1.1\r\n");
	else add_to_str(&hdr, &l, (unsigned char *)" HTTP/1.0\r\n");
	if ((h = get_host_name(host))) {
		add_to_str(&hdr, &l, (unsigned char *)"Host: ");
		add_to_str(&hdr, &l, h);
		mem_free(h);
		if ((h = get_port_str(host))) {
			add_to_str(&hdr, &l, (unsigned char *)":");
			add_to_str(&hdr, &l, h);
			mem_free(h);
		}
		add_to_str(&hdr, &l, (unsigned char *)"\r\n");
	}
	add_to_str(&hdr, &l, (unsigned char *)"User-Agent: ");
	if (!(*fake_useragent)) {
		add_to_str(&hdr, &l, (unsigned char *)"Links ");
		add_to_str(&hdr, &l, system_name);
		if (!F && !list_empty(terminals)) {
			struct terminal *t = terminals.prev;
			add_to_str(&hdr, &l, (unsigned char *)"; ");
			add_num_to_str(&hdr, &l, t->x);
			add_to_str(&hdr, &l,(unsigned char *) "x");
			add_num_to_str(&hdr, &l, t->y);
		}
#ifdef G
		if (F && drv) {
			add_to_str(&hdr, &l, (unsigned char *)"; ");
			add_to_str(&hdr, &l, drv->name);
		}
#endif
		add_to_str(&hdr, &l, (unsigned char *)")\r\n");
	}
	else {
		add_to_str(&hdr, &l, fake_useragent);
		add_to_str(&hdr, &l, (unsigned char *)"\r\n");
	}
	switch (referer)
	{
		case REFERER_FAKE:
		add_to_str(&hdr, &l, (unsigned char *)"Referer: ");
		add_to_str(&hdr, &l, fake_referer);
		add_to_str(&hdr, &l, (unsigned char *)"\r\n");
		break;

		case REFERER_SAME_URL:
		add_to_str(&hdr, &l, (unsigned char *)"Referer: ");
		if (!post) add_to_str(&hdr, &l, c->url);
		else add_bytes_to_str(&hdr, &l, u, post - c->url - 1);
		add_to_str(&hdr, &l, (unsigned char *)"\r\n");
		break;

		case REFERER_REAL:
		{
			unsigned char *post2;
			if (!(c->prev_url))break;   /* no referrer */

			post2 = (unsigned char *)strchr((char *)c->prev_url, POST_CHAR);
			add_to_str(&hdr, &l, (unsigned char *)"Referer: ");
			if (!post2) add_to_str(&hdr, &l, c->prev_url);
			else add_bytes_to_str(&hdr, &l, c->prev_url, post2 - c->prev_url);
			add_to_str(&hdr, &l, (unsigned char *)"\r\n");
		}
		break;
	}

	add_to_str(&hdr, &l, (unsigned char *)"Accept: */*\r\n");
	if (!(accept_charset)) {
		int i;
		unsigned char *cs, *ac;
		int aclen = 0;
		ac = init_str();
		for (i = 0; (cs = get_cp_mime_name(i)); i++) {
			if (aclen) add_to_str(&ac, &aclen, (unsigned char *)", ");
			else add_to_str(&ac, &aclen, (unsigned char *)"Accept-Charset: ");
			add_to_str(&ac, &aclen, cs);
		}
		if (aclen) add_to_str(&ac, &aclen, (unsigned char *)"\r\n");
		if ((accept_charset = malloc(strlen((char *)ac) + 1))) strcpy((char *)accept_charset, (char *)ac);
		else accept_charset = (unsigned char *)"";
		mem_free(ac);
	}
	if (!(info->bl_flags & BL_NO_CHARSET) && !http_bugs.no_accept_charset) add_to_str(&hdr, &l, accept_charset);
	if (!http10) {
		if (upcase(c->url[0]) != 'P') add_to_str(&hdr, &l, (unsigned char *)"Connection: ");
		else add_to_str(&hdr, &l, (unsigned char *)"Proxy-Connection: ");
		if (!post || !http_bugs.bug_post_no_keepalive) add_to_str(&hdr, &l, (unsigned char *)"Keep-Alive\r\n");
		else add_to_str(&hdr, &l,(unsigned char *) "close\r\n");
	}
	if ((e = c->cache)) {
		if (!e->incomplete && e->head && c->no_cache <= NC_IF_MOD) {
			unsigned char *m;
			if (e->last_modified) m = stracpy(e->last_modified);
			else if ((m = parse_http_header(e->head, (unsigned char *)"Date", NULL))) ;
			else if ((m = parse_http_header(e->head, (unsigned char *)"Expires", NULL))) ;
			else goto skip;
			add_to_str(&hdr, &l, (unsigned char *)"If-Modified-Since: ");
			add_to_str(&hdr, &l, m);
			add_to_str(&hdr, &l, (unsigned char *)"\r\n");
			mem_free(m);
		}
		skip:;
	}
	if (c->no_cache >= NC_PR_NO_CACHE) add_to_str(&hdr, &l, (unsigned char *)"Pragma: no-cache\r\nCache-Control: no-cache\r\n");
	if (c->from && c->no_cache < NC_IF_MOD) {
		add_to_str(&hdr, &l, (unsigned char *)"Range: bytes=");
		add_num_to_str(&hdr, &l, c->from);
		add_to_str(&hdr, &l, (unsigned char *)"-\r\n");
	}
	if ((h = (unsigned char *)get_auth_string((char *)c->url))) {
		add_to_str(&hdr, &l, h);
		mem_free(h);
	}
	if (post) {
		unsigned char *pd = (unsigned char *)strchr((char *)post, '\n');
		if (pd) {
			add_to_str(&hdr, &l, (unsigned char *)"Content-Type: ");
			add_bytes_to_str(&hdr, &l, post, pd - post);
			add_to_str(&hdr, &l, (unsigned char *)"\r\n");
			post = pd + 1;
		}
		add_to_str(&hdr, &l,(unsigned char *) "Content-Length: ");
		add_num_to_str(&hdr, &l, strlen((char *)post) / 2);
		add_to_str(&hdr, &l, (unsigned char *)"\r\n");
	}
	send_cookies(&hdr, &l, host);
	add_to_str(&hdr, &l, (unsigned char *)"\r\n");
	if (post) {
		while (post[0] && post[1]) {
			int h1, h2;
			h1 = post[0] <= '9' ? post[0] - '0' : post[0] >= 'A' ? upcase(post[0]) - 'A' + 10 : 0;
			if (h1 < 0 || h1 >= 16) h1 = 0;
			h2 = post[1] <= '9' ? post[1] - '0' : post[1] >= 'A' ? upcase(post[1]) - 'A' + 10 : 0;
			if (h2 < 0 || h2 >= 16) h2 = 0;
			add_chr_to_str(&hdr, &l, h1 * 16 + h2);
			post += 2;
		}
	}
	if (0)
		fprintf(stderr,
	"http_send_header: calling write_to_socket(c, %d, hdr, %d, ...)\n",
			c->sock1, l);
	write_to_socket(c, c->sock1, hdr, l, http_get_header);
	mem_free(hdr);
	setcstate(c, S_SENT);
}

int is_line_in_buffer(struct read_buffer *rb)
{
	int l;
	for (l = 0; l < rb->len; l++) {
		if (rb->data[l] == 10) return l + 1;
		if (l < rb->len - 1 && rb->data[l] == 13 && rb->data[l + 1] == 10) return l + 2;
		if (l == rb->len - 1 && rb->data[l] == 13) return 0;
		if (rb->data[l] < ' ') return -1;
	}
	return 0;
}

void
read_http_data(Connection *c, Rdbuff *rb)
{
	struct http_connection_info *info = c->info;

	set_timeout(c);
	if (rb->close == 2) {
		setcstate(c, S_OK);
		http_end_request(c, 0);
		return;
	}
	if (info->length != -2) {
		int l = rb->len;

		if (info->length >= 0 && info->length < l)
			l = info->length;
		c->received += l;
		if (add_fragment(c->cache, c->from, rb->data, l) == 1)
			c->tries = 0;
		if (info->length >= 0)
			info->length -= l;
		c->from += l;
		kill_buffer_data(rb, l);
		if (!info->length && !rb->close) {
			setcstate(c, S_OK);
			http_end_request(c, 0);
			return;
		}
	} else {
next_chunk:
		if (info->chunk_remaining == -2) {
			int l;

			if ((l = is_line_in_buffer(rb))) {
				if (l == -1) {
					setcstate(c, S_HTTP_ERROR);
					abort_connection(c);
					return;
				}
				kill_buffer_data(rb, l);
				if (l <= 2) {
					setcstate(c, S_OK);
					http_end_request(c, 0);
					return;
				}
				goto next_chunk;
			}
		} else if (info->chunk_remaining == -1) {
			int l;

			if ((l = is_line_in_buffer(rb))) {
				uchar *de;
				int n = 0;	/* warning, go away */

				if (l != -1)
					n = strtol((char *)rb->data,
						(char **)(void *)&de, 16);
				if (l == -1 || de == rb->data) {
					setcstate(c, S_HTTP_ERROR);
					abort_connection(c);
					return;
				}
				kill_buffer_data(rb, l);
				if ((info->chunk_remaining = n) == 0)
					info->chunk_remaining = -2;
				goto next_chunk;
			}
		} else {
			int l = info->chunk_remaining;

			if (l > rb->len)
				l = rb->len;
			c->received += l;
			if (add_fragment(c->cache, c->from, rb->data, l) == 1)
				c->tries = 0;
			info->chunk_remaining -= l;
			c->from += l;
			kill_buffer_data(rb, l);
			if (!info->chunk_remaining && rb->len >= 1) {
				if (rb->data[0] == 10)
					kill_buffer_data(rb, 1);
				else {
					if (rb->data[0] != 13 ||
					    rb->len >= 2 && rb->data[1] != 10) {
						setcstate(c, S_HTTP_ERROR);
						abort_connection(c);
						return;
					}
					if (rb->len < 2)
						goto read_more;
					kill_buffer_data(rb, 2);
				}
				info->chunk_remaining = -1;
				goto next_chunk;
			}
		}
	}
read_more:
	rb->idmsg = "read_http_data";
	read_from_socket(c, c->sock1, rb, read_http_data);
	setcstate(c, S_TRANS);
}

int get_header(struct read_buffer *rb)
{
	int i;
	for (i = 0; i < rb->len; i++) {
		unsigned char a = rb->data[i];
		if (/*a < ' ' && a != 10 && a != 13*/!a) return -1;
		if (i < rb->len - 1 && a == 10 && rb->data[i + 1] == 10) return i + 2;
		if (i < rb->len - 3 && a == 13) {
			if (rb->data[i + 1] != 10) return -1;
			if (rb->data[i + 2] == 13) {
				if (rb->data[i + 3] != 10) return -1;
				return i + 4;
			}
		}
	}
	return 0;
}

void
http_got_header(Connection *c, Rdbuff *rb)
{
	int a, h, version, cf, state = c->state != S_PROC? S_GETH: S_PROC;
	uchar *head, *cookie, *ch, *d;
	uchar *host = upcase(c->url[0]) != 'P'? c->url: get_url_data(c->url);
	struct cache_entry *e;
	struct http_connection_info *info;

	set_timeout(c);
	info = c->info;
	if (rb->close == 2) {
		uchar * h;
		if (!c->tries && (h = get_host_name(host))) {
			if (info->bl_flags & BL_NO_CHARSET)
				del_blacklist_entry(h, BL_NO_CHARSET);
			else {
				add_blacklist_entry(h, BL_NO_CHARSET);
				c->tries = -1;
			}
			mem_free(h);
		}
		setcstate(c, S_CANT_READ);
		retry_connection(c);
		return;
	}
	rb->close = 0;
again:
	if ((a = get_header(rb)) == -1) {
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if (!a) {
		rb->idmsg = "http_got_header";
		read_from_socket(c, c->sock1, rb, http_got_header);
		setcstate(c, state);
		return;
	}
	if (get_http_code(rb->data, &h, &version) || h == 101) {
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if (!(head = mem_alloc(a + 1))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	memcpy(head, rb->data, a);
	head[a] = 0;
	if (check_http_server_bugs(host, c->info, head)) {
		mem_free(head);
		setcstate(c, S_RESTART);
		retry_connection(c);
		return;
	}
	ch = head;
	while ((cookie = parse_http_header(ch, (uchar * )"Set-Cookie", &ch))) {
		uchar *host = upcase(c->url[0]) != 'P'?
			c->url: get_url_data(c->url);

		set_cookie(NULL, host, cookie);
		mem_free(cookie);
	}
	if (h == 100) {
		mem_free(head);
		state = S_PROC;
		kill_buffer_data(rb, a);
		goto again;
	}
	if (h < 200) {
		mem_free(head);
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if (h == 304) {
		mem_free(head);
		setcstate(c, S_OK);
		http_end_request(c, 1);
		return;
	}
	if (get_cache_entry(c->url, &e)) {
		mem_free(head);
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	e->http_code = h;
	if (e->head)
		mem_free(e->head);
	e->head = head;
	if ((d = parse_http_header(head, (uchar * )"Expires", NULL))) {
		time_t t = parse_http_date((char *)d);

		if (t && e->expire_time != 1)
			e->expire_time = t;
		mem_free(d);
	}
	if ((d = parse_http_header(head, (uchar * ) "Pragma", NULL))) {
		if (!casecmp(d, (uchar * )"no-cache", 8))
			e->expire_time = 1;
		mem_free(d);
	}
	if ((d = parse_http_header(head, (uchar * )"Cache-Control", NULL))) {
		if (!casecmp(d, (uchar * )"no-cache", 8))
			e->expire_time = 1;
		if (!casecmp(d, (uchar * )"max-age=", 8)) {
			if (e->expire_time != 1)
				e->expire_time = time(NULL) + atoi((char *)d+8);
		}
		mem_free(d);
	}
#ifdef HAVE_SSL
	if (c->ssl) {
		int l = 0;

		if (e->ssl_info)
			mem_free(e->ssl_info);
		e->ssl_info = init_str();

		add_num_to_str(&e->ssl_info, &l,
			SSL_get_cipher_bits(c->ssl, NULL));
		add_to_str(&e->ssl_info, &l, (uchar *)"-bit ");
		add_to_str(&e->ssl_info, &l, SSL_get_cipher_version(c->ssl));
		add_to_str(&e->ssl_info, &l, (uchar *)" ");
		add_to_str(&e->ssl_info, &l,
			(uchar *)SSL_get_cipher_name(c->ssl));
	}
#endif
	if (h == 204) {
		setcstate(c, S_OK);
		http_end_request(c, 0);
		return;
	}
	if (e->redirect)
		mem_free(e->redirect), e->redirect = NULL;
	if (h == 301 || h == 302 || h == 303) {
		if (h == 302 && !e->expire_time)
			e->expire_time = 1;
		d = parse_http_header(e->head, (uchar * )"Location", NULL);
		if (d) {
			if (e->redirect)
				mem_free(e->redirect);
			e->redirect = d;
			e->redirect_get = h == 303;
		}
	}
	kill_buffer_data(rb, a);
	c->cache = e;
	info->close = 0;
	info->length = -1;
	info->version = version;
	if ((d = parse_http_header(e->head, (uchar *)"Connection", NULL)) ||
	    (d = parse_http_header(e->head, (uchar *)"Proxy-Connection", NULL))) {
		if (!strcasecmp((char *)d, "close"))
			info->close = 1;
		mem_free(d);
	} else if (version < 11)
		info->close = 1;
	cf = c->from;
	c->from = 0;
	if ((d = parse_http_header(e->head, (uchar *)"Content-Range", NULL))) {
		if (strlen((char *)d) > 6) {
			d[5] = 0;
			if (strcasecmp((char *)d, "bytes") == 0 &&
			    d[6] >= '0' && d[6] <= '9') {
				int f = strtol((char *)d + 6, NULL, 10);

				if (f >= 0)
					c->from = f;
			}
		}
		mem_free(d);
	}
	if (cf && !c->from && !c->unrestartable)
		c->unrestartable = 1;
	if (c->from > cf || c->from < 0) {
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if ((d = parse_http_header(e->head, (uchar *)"Content-Length", NULL))) {
		uchar *ep;
		int l = strtol((char *)d, (char **)(void *)&ep, 10);

		if (!*ep && l >= 0) {
			if (!info->close || version >= 11)
				info->length = l;
			c->est_length = c->from + l;
		}
		mem_free(d);
	}
	if ((d = parse_http_header(e->head, (uchar *)"Accept-Ranges", NULL))) {
		if (!strcasecmp((char *)d, "none") && !c->unrestartable)
			c->unrestartable = 1;
		mem_free(d);
	} else if (!c->unrestartable && !c->from)
		c->unrestartable = 1;
	d = parse_http_header(e->head, (uchar *)"Transfer-Encoding", NULL);
	if (d) {
		if (!strcasecmp((char *)d, "chunked")) {
			info->length = -2;
			info->chunk_remaining = -1;
		}
		mem_free(d);
	}
	if (!info->close && info->length == -1)
		info->close = 1;
	if ((d = parse_http_header(e->head, (uchar * )"Last-Modified", NULL))) {
		if (e->last_modified &&
		    strcasecmp((char *)e->last_modified, (char *)d)) {
			delete_entry_content(e);
			if (c->from) {
				c->from = 0;
				mem_free(d);
				setcstate(c, S_MODIFIED);
				retry_connection(c);
				return;
			}
		}
		if (!e->last_modified)
			e->last_modified = d;
		else
			mem_free(d);
	}
	if (!e->last_modified &&
	    (d = parse_http_header(e->head, (uchar * )"Date", NULL)))
		e->last_modified = d;
	if (info->length == -1 || (version < 11 && info->close))
		rb->close = 1;
	read_http_data(c, rb);
}

void
http_get_header(Connection *c)
{
	Rdbuff * rb;

	set_timeout(c);
	if (!(rb = alloc_read_buffer(c)))
		return;
	rb->close = 1;
	rb->idmsg = "http_get_header";
	read_from_socket(c, c->sock1, rb, http_got_header);
}
