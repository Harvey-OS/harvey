/* connect.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

/* #define LOG_TRANSFER	"/tmp/log" */

#ifdef LOG_TRANSFER
void log_data(uchar *data, int len)
{
	static int hlaseno = 0;
	int fd;
	if (!hlaseno) {
		printf("\n\e[1mWARNING -- LOGGING NETWORK TRANSFERS !!!\e[0m%c\n", 7);
		fflush(stdout);
		sleep(1);
		hlaseno = 1;
	}
	if ((fd = open(LOG_TRANSFER, O_WRONLY | O_APPEND | O_CREAT, 0622)) != -1) {
		set_bin(fd);
		write(fd, data, len);
		close(fd);
	}
}

#else
#define log_data(x, y)
#endif

static void
exception(Connection *c)
{
	if (0)
		fprintf(stderr, "exception: for fd %d\n", c->sock1);
	setcstate(c, S_EXCEPT);
	retry_connection(c);
}

void
close_socket(int *s)
{
	if (*s >= 0) {
		close(*s);
		set_handlers(*s, NULL, NULL, NULL, NULL);
		*s = -1;
	}
}

void connected(Connection *);

struct conn_info {
	void	(*func)(Connection *);
	struct sockaddr_in sa;
	ip__address addr;
	int	port;
	int	*sock;
};

void dns_found(Connection *, int);

void
make_connection(Connection *c, int port, int *sock, void (*func)(Connection *))
{
	int as;
	uchar *host;
	struct conn_info *b;

	if (!(host = get_host_name(c->url))) {
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	if (!(b = mem_alloc(sizeof(struct conn_info )))) {
		mem_free(host);
		setcstate(c, S_OUT_OF_MEM);
		retry_connection(c);
		return;
	}
	b->func = func;
	b->sock = sock;
	b->port = port;
	c->buffer = b;
	log_data("\nCONNECTION: ", 13);
	log_data(host, strlen(host));
	log_data("\n", 1);
	if (c->no_cache >= NC_RELOAD)
		as = find_host_no_cache(host, &b->addr, &c->dnsquery,
			(void (*)(void *, int))dns_found, c);
	else
		as = find_host(host, &b->addr, &c->dnsquery,
			(void (*)(void *, int))dns_found, c);
	mem_free(host);
	if (as)
		setcstate(c, S_DNS);
}

#define EALREADY 114

int
get_pasv_socket(Connection *c, int cc, int *sock, uchar *port)
{
	struct sockaddr_in sa, sb;
	int s, len = sizeof sa;

	memset(&sa, 0, sizeof sa);
	memset(&sb, 0, sizeof sb);
	if (getsockname(cc, (struct sockaddr *)&sa, &len)) {
e:
		setcstate(c, -errno);
		retry_connection(c);
		return -2;
	}
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		goto e;
	*sock = s;
	fcntl(s, F_SETFL, O_NONBLOCK);	/* danger, will robinson! */
	memcpy(&sb, &sa, sizeof sb);
	sb.sin_port = 0;
	len = sizeof sa;
	if (bind(s, (struct sockaddr *)&sb, sizeof sb) ||
	    getsockname(s, (struct sockaddr *)&sa, &len) ||
	    listen(s, 1))
		goto e;
	memcpy(port, &sa.sin_addr.s_addr, 4);
	memcpy(port + 4, &sa.sin_port, 2);
#if defined(IP_TOS) && defined(IPTOS_THROUGHPUT)
	 {
		int on = IPTOS_THROUGHPUT;

		setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&on, sizeof on);
	}
#endif
	return 0;
}

int
sslconnect(Connection *c)
{
#ifdef HAVE_SSL
	int err;
	SSL *ssl = c->ssl;

	if (c->no_tsl)
		ssl->options |= SSL_OP_NO_TLSv1;
	err = SSL_get_error(ssl, SSL_connect(ssl));
	switch (err) {
	case SSL_ERROR_NONE:
	case SSL_ERROR_WANT_READ:
		break;
	default:
		c->no_tsl++;
		setcstate(c, S_SSL_ERROR);
		retry_connection(c);
		break;
	}
	return err;
#endif
}

void
ssl_want_read(Connection *c)
{
#ifdef HAVE_SSL
	struct conn_info *b = c->buffer;

	switch (sslconnect(c)) {
	case SSL_ERROR_NONE:
		c->buffer = NULL;
		(*b->func)(c);
		mem_free(b);
		break;
	}
#endif
}

int
pushssl(Connection *c, int s)
{
#ifdef HAVE_SSL
	c->ssl = getSSL();
	SSL_set_fd(c->ssl, s);
	switch (sslconnect(c)) {
	case SSL_ERROR_NONE:
		return 0;
	case SSL_ERROR_WANT_READ:
		setcstate(c, S_SSL_NEG);
		set_handlers(s, (void(*)(void *))ssl_want_read,
			NULL, (void(*)(void *))exception, c);
		return -1;
	default:
		return -1;
	}
#endif
}

/* got IP for name now, so dial it */
void
dns_found(Connection *c, int state)
{
	int s;
	struct conn_info *b = c->buffer;

	if (state) {
		setcstate(c, S_NO_DNS);
		abort_connection(c);
		return;
	}
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		setcstate(c, -errno);
		retry_connection(c);
		return;
	}
	*b->sock = s;
	fcntl(s, F_SETFL, O_NONBLOCK);	/* danger, will robinson! */
	memset(&b->sa, 0, sizeof b->sa);
	b->sa.sin_family = PF_INET;
	b->sa.sin_addr.s_addr = b->addr;
	b->sa.sin_port = htons(b->port);
	if (connect(s, (struct sockaddr *)&b->sa, sizeof b->sa)) {
		if (errno != EALREADY && errno != EINPROGRESS) {
			setcstate(c, -errno);
			retry_connection(c);
			return;
		}
		set_handlers(s, NULL, (void(*)(void *))connected,
			(void(*)(void *))exception, c);
		setcstate(c, S_CONN);
	} else {
#ifdef HAVE_SSL
		if(c->ssl && pushssl(c, s) < 0)
			return;
#endif
		c->buffer = NULL;
		(*b->func)(c);
		mem_free(b);
	}
}

void
connected(Connection *c)
{
	int err = 0, len = sizeof err;
	struct conn_info *b = c->buffer;
	void (*done)(Connection *) = b->func;

	if (getsockopt(*b->sock, SOL_SOCKET, SO_ERROR, (void *)&err, &len))
		if (!(err = errno)) {
			err = -(S_STATE);
			goto bla;
		}
	if (err >= 10000)
		err -= 10000;	/* Why does EMX return so large values? */
bla:
	if (err > 0)
		setcstate(c, -err), retry_connection(c);
	else {
#ifdef HAVE_SSL
		if(c->ssl && pushssl(c, *b->sock) < 0)
			return;
#endif
		c->buffer = NULL;
		mem_free(b);
		(*done)(c);
	}
}

typedef struct write_buffer Wrbuff;
struct write_buffer {
	int	sock;
	int	len;
	int	pos;
	void	(*done)(Connection *);
	uchar	data[1];
};

/*
 * called when we can write on c;
 * call (*c->buffer->done)() when done.
 */
static void
rdywrite(Connection *c)
{
	int wr;
	Wrbuff *wb = c->buffer;

	if (wb == NULL) {
		internal((uchar *)"write socket has no buffer");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
/*
	printf("ws: %d\n",wb->len-wb->pos);
	for (wr = wb->pos; wr < wb->len; wr++)
		printf("%c", wb->data[wr]);
	printf("-\n");
 */

#ifdef HAVE_SSL
	if(c->ssl) {
		wr = SSL_write(c->ssl, wb->data + wb->pos, wb->len - wb->pos);
		if (wr <= 0) {
			int err = SSL_get_error(c->ssl, wr);

			if (err != SSL_ERROR_WANT_WRITE) {
				setcstate(c, wr? (err == SSL_ERROR_SYSCALL?
					-errno: S_SSL_ERROR): S_CANT_WRITE);
				if (!wr || err == SSL_ERROR_SYSCALL)
					retry_connection(c);
				else
					abort_connection(c);
			}
		}
	} else
#endif
	{
		wr = write(wb->sock, wb->data + wb->pos, wb->len - wb->pos);
		if (wr <= 0) {
			setcstate(c, wr? -errno: S_CANT_WRITE);
			retry_connection(c);
		}
	}
	if (wr <= 0)
		return;

	/*printf("wr: %d\n", wr);*/
	wb->pos += wr;
	if (wb->pos >= wb->len) {
		void (*done)(Connection *) = wb->done;

		c->buffer = NULL;
		set_handlers(wb->sock, NULL, NULL, NULL, NULL);
		mem_free(wb);
		(*done)(c);
	}
}

/*
 * schedule output via rdywrite when select reports fd available.
 * upon completion, (*done)() will be called.
 */
void
write_to_socket(Connection *c, int s, uchar *data, int len,
	void (*done)(Connection *))
{
	Wrbuff *wb;

	log_data(data, len);
	if (!(wb = mem_alloc(sizeof(Wrbuff) + len))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	wb->sock = s;
	wb->pos = 0;
	wb->len = len;
	memcpy(wb->data, data, len);
	wb->done = done;

	if (c->buffer)
		mem_free(c->buffer);
	c->buffer = wb;
	/*
	 * instead of using select for output streams (which only tells you
	 * that there's room for 001 byte), we're going to
	 * rely on network buffering and network speed.
	 */
	rdywrite(c);
}

#define READ_SIZE 16384

int
sslread(Connection *c)
{
#ifdef HAVE_SSL
	int rd, err;
	Rdbuff *rb = c->buffer;
	SSL *ssl = c->ssl;

	if (ssl == NULL) {
		abort_connection(c);
		return;
	}

	rd = SSL_read(ssl, rb->data + rb->len, READ_SIZE);
	if (rd > 0)
		return rd;

	/* cope with errors of various sorts */
	err = SSL_get_error(ssl, rd);
	if (err == SSL_ERROR_WANT_READ) {
		rb->idmsg = "rdyread (sslread)";
		read_from_socket(c, rb->sock, rb, rb->done);
		return -1;
	}
	if (rd < 0 && errno == EBADF) {
		SSL_close(ssl);
		c->ssl = NULL;
	}
	if (rb->close && rd == 0) {
		rb->close = 2;
		(*rb->done)(c, rb);
		return -1;
	}
	setcstate(c, rd? (err == SSL_ERROR_SYSCALL? -errno: S_SSL_ERROR):
		S_CANT_READ);
	/* mem_free(rb); */
	if (rd == 0 || err == SSL_ERROR_SYSCALL)
		retry_connection(c);
	else
		abort_connection(c);
	return -1;
#endif
}

/*
 * this will block, so only call when select says there is data.
 * upon completion, calls (c->buffer->done)().
 */
static void
rdyread(Connection *c)
{
	int rd;
	Rdbuff *rb = c->buffer;

	if (rb == NULL) {
		internal((uchar *)"read socket has no buffer");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}

	/* `interrupt disable' */
	set_handlers(rb->sock, NULL, NULL, NULL, NULL);

	/* just suck the whole response into core(!) */
	rb = mem_realloc(rb, sizeof(Rdbuff) + rb->len + READ_SIZE);
	if (rb == NULL) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	c->buffer = rb;

#ifdef HAVE_SSL
	if(c->ssl) {
		if (sslread(c) < 0)
			return;
	} else
#endif
	{
		if (rb->sock < 0) {
			abort_connection(c);
			return;
		}
		rd = read(rb->sock, rb->data + rb->len, READ_SIZE);
		if (0)
			fprintf(stderr, "%d = read(%d, ...), errno = %d\n",
				rd, rb->sock, errno);
		if (rd <= 0) {
			if (rd < 0 && errno == EBADF) {
				set_handlers(rb->sock, NULL, NULL, NULL, NULL);
				close(rb->sock);
				rb->sock = -1;
			}
			if (rb->close && rd == 0) {
				rb->close = 2;
				(*rb->done)(c, rb);
				return;
			}
			setcstate(c, rd? -errno: S_CANT_READ);
			/* mem_free(rb); */
			retry_connection(c);
			return;
		}
	}

	/* successful read: update variables & call completion routine */
	log_data(rb->data + rb->len, rd);
	rb->len += rd;
	(*rb->done)(c, rb);
}

Rdbuff *
alloc_read_buffer(Connection *c)
{
	Rdbuff *rb;

	if (!(rb = mem_alloc(sizeof(Rdbuff) + READ_SIZE))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return NULL;
	}
	memset(rb, 0, sizeof *rb);
	return rb;
}

void
read_from_socket(Connection *c, int s, Rdbuff *rb,
	void (*done)(Connection *, Rdbuff *))
{
	struct stat stbuf;

	rb->done = done;
	rb->sock = s;

	if (c->buffer && rb != c->buffer)
		mem_free(c->buffer);
	c->buffer = rb;

	if (0 && rb->idmsg) {
		fprintf(stderr, "read_from_socket(%d [%s])\n", s, rb->idmsg);
		/* rb->idmsg = NULL; */
	}
	if (s < 0 || fstat(s, &stbuf) < 0 && errno == EBADF) {
		if (s >= 0) {
			if (0)
				fprintf(stderr, "read_from_socket: bad fd %d\n",
					s);
			set_handlers(s, NULL, NULL, NULL, NULL);
		}
		exception(c);
		return;
	}
	set_handlers(s, (void (*)())rdyread, NULL, (void (*)())exception, c);
}

void
kill_buffer_data(Rdbuff *rb, int n)
{
	if (n > rb->len || n < 0) {
		internal((uchar *)"called kill_buffer_data with bad value");
		rb->len = 0;
		return;
	}
	memmove(rb->data, rb->data + n, rb->len - n);
	rb->len -= n;
}
