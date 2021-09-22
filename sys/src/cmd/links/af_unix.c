/* af_unix.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL
 */

#include "links.h"

#ifdef DONT_USE_AF_UNIX

int bind_to_af_unix(void)
{
	return -1;
}

void af_unix_close(void)
{
}

#else

#ifdef USE_AF_UNIX
#include <sys/un.h>
#endif

void af_unix_connection(void *);

struct sockaddr *s_unix_acc = NULL;
struct sockaddr *s_unix = NULL;
int s_unix_l;
int s_unix_fd = -1;

#ifdef USE_AF_UNIX

int get_address(void)
{
	struct sockaddr_un *su;
	unsigned char *path;
	if (!links_home) return -1;
	path = stracpy(links_home);
	if (!(su = mem_alloc(sizeof(struct sockaddr_un) + strlen((char *)path) + 1))) {
		mem_free(path);
		return -1;
	}
	if (!(s_unix_acc = mem_alloc(sizeof(struct sockaddr_un) + strlen((char *)path) + 1))) {
		mem_free(su);
		mem_free(path);
		return -1;
	}
	memset(su, 0, sizeof(struct sockaddr_un) + strlen((char *)path) + 1);
	su->sun_family = AF_UNIX;
	add_to_strn(&path, (unsigned char *)LINKS_SOCK_NAME);
	strcpy(su->sun_path, ( char *)path);
	mem_free(path);
	s_unix = (struct sockaddr *)su;
	s_unix_l = (char *)&su->sun_path - (char *)su + strlen(su->sun_path) + 1;
	return AF_UNIX;
}

void unlink_unix(void)
{
	if (unlink(((struct sockaddr_un *)s_unix)->sun_path)) {
		/*perror("unlink");
		debug("unlink: %s", ((struct sockaddr_un *)s_unix)->sun_path);*/
	}
}

#else

int get_address(void)
{
	struct sockaddr_in *sin;
	if (!(sin = mem_alloc(sizeof(struct sockaddr_in)))) return -1;
	if (!(s_unix_acc = mem_alloc(sizeof(struct sockaddr_in)))) {
		mem_free(sin);
		return -1;
	}
	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(LINKS_PORT);
	sin->sin_addr.s_addr = htonl(0x7f000001);
	s_unix = (struct sockaddr *)sin;
	s_unix_l = sizeof(struct sockaddr_in);
	return AF_INET;
}

void unlink_unix(void)
{
}

#endif

int bind_to_af_unix(void)
{
	int u = 0;
	int a1 = 1;
	int cnt = 0;
	int af;
	if ((af = get_address()) == -1) return -1;
	again:
	if ((s_unix_fd = socket(af, SOCK_STREAM, 0)) == -1) return -1;
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
	setsockopt(s_unix_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&a1, sizeof a1);
#endif
	if (bind(s_unix_fd, s_unix, s_unix_l)) {
		/*perror("");
		debug("bind: %d", errno);*/
		close(s_unix_fd);
		if ((s_unix_fd = socket(af, SOCK_STREAM, 0)) == -1) return -1;
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
		setsockopt(s_unix_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&a1, sizeof a1);
#endif
		if (connect(s_unix_fd, s_unix, s_unix_l)) {
			/*perror("");
			debug("connect: %d", errno);*/
			if (++cnt < MAX_BIND_TRIES) {
				struct timeval tv = { 0, 100000 };
				fd_set dummy;
				FD_ZERO(&dummy);
				select(0, &dummy, &dummy, &dummy, &tv);
				close(s_unix_fd);
				goto again;
			}
			close(s_unix_fd), s_unix_fd = -1;
			if (!u) {
				unlink_unix();
				u = 1;
				goto again;
			}
			mem_free(s_unix), s_unix = NULL;
			return -1;
		}
		mem_free(s_unix), s_unix = NULL;
		return s_unix_fd;
	}
	if (listen(s_unix_fd, 100)) {
		error((unsigned char *)"ERROR: listen failed: %d", errno);
		mem_free(s_unix), s_unix = NULL;
		close(s_unix_fd), s_unix_fd = -1;
		return -1;
	}
	set_handlers(s_unix_fd, af_unix_connection, NULL, NULL, NULL);
	return -1;
}

void af_unix_connection(void *xxx)
{
	int l = s_unix_l;
	int ns;
	memset(s_unix_acc, 0, l);
	ns = accept(s_unix_fd, (struct sockaddr *)s_unix_acc, &l);
	init_term(ns, ns, win_func);
	set_highpri();
}

void af_unix_close(void)
{
	if (s_unix_fd != -1) close(s_unix_fd);
	if (s_unix) unlink_unix(), mem_free(s_unix), s_unix = NULL;
	if (s_unix_acc) mem_free(s_unix_acc), s_unix_acc = NULL;
}

#endif
