/*
 * links.h
 * (c) 2002 Mikulas Patocka, Karel 'Clock' Kulhavy, Petr 'Brain' Kulhavy,
 *          Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

/*
 * WARNING: this file MUST be C++ compatible. this means:
 *	no implicit conversions from void *:
 *		BAD: uchar *c = mem_alloc(4);
 *		GOOD: uchar *c = (uchar *)mem_alloc(4);
 *	no implicit char * -> uchar * conversions:
 *		BAD: uchar *c = stracpy("A");
 *		GOOD: uchar *c = stracpy((uchar *)"A");
 *	no implicit uchar * -> char * conversions:
 *		BAD: uchar *x, *y, *z; z = strcpy(x, y);
 *		BAD: l = strlen(x);
 *		GOOD: uchar *x, *y; z = (uchar *)strcpy((char *)x, (char *)y);
 *		GOOD: l = strlen((char *)x);
 *	don't use C++ keywords (like template)
 *	if there is struct X, you cannot use variable X or typedef X
 *		(this applies to typedef ip as well -- don't use it!)
 *
 *	IF YOU WRITE ANYTHING NEW TO THIS FILE, try compiling this file in c++
 *		to make sure that you didn't break anything:
 *			g++ -DHAVE_CONFIG_H -x c++ links.h
 */
#ifndef _LINKS_H
#define _LINKS_H

#define _BSD_EXTENSION
#define _POSIX_SOURCE

#define uchar	unsigned char
#define ushort	unsigned short
#define ulong	unsigned long

#define LINKS_COPYRIGHT "(C) "
#define LINKS_COPYRIGHT_8859_1 "©"
#define LINKS_COPYRIGHT_8859_2 "(C)l"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

#define VERSION_STRING 	" 2.14 "
#define	ECONNRESET	104 

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif

#include "cfg.h"
#include "os_dep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <bsd.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <termios.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#ifndef DONT_INCLUDE_SETJMP
#include <setjmp.h>
#endif

#ifdef HAVE_SYS_CYGWIN_H
#include <sys/cygwin.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#else
#ifdef HAVE_NETINET_IN_SYSTEM_H
#include <netinet/in_system.h>
#endif
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#include <openssl/rand.h>
#endif

#if defined(G) 
#if defined(HAVE_PNG_H)
#define PNG_THREAD_UNSAFE_OK
#include <png.h>
#elif defined(HAVE_LIBPNG_PNG_H)
#define PNG_THREAD_UNSAFE_OK
#include <libpng/png.h>
#endif		/* #if defined(HAVE_PNG_H) */
#endif		/* #if defined(G) */

#ifdef HAVE_LONG_LONG
#define longlong long long
#else
#define longlong long
#endif

#include "os_depx.h"
#include "setup.h"

#define DUMMY ((void *)-1L)

#ifndef HAVE_SNPRINTF

int my_snprintf(char *, int n, char *format, ...);
#define snprintf my_snprintf

#endif

#define option	option_NOCLASH_with_include_on_cygwin
#define table	table_NOCLASH_with_libraries_on_macos
#define scroll	scroll_NOCLASH_with_libraries_on_macos
#define list	list_NOCLASH_in_stl_with_class_list

#ifndef G
#define F 0
#else
extern int F;
#endif

#if defined(DEBUG)
#if defined(G)
#define NO_GFX	do {if (F) internal((uchar *) "call to text-only function");} while (0)
#define NO_TXT	do {if (!F) internal((uchar *) "call to graphics-only function");} while (0)
#else
#define NO_GFX	do {} while (0)
#define NO_TXT	this_should_not_be_compiled
#endif
#else
#define NO_GFX	do {} while (0)
#define NO_TXT	do {} while (0)
#endif

#ifndef G
#define gf_val(x, y) (x)
#define GF(x)
#else
#define gf_val(x, y) (F ? (y) : (x))
#define GF(x) if (F) {x;}
#endif

#define BIN_SEARCH(table, entries, eq, ab, key, result)			\
{									\
	int _s = 0, _e = (entries) - 1;					\
	while (_s <= _e || !((result) = -1)) {				\
		int _m = (_s + _e) / 2;					\
		if (eq(((table)[_m]), (key))) {				\
			(result) = _m;					\
			break;						\
		}							\
		if (ab(((table)[_m]), (key))) _e = _m - 1;		\
		else _s = _m + 1;					\
	}								\
}									\

/* error.c */

void do_not_optimize_here(void *p);
void check_memory_leaks(void);
void error(uchar *, ...);
void debug_msg(uchar *, ...);
void int_error(uchar *, ...);
extern int errline;
extern uchar *errfile;

#define internal errfile = (uchar *)__FILE__, errline = __LINE__, int_error
#define debug errfile = (uchar *)__FILE__, errline = __LINE__, debug_msg

/* inline */

#ifdef HAVE_CALLOC
#define x_calloc(x) calloc((x), 1)
#else
static inline void *x_calloc(size_t x)
{
	void *p;
	if ((p = malloc(x))) memset(p, 0, x);
	return p;
}
#endif

#ifdef LEAK_DEBUG

extern long mem_amount;
extern long last_mem_amount;

#define ALLOC_MAGIC		0xa110c
#define ALLOC_FREE_MAGIC	0xf4ee
#define ALLOC_REALLOC_MAGIC	0x4ea110c

#ifndef LEAK_DEBUG_LIST
struct alloc_header {
	int magic;
	int size;
};
#else
struct alloc_header {
	struct alloc_header *next;
	struct alloc_header *prev;
	int magic;
	int size;
	int line;
	uchar *file;
	uchar *comment;
};
#endif

#define L_D_S ((sizeof(struct alloc_header) + 15) & ~15)

#endif

#ifdef LEAK_DEBUG

void *debug_mem_alloc(uchar *, int, size_t);
void *debug_mem_calloc(uchar *, int, size_t);
void debug_mem_free(uchar *, int, void *);
void *debug_mem_realloc(uchar *, int, void *, size_t);
void set_mem_comment(void *, uchar *, int);
uchar *get_mem_comment(void *);

#define mem_alloc(x) debug_mem_alloc((uchar *)__FILE__, __LINE__, x)
#define mem_calloc(x) debug_mem_calloc((uchar *)__FILE__, __LINE__, x)
#define mem_free(x) debug_mem_free((uchar *)__FILE__, __LINE__, x)
#define mem_realloc(x, y) debug_mem_realloc((uchar *)__FILE__, __LINE__, x, y)

#else

static inline void *mem_alloc(size_t size)
{
	void *p;
	if (!size) return DUMMY;
	if (!(p = malloc(size))) {
		error((uchar *)"ERROR: out of memory (malloc returned NULL)\n");
		return NULL;
	}
	return p;
}

static inline void *mem_calloc(size_t size)
{
	void *p;
	if (!size) return DUMMY;
	if (!(p = x_calloc(size))) {
		error((uchar *)"ERROR: out of memory (calloc returned NULL)\n");
		return NULL;
	}
	return p;
}

static inline void mem_free(void *p)
{
	if (p == DUMMY) return;
	if (!p) {
		internal((uchar *)"mem_free(NULL)");
		return;
	}
	free(p);
}

static inline void *mem_realloc(void *p, size_t size)
{
	if (p == DUMMY) return mem_alloc(size);
	if (!p) {
		internal((uchar *)"mem_realloc(NULL, %d)", size);
		return NULL;
	}
	if (!size) {
		mem_free(p);
		return DUMMY;
	}
	if (!(p = realloc(p, size))) {
		error((uchar *)"ERROR: out of memory (realloc returned NULL)\n");
		return NULL;
	}
	return p;
}

static inline void *debug_mem_alloc(uchar *f, int l, size_t s) { return mem_alloc(s); }
static inline void *debug_mem_calloc(uchar *f, int l, size_t s) { return mem_calloc(s); }
static inline void debug_mem_free(uchar *f, int l, void *p) { mem_free(p); }
static inline void *debug_mem_realloc(uchar *f, int l, void *p, size_t s) { return mem_realloc(p, s); }
static inline void set_mem_comment(void *p, uchar *c, int l) {}
static inline uchar *get_mem_comment(void *p){return (uchar *)"";}
#endif

static inline uchar upcase(uchar a)
{
	if (a>='a' && a<='z') a -= 0x20;
	return a;
}

static inline int xstrcmp(uchar *s1, uchar *s2)
{
        if (!s1 && !s2) return 0;
        if (!s1) return -1;
        if (!s2) return 1;
        return strcmp((char *)s1, (char *)s2);
}

static inline int cmpbeg(uchar *str, uchar *b)
{
	while (*str && upcase(*str) == upcase(*b)) str++, b++;
	return !!*b;
}

#if !(defined(LEAK_DEBUG) && defined(LEAK_DEBUG_LIST))

static inline uchar *memacpy(const uchar *src, int len)
{
	uchar *m;
	if ((m = (uchar *)mem_alloc(len + 1))) {
		memcpy(m, src, len);
		m[len] = 0;
	}
	return m;
}

static inline uchar *stracpy(const uchar *src)
{
	return src ? memacpy(src, src != DUMMY ? strlen((char *)src) : 0) : NULL;
}

#else

static inline uchar *debug_memacpy(uchar *f, int l, uchar *src, int len)
{
	uchar *m;
	if ((m = (uchar *)debug_mem_alloc(f, l, len + 1))) {
		memcpy(m, src, len);
		m[len] = 0;
	}
	return m;
}

#define memacpy(s, l) debug_memacpy((uchar *)__FILE__, __LINE__, s, l)

static inline uchar *debug_stracpy(uchar *f, int l, uchar *src)
{
	return src ? (uchar *)debug_memacpy(f, l, src, src != DUMMY ? strlen((char *)src) : 0) : NULL;
}

#define stracpy(s) debug_stracpy((uchar *)__FILE__, __LINE__, s)

#endif

#if !defined(HAVE_SIGSETJMP) || !defined(HAVE_SETJMP_H)
#ifdef OOPS
#undef OOPS
#endif
#endif

#ifndef OOPS
#define pr(code) if (1) {code;} else
static inline void nopr(void) {}
static inline void xpr(void) {}
#else
sigjmp_buf *new_stack_frame(void);
void xpr(void);
#define pr(code) if (!sigsetjmp(*new_stack_frame(), 1)) {do {code;} while (0); xpr();} else
void nopr(void);
#endif

static inline int snprint(uchar *s, int n, unsigned num)
{
	unsigned q = 1;
	while (q <= num / 10) q *= 10;
	while (n-- > 1 && q) *(s++) = num / q + '0', num %= q, q /= 10;
	*s = 0;
	return !!q;
}

static inline int snzprint(uchar *s, int n, int num)
{
	if (n > 1 && num < 0) *(s++) = '-', num = -num, n--;
	return snprint(s, n, num);
}

static inline void add_to_strn(uchar **s, uchar *a)
{
	uchar *p;
	if (!(p = (uchar *)mem_realloc(*s, strlen((char *)*s) + strlen((char *)a) + 1))) return;
	strcat((char *)p, (char *)a);
	*s = p;
}

#define ALLOC_GR	0x100		/* must be power of 2 */

#define init_str() init_str_x((uchar *)__FILE__, __LINE__)

static inline uchar *init_str_x(uchar *file, int line)
{
	uchar *p;
	
	p=(uchar *)debug_mem_alloc(file, line, 1);
       	*p = 0;
	return p;
}

static inline void add_to_str(uchar **s, int *l, uchar *a)
{
	uchar *p=*s;
	unsigned old_length;
	unsigned new_length;
	unsigned x;

	old_length=*l;
	new_length=strlen((char *)a)+old_length;
	*l=new_length;
	x=old_length^new_length;
	if (x>=old_length){
		/* Need to realloc */
		new_length|=(new_length>>1);
		new_length|=(new_length>>2);
		new_length|=(new_length>>4);
		new_length|=(new_length>>8);
		new_length|=(new_length>>16);
		p=(uchar *)mem_realloc(p,new_length+1);
	}
	*s=p;
	strcpy((char *)(p+old_length),(char *)a);
}

static inline void add_bytes_to_str(uchar **s, int *l, uchar *a, int ll)
{
	uchar *p=*s;
	unsigned old_length;
	unsigned new_length;
	unsigned x;

	old_length=*l;
	new_length=old_length+ll;
	*l=new_length;
	x=old_length^new_length;
	if (x>=old_length){
		/* Need to realloc */
		new_length|=(new_length>>1);
		new_length|=(new_length>>2);
		new_length|=(new_length>>4);
		new_length|=(new_length>>8);
		new_length|=(new_length>>16);
		p=(uchar *)mem_realloc(p,new_length+1);
	}
	*s=p;
	memcpy(p+old_length,a,ll);
	p[*l]=0;
}

static inline void add_chr_to_str(uchar **s, int *l, uchar a)
{
	uchar *p=*s;
	unsigned old_length;
	unsigned new_length;
	unsigned x;

	old_length=*l;
	new_length=old_length+1;
	*l=new_length;
	x=old_length^new_length;
	if (x>=old_length){
		p=(uchar *)mem_realloc(p,new_length<<1);
	}
	*s=p;
	p[old_length]=a;
	p[new_length]=0;
}

static inline void add_num_to_str(uchar **s, int *l, int n)
{
	uchar a[64];
	/*sprintf(a, "%d", n);*/
	snzprint(a, 64, n);
	add_to_str(s, l, a);
}

static inline void add_knum_to_str(uchar **s, int *l, int n)
{
	uchar a[13];
	if (n && n / (1024 * 1024) * (1024 * 1024) == n) snzprint(a, 12, n / (1024 * 1024)), a[strlen((char *)a) + 1] = 0, a[strlen((char *)a)] = 'M';
	else if (n && n / 1024 * 1024 == n) snzprint(a, 12, n / 1024), a[strlen((char *)a) + 1] = 0, a[strlen((char *)a)] = 'k';
	else snzprint(a, 13, n);
	add_to_str(s, l, a);
}

static inline long strtolx(uchar *c, uchar **end)
{
	long l = strtol((char *)c, (char **)end, 10);
	if (!*end) return l;
	if (upcase(**end) == 'K') {
		(*end)++;
		if (l < -MAXINT / 1024) return -MAXINT;
		if (l > MAXINT / 1024) return MAXINT;
		return l * 1024;
	}
	if (upcase(**end) == 'M') {
		(*end)++;
		if (l < -MAXINT / (1024 * 1024)) return -MAXINT;
		if (l > MAXINT / (1024 * 1024)) return MAXINT;
		return l * (1024 * 1024);
	}
	return l;
}

static inline uchar *copy_string(uchar **dst, uchar *src)
{
	if ((*dst = src) && (*dst = (uchar *)mem_alloc(strlen((char *)src) + 1))) strcpy((char *)*dst, (char *)src);
	return *dst;
}

/* Copies at most dst_size chars into dst. Ensures null termination of dst. */
static inline uchar *safe_strncpy(uchar *dst, const uchar *src, size_t dst_size)
{
	strncpy((char *)dst, (char *)src, dst_size);
	if (strlen((char *)src) >= dst_size) dst[dst_size - 1] = 0;
	return dst;
}



/* deletes all nonprintable characters from string */
static inline void skip_nonprintable(uchar *txt)
{
	uchar *txt1=txt;

	if (!txt||!*txt)return;
	for (;*txt;txt++)
		switch(*txt)
		{
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			break;

			case 9:
			*txt1=' ';
			txt1++;
			break;

			default:
			*txt1=*txt;
			txt1++;
			break;
		}
	*txt1=0;
}


struct list_head {
	void *next;
	void *prev;
};

struct xlist_head {
	struct xlist_head *next;
	struct xlist_head *prev;
};

#define init_list(x) { do_not_optimize_here(&x); (x).next=&(x); (x).prev=&(x); do_not_optimize_here(&x);}
#define list_empty(x) ((x).next == &(x))
#define del_from_list(x) {do_not_optimize_here(x); ((struct list_head *)(x)->next)->prev=(x)->prev; ((struct list_head *)(x)->prev)->next=(x)->next; do_not_optimize_here(x);}
/*#define add_to_list(l,x) {(x)->next=(l).next; (x)->prev=(typeof(x))&(l); (l).next=(x); if ((l).prev==&(l)) (l).prev=(x);}*/

#ifdef HAVE_TYPEOF
#define add_at_pos(p,x) do {do_not_optimize_here(p); (x)->next=(p)->next; (x)->prev=(p); (p)->next=(x); (x)->next->prev=(x);do_not_optimize_here(p);} while(0)
#define add_to_list(l,x) add_at_pos((typeof(x))&(l),(x))
#define foreach(e,l) for ((e)=(l).next; (e)!=(typeof(e))&(l); (e)=(e)->next)
#define foreachback(e,l) for ((e)=(l).prev; (e)!=(typeof(e))&(l); (e)=(e)->prev)
#else
#define add_at_pos(p,x) do {do_not_optimize_here(p); (x)->next=(p)->next; (x)->prev=(void *)(p); (p)->next=(x); (x)->next->prev=(x); do_not_optimize_here(p); } while(0)
#define add_to_list(l,x) add_at_pos(&(l),(x))
#define foreach(e,l) for ((e)=(l).next; (e)!=(void *)&(l); (e)=(e)->next)
#define foreachback(e,l) for ((e)=(l).prev; (e)!=(void *)&(l); (e)=(e)->prev)
#endif
#define free_list(l) {do_not_optimize_here(&l); while ((l).next != &(l)) {struct list_head *a__=(l).next; del_from_list(a__); mem_free(a__); } do_not_optimize_here(&l);}

#define WHITECHAR(x) ((x) == 9 || (x) == 10 || (x) == 12 || (x) == 13 || (x) == ' ')
#define U(x) ((x) == '"' || (x) == '\'')

static inline int isA(uchar c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		(c >= '0' && c <= '9') ||  c == '_' || c == '-';
}

/* case insensitive compare of 2 strings */
/* comparison ends after len (or less) characters */
/* return value: 1=strings differ, 0=strings are same */
static inline int casecmp(uchar *c1, uchar *c2, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (upcase(c1[i]) != upcase(c2[i]))
			return 1;
	return 0;
}


static inline int srch_cmp(uchar c1, uchar c2)
{
	return upcase(c1) != upcase(c2);
}

static inline int casestrstr(uchar *h, uchar *n)
{
	uchar *p;

	for (p = h; *p; p++) {
		if (!srch_cmp(*p,*n));  /* same */
		{
			uchar *q, *r;

			for (q=n, r=p; *r && *q; )
				if (!srch_cmp(*q,*r))
					r++,q++;    /* same */
				else
					break;
			if (!*q)
				return 1;
		}
	}
	return 0;
}

static inline int can_write(int fd)
{
	fd_set fds;
	struct timeval tv = {0, 0};
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	return select(fd + 1, NULL, &fds, NULL, &tv);
}

static inline int can_read(int fd)
{
	fd_set fds;
	struct timeval tv = {0, 0};
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	return select(fd + 1, &fds, NULL, NULL, &tv);
}

#define CI_BYTES	1
#define CI_FILES	2
#define CI_LOCKED	3
#define CI_LOADING	4
#define CI_TIMERS	5
#define CI_TRANSFER	6
#define CI_CONNECTING	7
#define CI_KEEP		8
#define CI_LIST		9

/* os_dep.c */

struct terminal;

struct open_in_new {
	uchar *text;
	uchar *hk;
	void (*fn)(struct terminal *term, uchar *, uchar *);
};

void close_fork_tty();
int get_system_env(void);
int is_xterm(void);
int can_twterm(void);
int get_terminal_size(int, int *, int *);
void handle_terminal_resize(int, void (*)(void));
void unhandle_terminal_resize(int);
void set_bin(int);
int c_pipe(int *);
int get_input_handle(void);
int get_output_handle(void);
int get_ctl_handle(void);
void want_draw(void);
void done_draw(void);
void terminate_osdep(void);
void *handle_mouse(int, void (*)(void *, uchar *, int), void *);
void unhandle_mouse(void *);
int check_file_name(uchar *);
int start_thread(void (*)(void *, int), void *, int);
uchar *get_clipboard_text(void);
void set_clipboard_text(struct terminal *, uchar *);
void set_window_title(uchar *);
uchar *get_window_title(void);
int is_safe_in_shell(uchar);
void check_shell_security(uchar **);
void block_stdin(void);
void unblock_stdin(void);
int exe(char *, int);
int resize_window(int, int);
int can_resize_window(int);
int can_open_os_shell(int);
struct open_in_new *get_open_in_new(int);
void set_highpri(void);
#ifdef HAVE_OPEN_PREALLOC
int open_prealloc(char *, int, int, int);
void prealloc_truncate(int, int);
#else
static inline void prealloc_truncate(int x, int y) {}
#endif
void os_cfmakeraw(struct termios *t);

/* memory.c */

#define SH_CHECK_QUOTA		0
#define SH_FREE_SOMETHING	1
#define SH_FREE_ALL		2

#define ST_SOMETHING_FREED	1
#define ST_CACHE_EMPTY	2

int shrink_memory(int);
void register_cache_upcall(int (*)(int), uchar *);
void free_all_caches(void);

/* select.c */

typedef long ttime;
typedef unsigned tcount;

extern volatile int terminate_loop;

long select_info(int);
void select_loop(void (*)(void));
int register_bottom_half(void (*)(void *), void *);
void unregister_bottom_half(void (*)(void *), void *);
void check_bottom_halves(void);
int install_timer(ttime, void (*)(void *), void *);
void kill_timer(int);
ttime get_time(void);

#define H_READ	0
#define H_WRITE	1
#define H_ERROR	2
#define H_DATA	3

void *get_handler(int, int);
void set_handlers(int, void (*)(void *), void (*)(void *), void (*)(void *), void *);
void install_signal_handler(int, void (*)(void *), void *, int);
void set_sigcld(void);

/* dns.c */

typedef unsigned ip__address;

int do_real_lookup(uchar *, ip__address *);
int find_host(uchar *, ip__address *, void **, void (*)(void *, int), void *);
int find_host_no_cache(uchar *, ip__address *, void **, void (*)(void *, int), void *);
void kill_dns_request(void **);
void init_dns(void);

/* cache.c */

struct cache_entry {
	struct cache_entry *next;
	struct cache_entry *prev;
	uchar *url;
	uchar *head;
	int http_code;
	uchar *redirect;
	int redirect_get;
	int length;
	int incomplete;
	int tgc;
	uchar *last_modified;
	time_t expire_time;	/* 0 never, 1 always */
	int data_size;
	struct list_head frag;	/* struct fragment */
	tcount count;
	tcount count2;
	int refcount;
#ifdef HAVE_SSL
	uchar *ssl_info;
#endif
};

struct fragment {
	struct fragment *next;
	struct fragment *prev;
	int offset;
	int length;
	int real_length;
	uchar data[1];
};

void init_cache(void);
long cache_info(int);
int find_in_cache(uchar *, struct cache_entry **);
int get_cache_entry(uchar *, struct cache_entry **);
int get_cache_data(struct cache_entry *e, uchar **, int *);
int add_fragment(struct cache_entry *, int, uchar *, int);
void defrag_entry(struct cache_entry *);
void truncate_entry(struct cache_entry *, int, int);
void free_entry_to(struct cache_entry *, int);
void delete_entry_content(struct cache_entry *);
void delete_cache_entry(struct cache_entry *);
void garbage_collection(int);

/* sched.c */

#define PRI_MAIN	0
#define PRI_DOWNLOAD	0
#define PRI_FRAME	1
#define PRI_NEED_IMG	2
#define PRI_IMG		3
#define PRI_PRELOAD	4
#define PRI_CANCEL	5
#define N_PRI		6

struct remaining_info {
	int	valid;
	int	size, loaded, last_loaded, cur_loaded;
	int	pos;
	ttime	elapsed;
	ttime	last_time;
	ttime	dis_b;
	int	data_in_secs[CURRENT_SPD_SEC];
	int	timer;
};

typedef struct chunk Chunk;
struct chunk {
	char	*base;
	ulong	len;
};

typedef struct block Block;
struct Block {
	Block*	next;
	Block*	list;
	uchar*	rp;			/* first unconsumed byte */
	uchar*	wp;			/* first empty byte */
	uchar*	lim;			/* 1 past the end of the buffer */
	uchar*	base;			/* start of the buffer */
	ushort	flag;
};
#define BLEN(s) ((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

typedef struct connection Connection;
struct connection {
	Connection *next;
	Connection *prev;
	tcount	count;
	uchar	*url;
	uchar	*prev_url;   /* allocated string with referrer or NULL */
	int	running;
	int	state;
	int	prev_error;
	int	from;
	int	pri[N_PRI];
	int	no_cache;
	int	sock1;
	int	sock2;
	void	*dnsquery;
	pid_t	pid;
	int	tries;
	struct list_head statuss;
	void	*info;
	void	*buffer;
	void	(*conn_func)(void *);
	struct cache_entry *cache;
	int	received;
	int	est_length;
	int	unrestartable;
	struct remaining_info prg;
	int	timer;
	int	detached;
#ifdef	HAVE_SSL
	SSL	*ssl;
	int	no_tsl;
#endif
};

void no_owner(void);

static inline int getpri(Connection *c)
{
	int i;

	for (i = 0; i < N_PRI; i++)
		if (c->pri[i])
			return i;
	no_owner();
	return N_PRI;
}

#define NC_ALWAYS_CACHE	0
#define NC_CACHE	1
#define NC_IF_MOD	2
#define NC_RELOAD	3
#define NC_PR_NO_CACHE	4

#define S_WAIT		0
#define S_DNS		1
#define S_CONN		2
#define S_SSL_NEG	3
#define S_SENT		4
#define S_LOGIN		5
#define S_GETH		6
#define S_PROC		7
#define S_TRANS		8

#define S_WAIT_REDIR		-999
#define S_OK			-1000
#define S_INTERRUPTED		-1001
#define S_EXCEPT		-1002
#define S_INTERNAL		-1003
#define S_OUT_OF_MEM		-1004
#define S_NO_DNS		-1005
#define S_CANT_WRITE		-1006
#define S_CANT_READ		-1007
#define S_MODIFIED		-1008
#define S_BAD_URL		-1009
#define S_TIMEOUT		-1010
#define S_RESTART		-1011
#define S_STATE			-1012
#define S_CYCLIC_REDIRECT	-1013

#define S_HTTP_ERROR		-1100
#define S_HTTP_100		-1101
#define S_HTTP_204		-1102

#define S_FILE_TYPE		-1200
#define S_FILE_ERROR		-1201

#define S_FTP_ERROR		-1300
#define S_FTP_UNAVAIL		-1301
#define S_FTP_LOGIN		-1302
#define S_FTP_PORT		-1303
#define S_FTP_NO_FILE		-1304
#define S_FTP_FILE_ERROR	-1305

#define S_SSL_ERROR		-1400
#define S_NO_SSL		-1401

extern struct s_msg_dsc {
	int	n;
	uchar	*msg;
} msg_dsc[];

struct status {
	struct status *next;
	struct status *prev;
	Connection *c;
	struct cache_entry *ce;
	int state;
	int prev_error;
	int pri;
	void (*end)(struct status *, void *);
	void *data;
	struct remaining_info *prg;
};

uchar *get_proxy(uchar *url);
void check_queue(void);
long connect_info(int);
void send_connection_info(Connection *c);
void setcstate(Connection *c, int);
int get_keepalive_socket(Connection *c);
void add_keepalive_socket(Connection *c, ttime);
void run_connection(Connection *c);
void retry_connection(Connection *c);
void abort_connection(Connection *c);
void end_connection(Connection *c);
int load_url(uchar *, uchar *, struct status *, int, int);
void change_connection(struct status *, struct status *, int);
void detach_connection(struct status *, int);
void abort_all_connections(void);
void abort_background_connections(void);
int is_entry_used(struct cache_entry *);
void connection_timeout(Connection *);
void set_timeout(Connection *);
void add_blacklist_entry(uchar *, int);
void del_blacklist_entry(uchar *, int);
int get_blacklist_flags(uchar *);
void free_blacklist(void);

#define BL_HTTP10	1
#define BL_NO_CHARSET	2

/* url.c */

struct session;

#define POST_CHAR 1

static inline int end_of_dir(uchar c)
{
	return c == POST_CHAR || c == '#' || c == ';' || c == '?';
}

int parse_url(uchar *, int *, uchar **, int *, uchar **, int *, uchar **, int *, uchar **, int *, uchar **, int *, uchar **);
uchar *get_host_name(uchar *);
uchar *get_host_and_pass(uchar *);
uchar *get_user_name(uchar *);
uchar *get_pass(uchar *);
int get_port(uchar *);
uchar *get_port_str(uchar *);
void (*get_protocol_handle(uchar *))(Connection *);
void (*get_external_protocol_function(uchar *))(struct session *, uchar *);
uchar *get_url_data(uchar *);
uchar *join_urls(uchar *, uchar *);
uchar *translate_url(uchar *, uchar *);
uchar *extract_position(uchar *);
void get_filename_from_url(uchar *, uchar **, int *);

/* connect.c */

typedef struct read_buffer Rdbuff;
struct read_buffer {
	int	sock;
	int	len;
	int	close;
	char	*idmsg;			/* for debugging */
	void	(*done)(Connection *, Rdbuff *);
	uchar	data[1];
};

void close_socket(int *);
void make_connection(Connection *, int, int *, void (*)(Connection *));
int get_pasv_socket(Connection *, int, int *, uchar *);
void write_to_socket(Connection *, int, uchar *, int, void (*)(Connection *));
Rdbuff *alloc_read_buffer(Connection *c);
void read_from_socket(Connection *, int, Rdbuff *, void (*)(Connection *, Rdbuff *));
void kill_buffer_data(Rdbuff *, int);

/* cookies.c */

struct cookie {
	struct cookie *next;
	struct cookie *prev;
	uchar *name, *value;
	uchar *server;
	uchar *path, *domain;
	time_t expires; /* zero means undefined */
	int secure;
	int id;
};

struct c_domain {
	struct c_domain *next;
	struct c_domain *prev;
	uchar domain[1];
};


extern struct list_head cookies;
extern struct list_head c_domains;

int set_cookie(struct terminal *, uchar *, uchar *);
void send_cookies(uchar **, int *, uchar *);
void init_cookies(void);
void cleanup_cookies(void);
int is_in_domain(uchar *d, uchar *s);
int is_path_prefix(uchar *d, uchar *s);
int cookie_expired(struct cookie *c);
void free_cookie(struct cookie *c);

/* auth.c */

uchar *get_auth_realm(uchar *url, uchar *head, int proxy);
char *get_auth_string(char *url);
void cleanup_auth();
void add_auth(uchar *url, uchar *realm, uchar *user, uchar *password, int proxy);
int find_auth(uchar *url, uchar *realm);

/* http.c */

int get_http_code(uchar *head, int *code, int *version);
uchar *parse_http_header(uchar *, uchar *, uchar **);
uchar *parse_header_param(uchar *, uchar *);
void http_func(Connection *);
void proxy_func(Connection *);


/* https.c */

void https_func(Connection *c);
#ifdef HAVE_SSL
void ssl_finish(void);
SSL *getSSL(void);
#endif

/* file.c */

void file_func(Connection *);

/* finger.c */

void finger_func(Connection *);

/* ftp.c */

extern int fast_ftp;
extern int passive_ftp;
void ftp_func(Connection *);

/* smb.c */

void smb_func(Connection *);

/* mailto.c */

void mailto_func(struct session *, uchar *);
void telnet_func(struct session *, uchar *);
void tn3270_func(struct session *, uchar *);

/* kbd.c */

#define BM_BUTT		15
#define B_LEFT		0
#define B_MIDDLE	1
#define B_RIGHT		2
#define B_WHEELUP	8
#define B_WHEELDOWN	9
#define B_WHEELUP1	10
#define B_WHEELDOWN1	11
#define B_WHEELLEFT	12
#define B_WHEELRIGHT	13
#define B_WHEELLEFT1	14
#define B_WHEELRIGHT1	15

#define BM_ACT		48
#define B_DOWN		0
#define B_UP		16
#define B_DRAG		32
#define B_MOVE		48

#define KBD_ENTER	-0x100
#define KBD_BS		-0x101
#define KBD_TAB		-0x102
#define KBD_ESC		-0x103
#define KBD_LEFT	-0x104
#define KBD_RIGHT	-0x105
#define KBD_UP		-0x106
#define KBD_DOWN	-0x107
#define KBD_INS		-0x108
#define KBD_DEL		-0x109
#define KBD_HOME	-0x10a
#define KBD_END		-0x10b
#define KBD_PAGE_UP	-0x10c
#define KBD_PAGE_DOWN	-0x10d

#define KBD_F1		-0x120
#define KBD_F2		-0x121
#define KBD_F3		-0x122
#define KBD_F4		-0x123
#define KBD_F5		-0x124
#define KBD_F6		-0x125
#define KBD_F7		-0x126
#define KBD_F8		-0x127
#define KBD_F9		-0x128
#define KBD_F10		-0x129
#define KBD_F11		-0x12a
#define KBD_F12		-0x12b

#define KBD_CTRL_C	-0x200
#define KBD_CLOSE	-0x201

#define KBD_SHIFT	1
#define KBD_CTRL	2
#define KBD_ALT		4

void handle_trm(int, int, int, int, int, void *, int);
void free_all_itrms(void);
void resize_terminal(void);
void dispatch_special(uchar *);
int is_blocked(void);
void kbd_ctrl_c(void);

struct os2_key {
	int x, y;
};

extern struct os2_key os2xtd[256];

struct itrm;

#if defined(GRDRV_SVGALIB) || defined(GRDRV_FB)
extern int kbd_set_raw;
struct itrm *handle_svgalib_keyboard(void (*)(void *, uchar *, int));
void svgalib_free_trm(struct itrm *);
void svgalib_block_itrm(struct itrm *);
void svgalib_unblock_itrm(struct itrm *);
#endif




struct rgb {
	uchar r, g, b; /* This is 3*8 bits with sRGB gamma (in sRGB space)
				* This is not rounded. */
	uchar pad;
};

#ifdef G

/* lru.c */

struct lru_entry{
	struct lru_entry *above, *below, *next;
	struct lru_entry **previous;
	void *data;
	unsigned bytes_consumed;
};

struct lru{
	int (*compare_function)(void *, void *);
	struct lru_entry *top, *bottom;
	unsigned bytes, max_bytes, items;
};

void lru_insert(struct lru *cache, void *entry, struct lru_entry ** row
	, unsigned bytes_consumed);
void * lru_get_bottom(struct lru *cache);
void lru_destroy_bottom(struct lru* cache);
void lru_destroy (struct lru* cache);
void lru_init (struct lru *cache, int
	(*compare_function)(void *entry, void *templ), int max_bytes);
void *lru_lookup(struct lru *cache, void *templ, struct lru_entry *row);

/* drivers.c */

struct irgb{
        int r,g,b; /* 0xffff=full white, 0x0000=full black */
};

/* Bitmap is allowed to pass only to that driver from which was obtained.
 * It is forbidden to get bitmap from svga driver and pass it to X driver.
 * It is impossible to get an error when registering a bitmap
 */
struct bitmap{
	int x,y; /* Dimensions */
	int skip; /* Byte distance between vertically consecutive pixels */
	void *data; /* Pointer to room for topleft pixel */
	void *user; /* For implementing LRU algor by higher layer or what*/
	void *flags; /* Allocation flags for the driver */
};

struct rect {
	int x1, x2, y1, y2;
};

struct rect_set {
	int rl;
	int m;
	struct rect r[1];
};

struct graphics_device{
        /* Only graphics driver is allowed to write to this */

        struct rect size; /* Size of the window */
        /*int left, right, top, bottom;*/
	struct rect clip;
                /* right, bottom are coords of the first point that are outside the clipping area */
        
        struct graphics_driver *drv;
        void * driver_data;

        /* Only user is allowed to write here, driver inits to zero's */
        void * user_data;
        void (*redraw_handler)(struct graphics_device *dev, struct rect *r);
        void (*resize_handler)(struct graphics_device *dev);
        void (*keyboard_handler)(struct graphics_device *dev, int key, int flags);
        void (*mouse_handler)(struct graphics_device *dev, int x, int y, int buttons);
};

struct graphics_driver{
	uchar *name;
	/* param is get from get_driver_param and saved into configure file */
	uchar *(*init_driver)(uchar *param, uchar *display);
	
	/* Creates new device and returns pointer to it */
	struct graphics_device *(* init_device)(void);
	
	/* Destroys the device */
	void (*shutdown_device)(struct graphics_device *dev);

	void (*shutdown_driver)(void);
	/* returns allocated string with parameter given to init_driver function */
	uchar *(*get_driver_param)(void);

	/* dest must have x and y filled in when get_empty_bitmap is called */
	int (*get_empty_bitmap)(struct bitmap *dest);

	/* dest must have x and y filled in when get_filled_bitmap is called. */
	/* bitmap is created, pre-filled with n_bytes of pattern, and registered */
	int (*get_filled_bitmap)(struct bitmap *dest, long color);

	void (*register_bitmap)(struct bitmap *bmp);

	void *(*prepare_strip)(struct bitmap *bmp, int top, int lines);
	void (*commit_strip)(struct bitmap *bmp, int top, int lines);

	/* Must not touch x and y. Suitable for re-registering. */
	void (*unregister_bitmap)(struct bitmap *bmp);
	void (*draw_bitmap)(struct graphics_device *dev, struct bitmap *hndl, int x, int y);
	void (*draw_bitmaps)(struct graphics_device *dev, struct bitmap **hndls, int n, int x, int y);

	/* Input into get_color has gamma 1/display_gamma.
	 * Input of 255 means exactly the largest sample the display is able to produce.
	 * Thus, if we have 3 bits for red, we will perform this code:
	 * red=((red*7)+127)/255;
	 */
	long (*get_color)(int rgb);
	
	void (*fill_area)(struct graphics_device *dev, int x1, int y1, int x2, int y2, long color);
	void (*draw_hline)(struct graphics_device *dev, int left, int y, int right, long color);
	void (*draw_vline)(struct graphics_device *dev, int x, int top, int bottom, long color);
	int (*hscroll)(struct graphics_device *dev, struct rect_set **set, int sc);
	int (*vscroll)(struct graphics_device *dev, struct rect_set **set, int sc);
	 /* When scrolling, the empty spaces will have undefined contents. */
	 /* returns:
	    0 - the caller should not care about redrawing, redraw will be sent
	    1 - the caller should redraw uncovered area */
	 /* when set is not NULL rectangles in the set (uncovered area) should be redrawn */
	void (*set_clip_area)(struct graphics_device *dev, struct rect *r);

	/* restore old videomode and disable drawing on terminal */
	int (*block)(struct graphics_device *dev);
	/* reenable the terminal */
	void (*unblock)(struct graphics_device *dev);

	/* set window title. title is in utf-8 encoding -- you should recode it to device charset */
	/* if device doesn't support titles (svgalib, framebuffer), this should be NULL, not empty function ! */
	void (*set_title)(struct graphics_device *dev, uchar *title);
	
	/* -if !NULL executes command on this graphics device, 
	   -if NULL links uses generic (console) command executing 
	    functions
	   -return value is the same as of the 'system' syscall 
	   -if flag is !0, run command in separate shell
	    else run command directly
	 */
	int (*exec)(uchar *command, int flag); 

		/* Data layout 
		    * depth
		    *  8 7 6 5 4 3 2 1 0
		    * +-+---------+-----+
		    * | |         |     |
		    * +-+---------+-----+
		    *
		    * 0 - 2 Number of bytes per pixel in passed bitmaps
		    * 3 - 7 Number of significant bits per pixel -- 1, 4, 8, 15, 16, 24
		    * 8   0-- normal order, 1-- misordered.Has the same value as vga_misordered from the VGA mode.
		    *
		    * This number is to be used by the layer that generates images.
		    * Memory layout for 1 bytes per pixel is:
		    * 2 colors:
		    *  7 6 5 4 3 2 1 0
		    * +-------------+-+
		    * |      0      |B| B is The Bit. 0 black, 1 white
		    * +-------------+-+
		    * 
		    * 16 colors:
		    *  7 6 5 4 3 2 1 0
		    * +-------+-------+
		    * |   0   | PIXEL | Pixel is 4-bit index into palette
		    * +-------+-------+
		    *
		    * 256 colors:
		    *  7 6 5 4 3 2 1 0
		    * +---------------+
		    * |  --PIXEL--    | Pixels is 8-bit index into palette
		    * +---------------+
		    */
	int depth;
	/* size of screen. only for drivers that use virtual devices */
	int x, y;
	int flags;		/* GD_xxx flags */
	int codepage;
	/* -if exec is NULL string is unused
 	   -otherwise this string describes shell to be executed by the 
	    exec function, the '%' char means string to be executed
	   -shell cannot be NULL
	   -if exec is !NULL and shell is empty, exec should use some 
 		    default shell (e.g. "xterm -e %")
	*/
	uchar *shell;  
};

#define GD_DONT_USE_SCROLL	1
#define GD_NEED_CODEPAGE	2

extern struct graphics_driver *drv;

uchar *init_graphics(uchar *, uchar *, uchar *);
void shutdown_graphics(void);
void update_driver_param(void);

int dummy_block(struct graphics_device *);
void dummy_unblock(struct graphics_device *);

extern struct graphics_device **virtual_devices;
extern int n_virtual_devices;
extern struct graphics_device *current_virtual_device;

int init_virtual_devices(struct graphics_driver *, int);
struct graphics_device *init_virtual_device(void);
void switch_virtual_device(int);
void shutdown_virtual_device(struct graphics_device *dev);
void shutdown_virtual_devices(void);

/* dip.c */

/* Digital Image Processing utilities
 * (c) 2000 Clock <clock@atrey.karlin.mff.cuni.cz>
 *
 * This file is a part of Links
 *
 * This file does gray scaling (for prescaling fonts), color scaling (for scaling images
 * where different size is defined in the HTML), two colors mixing (alpha monochromatic letter
 * on a monochromatic backround and font operations.
 */

#define FC_COLOR 0
#define FC_BW 1

extern unsigned long aspect, aspect_native; /* Must hold at least 20 bits */
extern double bfu_aspect;
extern int aspect_on;
extern struct lru font_cache; /* For menu.c so that it can print out cache
			       * statistics. */
#endif /* #ifdef G */

extern double display_red_gamma,display_green_gamma,display_blue_gamma;
extern double user_gamma;
extern int menu_font_size;
extern double sRGB_gamma;

#ifdef G

#define G_BFU_FONT_SIZE menu_font_size

struct read_work{
	uchar *pointer;
	int length;
};

struct letter {
        int begin; /* Begin in the byte stream (of PNG data) */
        int length; /* Length (in bytes) of the PNG data in the byte stream */
        int code; /* Unicode code of the character */
        int xsize; /* x size of the PNG image */
        int ysize; /* y size of the PNG image */
	struct lru_entry* color_list;
	struct lru_entry* bw_list;
};

struct font {
	uchar *family;
	uchar *weight;
	uchar *slant;
	uchar *adstyl;
	uchar *spacing;
	int begin; /* Begin in the letter stream */
	int length; /* Length in the letter stream */
};

struct style{
	int refcount;
	uchar r0, g0, b0, r1, g1, b1;
       	/* ?0 are background, ?1 foreground.
	 * These are unrounded 8-bit sRGB space 
	 */
	int height;
	int flags; /* non-zero means underline */
	long underline_color; /* Valid only if flags are nonzero */
	int *table; /* First is refcount, then n_fonts entries. Total
                     * size is n_fonts+1 integers.
		     */
	int mono_space; /* -1 if the font is not monospaced
			 * width of the space otherwise
			 */
	int mono_height; /* Height of the space if mono_space is >=0
			  * undefined otherwise
			  */
	/*
	uchar font[1];
	*/
};

struct font_cache_entry{
	int type; /* One of FC_BW or FC_COLOR */
	int r0,g0,b0,r1,g1,b1; /* Invalid for FC_BW */
	struct bitmap bitmap; /* If type==FC_BW, then this is not a normal registered
	                       * bitmap, but a black-and-white bitmap
			       */
	int mono_space, mono_height; /* if the letter was rendered for a
	monospace font, then size of the space. Otherwise, mono_space
	is -1 and mono_height is undefined. */
};


#endif

extern int dither_letters;

#ifdef G

struct cached_image;

void g_print_text(struct graphics_driver *, struct graphics_device *device, int x, int y, struct style *style, uchar *text, int *width);
int g_text_width(struct style *style, uchar *text);
int g_char_width(struct style *style, int ch);
void destroy_font_cache(void);
uchar apply_gamma_single_8_to_8(uchar input, float gamma);
unsigned short apply_gamma_single_8_to_16(uchar input, float gamma);
uchar apply_gamma_single_16_to_8(unsigned short input, float gamma);
unsigned short apply_gamma_single_16_to_16(unsigned short input, float gamma);
void mix_one_color(uchar *dest, int length,
		   int r, int g, int b);
void apply_gamma_exponent_24_to_48(unsigned short *dest, uchar *src, int
			  lenght, float red_gamma, float green_gamma, float
			  blue_gamma);
void make_gamma_table(struct cached_image *cimg);
void apply_gamma_exponent_24_to_48_table(unsigned short *dest, uchar *src
	,int lenght, unsigned short *gamma_table);
void apply_gamma_exponent_48_to_48_table(unsigned short *dest,
		unsigned short *src, int lenght, unsigned short *table);
void apply_gamma_exponent_48_to_48(unsigned short *dest,
		unsigned short *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma);
void apply_gamma_exponent_and_undercolor_32_to_48_table(unsigned short *dest,
		uchar *src, int lenght, unsigned short *table
		,unsigned short rb, unsigned short gb, unsigned short bb);
void apply_gamma_exponent_and_undercolor_32_to_48(unsigned short *dest,
		uchar *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma, unsigned short rb, unsigned
		short gb, unsigned short bb);
void apply_gamma_exponent_and_undercolor_64_to_48_table(unsigned short *dest
		,unsigned short *src, int lenght, unsigned short *gamma_table
		,unsigned short rb, unsigned short gb, unsigned short bb);
void apply_gamma_exponent_and_undercolor_64_to_48(unsigned short *dest,
		unsigned short *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma, unsigned short rb, unsigned
		short gb, unsigned short bb);
void mix_one_color_48(unsigned short *dest, int length,
		   unsigned short r, unsigned short g, unsigned short b);
void mix_one_color_24(uchar *dest, int length,
		   uchar r, uchar g, uchar b);
void scale_color(unsigned short *in, int ix, int iy, unsigned short **out,
	int ox, int oy);
void update_aspect(void);

struct wrap_struct {
	struct style *style;
	uchar *text;
	int pos;
	int width;
	void *obj;
	void *last_wrap_obj;
	uchar *last_wrap;
};

int g_wrap_text(struct wrap_struct *);

#define FF_UNDERLINE	1

struct style *g_get_style(int fg, int bg, int size, uchar *font, int fflags);
struct style *g_invert_style(struct style *);
void g_free_style(struct style *style0);
struct style *g_clone_style(struct style *);

extern long gamma_cache_color;
extern int gamma_cache_rgb;

extern long real_dip_get_color_sRGB(int rgb);

static inline long dip_get_color_sRGB(int rgb)
{
	if (rgb == gamma_cache_rgb) return gamma_cache_color;
	else return real_dip_get_color_sRGB(rgb);
}

extern void apply_gamma_exponent_8_to_8(uchar *dest, uchar *src, int lenght,
					float red_gamma, float green_gamma,
					float blue_gamma);
extern void apply_gamma_exponent_8_to_16(unsigned short *dest, uchar *src, int lenght,
					float red_gamma, float green_gamma,
					float blue_gamma);


void init_dip(void);
void shutdown_dip(void);
void get_links_icon(uchar **data, int *width, int* height, int depth);

#endif

/* x.c */
#if defined(G) && defined (GRDRV_X)
void x_set_clipboard_text(struct graphics_device *gd, uchar * text);
#endif

/* links_icon.c */

#ifdef G
extern uchar *links_icon;
#endif /* #ifdef G */

/* dither.c */

#ifdef G

/* Dithering functions (for blocks of pixels being dithered into bitmaps) */
void dither (unsigned short *in, struct bitmap *out);
int *dither_start(unsigned short *in, struct bitmap *out);
void dither_restart(unsigned short *in, struct bitmap *out, int *dregs);
extern void (*round_fn)(unsigned short *in, struct bitmap *out);

long (*get_color_fn(int depth))(int rgb);
void init_dither(int depth);
void round_color_sRGB_to_48(unsigned short *red, unsigned short *green,
		unsigned short *blue, int rgb);

#endif

/* terminal.c */

typedef unsigned short chr;

struct event {
	long ev;
	long x;
	long y;
	long b;
};

#define EV_INIT		0
#define EV_KBD		1
#define EV_MOUSE	2
#define EV_REDRAW	3
#define EV_RESIZE	4
#define EV_ABORT	5

struct window {
	struct window *next;
	struct window *prev;
	void (*handler)(struct window *, struct event *, int fwd);
	void *data;
	int xp, yp;
	struct terminal *term;
#ifdef G
	struct rect pos;
	struct rect redr;
#endif
};

#define MAX_TERM_LEN	32	/* this must be multiple of 8! (alignment problems) */

#define MAX_CWD_LEN	256	/* this must be multiple of 8! (alignment problems) */	

#define ENV_XWIN	1
#define ENV_SCREEN	2
#define ENV_OS2VIO	4
#define ENV_BE		8
#define ENV_TWIN	16
#define ENV_WIN32	32
#define ENV_G		32768

struct terminal {
	struct terminal *next;
	struct terminal *prev;
	tcount count;

	int x;
	int y;
	/* text only */
	int master;
	int fdin;
	int fdout;
	int environment;
	uchar term[MAX_TERM_LEN];
	uchar cwd[MAX_CWD_LEN];
	unsigned *screen;
	unsigned *last_screen;
	struct term_spec *spec;
	int cx;
	int cy;
	int lcx;
	int lcy;
	int dirty;
	int redrawing;
	int blocked;
	uchar *input_queue;
	int qlen;
	/* end-of text only */

	struct list_head windows;
	uchar *title;
#ifdef G
	struct graphics_device *dev;
#endif
};

struct term_spec {
	struct term_spec *next;
	struct term_spec *prev;
	uchar term[MAX_TERM_LEN];
	int mode;
	int m11_hack;
	int restrict_852;
	int block_cursor;
	int col;
	int charset;
};

#define TERM_DUMB	0
#define TERM_VT100	1
#define TERM_LINUX	2
#define TERM_KOI8	3

#define ATTR_FRAME	0x8000

extern struct list_head term_specs;
extern struct list_head terminals;

int hard_write(int, uchar *, int);
int hard_read(int, uchar *, int);
uchar *get_cwd(void);
void set_cwd(uchar *);
struct terminal *init_term(int, int, void (*)(struct window *, struct event *, int));
#ifdef G
struct terminal *init_gfx_term(void (*)(struct window *, struct event *, int), void *, int);
int restrict_clip_area(struct graphics_device *, struct rect *, int, int, int, int);
#endif
void sync_term_specs(void);
struct term_spec *new_term_spec(uchar *);
void free_term_specs(void);
void destroy_terminal(struct terminal *);
void redraw_terminal(struct terminal *);
void redraw_terminal_all(struct terminal *);
void redraw_terminal_cls(struct terminal *);
void cls_redraw_all_terminals(void);
void redraw_below_window(struct window *);
void add_window(struct terminal *, void (*)(struct window *, struct event *, int), void *);
void add_window_at_pos(struct terminal *, void (*)(struct window *, struct event *, int), void *, struct window *);
void delete_window(struct window *);
void delete_window_ev(struct window *, struct event *ev);
void set_window_ptr(struct window *, int, int);
void get_parent_ptr(struct window *, int *, int *);
void add_empty_window(struct terminal *, void (*)(void *), void *);
void draw_to_window(struct window *, void (*)(struct terminal *, void *), void *);
void redraw_screen(struct terminal *);
void redraw_all_terminals(void);

#ifdef G

void set_window_pos(struct window *, int, int, int, int);
int do_rects_intersect(struct rect *, struct rect *);
void intersect_rect(struct rect *, struct rect *, struct rect *);
void unite_rect(struct rect *, struct rect *, struct rect *);
int is_rect_valid(struct rect *);

struct rect_set *init_rect_set(void);
void add_to_rect_set(struct rect_set **, struct rect *);
void exclude_rect_from_set(struct rect_set **, struct rect *);
static inline void exclude_from_set(struct rect_set **s, int x1, int y1, int x2, int y2)
{
	struct rect r;
	r.x1 = x1, r.x2 = x2, r.y1 = y1, r.y2 = y2;
	exclude_rect_from_set(s, &r);
}
void t_redraw(struct graphics_device *, struct rect *);

#endif

/* text only */
void set_char(struct terminal *, int, int, unsigned);
unsigned get_char(struct terminal *, int, int);
void set_color(struct terminal *, int, int, unsigned);
void set_only_char(struct terminal *, int, int, unsigned);
void set_line(struct terminal *, int, int, int, chr *);
void set_line_color(struct terminal *, int, int, int, unsigned);
void fill_area(struct terminal *, int, int, int, int, unsigned);
void draw_frame(struct terminal *, int, int, int, int, unsigned, int);
void print_text(struct terminal *, int, int, int, uchar *, unsigned);
void set_cursor(struct terminal *, int, int, int, int);

void destroy_all_terminals(void);
void block_itrm(int);
int unblock_itrm(int);
void exec_thread(uchar *, int);
void close_handle(void *);

#define TERM_FN_TITLE	1
#define TERM_FN_RESIZE	2

void exec_on_terminal(struct terminal *, uchar *, uchar *, int);
void set_terminal_title(struct terminal *, uchar *);
void do_terminal_function(struct terminal *, uchar, uchar *);

/* language.c */

#include "language.h"

extern uchar dummyarray[];

extern int current_language;

void init_trans(void);
void shutdown_trans(void);
uchar *get_text_translation(uchar *, struct terminal *term);
uchar *get_english_translation(uchar *);
void set_language(int);
int n_languages(void);
uchar *language_name(int);
int language_index(uchar *);

#define _(_x_, _y_) get_text_translation(_x_, _y_)
#define TEXT(x) (dummyarray + x)

/* af_unix.c */

int bind_to_af_unix(void);
void af_unix_close(void);

/* main.c */

#define RET_OK		0
#define RET_ERROR	1
#define RET_SIGNAL	2
#define RET_SYNTAX	3
#define RET_FATAL	4

extern int retval;

extern uchar *path_to_exe;
extern uchar **g_argv;
extern int g_argc;

void sig_tstp(struct terminal *t);
void sig_cont(struct terminal *t);
void sig_ign(void *x);

void unhandle_terminal_signals(struct terminal *term);
int attach_terminal(int, int, int, void *, int);
#ifdef G
int attach_g_terminal(void *, int);
#endif
void program_exit(void);

/* types.c */

struct assoc {
	struct assoc *next;
	struct assoc *prev;
	uchar type;
	int depth;
	void *fotr;
	
	uchar *label;
	uchar *ct;
	uchar *prog;
	int cons;
	int xwin;
	int block;
	int ask;
	int system;
};

struct extension {
	struct extension *next;
	struct extension *prev;
	uchar type;
	int depth;
	void *fotr;

	uchar *ext;
	uchar *ct;
};

struct protocol_program {
	struct protocol_program *next;
	struct protocol_program *prev;
	uchar *prog;
	int system;
};

extern struct list assoc;
extern struct list extensions;

extern struct list_head mailto_prog;
extern struct list_head telnet_prog;
extern struct list_head tn3270_prog;

uchar *get_content_type(uchar *, uchar *);
struct assoc *get_type_assoc(struct terminal *term, uchar *, int *);
void update_assoc(struct assoc *);
void update_ext(struct extension *);
void create_initial_extensions(void);
void update_prog(struct list_head *, uchar *, int);
uchar *get_prog(struct list_head *);
void free_types(void);

extern void menu_assoc_manager(struct terminal *,void *,struct session *);
extern void menu_ext_manager(struct terminal *,void *,struct session *);

/* objreq.c */

#define O_WAITING	0
#define O_LOADING	1
#define O_FAILED	-1
#define O_INCOMPLETE	-2
#define O_OK		-3

struct object_request {
	struct object_request *next;
	struct object_request *prev;
	int refcount;
	tcount count;
	tcount term;
	struct status stat;
	struct cache_entry *ce;
	uchar *orig_url;
	uchar *url;
	uchar *prev_url;   /* allocated string with referrer or NULL */
	int pri;
	int cache;
	void (*upcall)(struct object_request *, void *);
	void *data;
	int redirect_cnt;
	int state;
	int timer;

	int last_bytes;

	ttime last_update;
	ttime z;
};

void request_object(struct terminal *, uchar *, uchar *, int, int, void (*)(struct object_request *, void *), void *, struct object_request **);
void clone_object(struct object_request *, struct object_request **);
void release_object(struct object_request **);
void release_object_get_stat(struct object_request **, struct status *, int);
void detach_object_connection(struct object_request *, int);

/* session.c */

struct link_def {
	uchar *link;
	uchar *target;

	uchar *shape;
	uchar *coords;

	uchar *onclick;
	uchar *ondblclick;
	uchar *onmousedown;
	uchar *onmouseup;
	uchar *onmouseover;
	uchar *onmouseout;
	uchar *onmousemove;
};

struct line {
	int l;
	chr c;
	chr *d;
};

struct point {
	int x;
	int y;
};

struct form {
	uchar *action;
	uchar *target;
	uchar *form_name;
	uchar *onsubmit;
	int method;
	int num;
};

#define FM_GET		0
#define FM_POST		1
#define FM_POST_MP	2

#define FC_TEXT		1
#define FC_PASSWORD	2
#define FC_FILE		3
#define FC_TEXTAREA	4
#define FC_CHECKBOX	5
#define FC_RADIO	6
#define FC_SELECT	7
#define FC_SUBMIT	8
#define FC_IMAGE	9
#define FC_RESET	10
#define FC_HIDDEN	11
#define FC_BUTTON	12

struct form_control {
	struct form_control *next;
	struct form_control *prev;
	int form_num;	/* cislo formulare */
	int ctrl_num;	/* identifikace polozky v ramci formulare */
	int g_ctrl_num;	/* identifikace polozky mezi vsemi polozkami (poradi v poli form_info) */
	int position;
	int method;
	uchar *action;
	uchar *target;
	uchar *onsubmit; /* script to be executed on submit */
	int type;
	uchar *name;
	uchar *form_name;
	uchar *alt;
	int ro;
	uchar *default_value;
	int default_state;
	int size;
	int cols, rows, wrap;
	int maxlength;
	int nvalues; /* number of values in a select item */
	uchar **values; /* values of a select item */
	uchar **labels; /* labels (shown text) of a select item */
	struct menu_item *menu;
};

struct form_state {
	int form_num;	/* cislo formulare */
	int ctrl_num;	/* identifikace polozky v ramci formulare */
	int g_ctrl_num;	/* identifikace polozky mezi vsemi polozkami (poradi v poli form_info) */
	int position;
	int type;
	uchar *value; /* selected value of a select item */
	int state; /* index of selected item of a select item */
	int vpos;
	int vypos;
	int changed;	/* flag if form element has changed --- for onchange handler */
};

struct link {
	int type;   /* one of L_XXX constants */
	int num;    /* link number (used when user turns on link numbering) */
	uchar *where;   /* URL of the link */
	uchar *target;   /* name of target frame where to open the link */
	uchar *where_img;   /* URL of image (if any) */
	uchar *img_alt;		/* alt of image (if any) - valid only when link is an image */
	struct form_control *form;   /* form info, usually NULL */
	unsigned sel_color;   /* link color */
	int n;   /* number of points */
	struct point *pos;
	struct js_event_spec *js_event;
	int obj_order;
#ifdef G
	struct rect r;
	struct g_object *obj;
#endif
};

#define L_LINK		0
#define L_BUTTON	1
#define L_CHECKBOX	2
#define L_SELECT	3
#define L_FIELD		4
#define L_AREA		5

struct link_bg {
	int x, y;
	unsigned c;
};

struct tag {
	struct tag *next;
	struct tag *prev;
	int x;
	int y;
	uchar name[1];
};

/* when you add anything, don't forget to initialize it in default.c on line:
 * struct document_setup dds = { ... };
 */
struct document_setup {
	int assume_cp, hard_assume;
	int tables, frames, images, image_names;
	int margin;
	int num_links, table_order;
	int auto_refresh;
	int font_size;
	int display_images;
	int image_scale;
	int target_in_new_window;
};


/* IMPORTANT!!!!!
 * if you add anything, fix it in compare_opt and if you add it into
 * document_setup, fix it in ds2do too
 */

struct document_options {
	int xw, yw; /* size of window */
	int xp, yp; /* pos of window */
	int col, cp, assume_cp, hard_assume;
	int tables, frames, images, image_names, margin;
	int js_enable;
	int plain;
	int num_links, table_order;
	int auto_refresh;
	struct rgb default_fg;
	struct rgb default_bg;
	struct rgb default_link;
	struct rgb default_vlink;
	uchar *framename;
	int font_size;
	int display_images;
	int image_scale;
	double bfu_aspect; /* 0.1 to 10.0, 1.0 default. >1 makes circle wider */
	int aspect_on;
};

static inline void ds2do(struct document_setup *ds, struct document_options *doo)
{
	doo->assume_cp = ds->assume_cp;
	doo->hard_assume = ds->hard_assume;
	doo->tables = ds->tables;
	doo->frames = ds->frames;
	doo->images = ds->images;
	doo->image_names = ds->image_names;
	doo->margin = ds->margin;
	doo->num_links = ds->num_links;
	doo->table_order = ds->table_order;
	doo->auto_refresh = ds->auto_refresh;
	doo->font_size = ds->font_size;
	doo->display_images = ds->display_images;
	doo->image_scale = ds->image_scale;
}

struct node {
	struct node *next;
	struct node *prev;
	int x, y;
	int xw, yw;
};

struct search {
	uchar c;
	int n:24;	/* This structure is size-critical */
	int x, y;
};

struct frameset_desc;

struct frame_desc {
	struct frameset_desc *subframe;
	uchar *name;
	uchar *url;
	int marginwidth;
	int marginheight;
	int line;
	int xw, yw;
};

struct frameset_desc {
	int n;			/* = x * y */
	int x, y;		/* velikost */
	int xp, yp;		/* pozice pri pridavani */
#ifdef JS
	uchar *onload_code;
#endif
	struct frame_desc f[1];
};

struct f_data;

#ifdef G

#define SHAPE_DEFAULT	0
#define SHAPE_RECT	1
#define SHAPE_CIRCLE	2
#define SHAPE_POLY	3

struct map_area {
	int shape;
	int *coords;
	int ncoords;
	int link_num;
};

struct image_map {
	int n_areas;
	struct map_area area[1];
};

struct background {
	int img;
	union {
		int sRGB; /* This is 3*8 bytes with sRGB_gamma (in sRGB space). This
			     is not rounded. */
		struct decoded_image *img;
	} u;
};

struct f_data_c;

struct g_object {
	void (*mouse_event)(struct f_data_c *, struct g_object *, int, int, int);
		/* pos is relative to object */
	void (*draw)(struct f_data_c *, struct g_object *, int, int);
		/* absolute pos on screen */
	void (*destruct)(struct g_object *);
	void (*get_list)(struct g_object *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw;
	struct g_object *parent;
	/* private data... */
};

struct g_object_text {
	void (*mouse_event)(struct f_data_c *, struct g_object_text *, int, int, int);
	void (*draw)(struct f_data_c *, struct g_object_text *, int, int);
	void (*destruct)(struct g_object_text *);
	void (*get_list)(struct g_object_text *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw;
	struct g_object *parent;
	int link_num;
	int link_order;
	struct image_map *map;
	int ismap;
	/* end of compatibility with g_object_image */
	struct style *style;
	struct decoded_image *bg;
	int srch_pos;
	uchar text[1];
};

struct g_object_line {
	void (*mouse_event)(struct f_data_c *, struct g_object_line *, int, int, int);
	void (*draw)(struct f_data_c *, struct g_object_line *, int, int);
	void (*destruct)(struct g_object_line *);
	void (*get_list)(struct g_object_line *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw;
	struct g_object *parent;
	struct background *bg;
	int n_entries;
	struct g_object *entries[1];
};

struct g_object_area {
	void (*mouse_event)(struct f_data_c *, struct g_object_area *, int, int, int);
	void (*draw)(struct f_data_c *, struct g_object_area *, int, int);
	void (*destruct)(struct g_object_area *);
	void (*get_list)(struct g_object_area *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw;
	struct g_object *parent;
	struct background *bg;
	int n_lfo;
	struct g_object **lfo;
	int n_rfo;
	struct g_object **rfo;
	int n_lines;
	struct g_object_line *lines[1];
};

struct g_object_table {
	void (*mouse_event)(struct f_data_c *, struct g_object_table *, int, int, int);
	void (*draw)(struct f_data_c *, struct g_object_table *, int, int);
	void (*destruct)(struct g_object_table *);
	void (*get_list)(struct g_object_table *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw;
	struct g_object *parent;
	struct table *t;
};

struct g_object_tag {
	void (*mouse_event)(struct f_data_c *, struct g_object *, int, int, int);
		/* pos is relative to object */
	void (*draw)(struct f_data_c *, struct g_object *, int, int);
		/* absolute pos on screen */
	void (*destruct)(struct g_object *);
	void (*get_list)(struct g_object *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw;
	struct g_object *parent;
	uchar name[1];
	/* private data... */
};

#define IM_PNG 0
#define IM_MNG 1

#ifdef HAVE_JPEG
#define IM_JPG 2
#endif /* #ifdef HAVE_JPEG */

#define IM_PCX 3
#define IM_BMP 4
#define IM_TIF 5
#define IM_GIF 6
#define IM_XBM 7

#ifdef HAVE_TIFF
#define IM_TIFF 8
#endif /* #ifdef HAVE_TIFF */

struct cached_image {
	struct cached_image *next;
	struct cached_image *prev;
	int refcount;

	int background_color; /* nezaokrouhlen pozad: 
			       * sRGB, (r<<16)+(g<<8)+b */
	uchar *url;
	int wanted_xw, wanted_yw;
	int scale;
	int aspect; /* What aspect ratio the image is for */

	int xww, yww;

	int width, height; /* From image header. 
			    * If the buffer is allocated, 
			    * it is always allocated to width*height.
			    * If the buffer is NULL then width and height
			    * are garbage.
			    */
	uchar image_type; /* IM_??? constant */
	uchar *buffer; /* Buffer with image data */
	uchar buffer_bytes_per_pixel; /* 3 or 4 or 6 or 8
				     * 3: RGB
				     * 4: RGBA
				     * 6: RRGGBB
				     * 8: RRGGBBAA
				     */
	float red_gamma, green_gamma, blue_gamma;
		/* data=light_from_monitor^[red|green|blue]_gamma.
		 * i. e. 0.45455 is here if the image is in sRGB
		 * makes sense only if buffer is !=NULL
		 */
	int gamma_stamp; /* Number that is increased every gamma change */
	struct bitmap bmp; /* Registered bitmap. bmp.x=-1 and bmp.y=-1
			    * if the bmp is not registered.
			    */
	int last_length; /* length of cache entry at which last decoding was
			  * done. Makes sense only if reparse==0
			  */
	int last_count2; /* Always valid. */
	void *decoder; 	      /* Decoder unfinished work. If NULL, decoder
			       * has finished or has not yet started.
			       */
	int rows_added; /* 1 if some rows were added inside the decoder */
	uchar state; /* 0...3 or 8...15 */
	uchar strip_optimized; /* 0 no strip optimization
				1 strip-optimized (no buffer allocated permanently
				and bitmap is always allocated)
			      */
	int *dregs; /* Only for stip-optimized cached images */
	unsigned short *gamma_table; /* When suitable and source is 8 bits per pixel,
			              * this is allocated to 256*3*sizeof(*gamma_table)
				      * = 1536 bytes and speeds up the gamma calculations
				      * tremendously */
};

struct g_object_image {
	void (*mouse_event)(struct f_data_c *, struct g_object_text *, int, int, int);
	void (*draw)(struct f_data_c *, struct g_object_image *, int, int);
	void (*destruct)(struct g_object *);
	void (*get_list)(struct g_object *, void (*)(struct g_object *parent, struct g_object *child));
	int x, y, xw, yw; /* For html parser. If xw or yw are zero, then entries
               background_color
               af
               width
               height
               image_type
               buffer
               buffer_bytes_per_pixel
               *_gamma
               gamma_stamp
               bmp
               last_length
               last_count2
               decoder
               rows_added
               reparse
       	are uninitialized and thus garbage
      	 */

	struct g_object *parent;
	int link_num;
	int link_order;
	struct image_map *map;
	int ismap;
	/* End of compatibility with g_object_text */

	struct xlist_head image_list;

	struct cached_image *cimg;
	struct additional_file *af;

	long id;
	uchar *name;
	uchar *alt;
	int vspace, hspace, border;
	uchar *orig_src;
	uchar *src;
	int background; /* Remembered background from insert_image
			 * (g_part->root->bg->u.sRGB)
			 */
};

void refresh_image(struct f_data_c *fd, struct g_object *img, ttime tm);

#endif

struct additional_file *request_additional_file(struct f_data *f, uchar *url);

struct js_event_spec {
#ifdef JS
	uchar *move_code;
	uchar *over_code;
	uchar *out_code;
	uchar *down_code;
	uchar *up_code;
	uchar *click_code;
	uchar *dbl_code;
	uchar *blur_code;
	uchar *focus_code;
	uchar *change_code;
	uchar *keypress_code;
	uchar *keyup_code;
	uchar *keydown_code;
#else
	char dummy;
#endif
};

/*
 * warning: if you add more additional file stuctures, you must
 * set RQ upcalls correctly
 */

struct additional_files {
	int refcount;
	struct list_head af;	/* struct additional_file */
};

struct additional_file {
	struct additional_file *next;
	struct additional_file *prev;
	struct object_request *rq;
	tcount use_tag;
	int need_reparse;
	uchar url[1];
};

#ifdef G
struct image_refresh {
	struct image_refresh *next;
	struct image_refresh *prev;
	struct g_object *img;
	ttime t;
};
#endif

struct f_data {
	struct f_data *next;
	struct f_data *prev;
	struct session *ses;
	struct f_data_c *fd;
	struct object_request *rq;
	tcount use_tag;
	struct additional_files *af;
	struct document_options opt;
	uchar *title;
	int cp, ass;
	int x, y; /* size of document */
	ttime time_to_get;
	ttime time_to_draw;
	struct frameset_desc *frame_desc;
	int frame_desc_link;	/* if != 0, do not free frame_desc because it is link */

	/* text only */
	int bg;
	struct line *data;
	struct link *links;
	int nlinks;
	struct link **lines1;
	struct link **lines2;
	struct list_head nodes;		/* struct node */
	struct search *search;
	int nsearch;
	struct search **slines1;
	struct search **slines2;

	struct list_head forms;		/* struct form_control */
	struct list_head tags;		/* struct tag */

	int are_there_scripts;
	uchar *script_href_base;

	uchar *refresh;
	int refresh_seconds;

	struct js_document_description *js_doc;
	int uncacheable;	/* cannot be cached - either created from source modified by document.write or modified by javascript */

	/* graphics only */
#ifdef G
	struct g_object *root;
	struct g_object *locked_on;
	int hsb;
	int vsb;
	int hsbsize;
	int vsbsize;

	uchar *srch_string;
	int srch_string_size;

	uchar *last_search;
	int last_search_len;
	int *search_positions;
	int n_search_positions;
	struct list_head images;	/* list of all images in this f_data */
	int n_images;	/* pocet obrazku (tim se obrazky taky identifikujou), po kazdem pridani obrazku se zvedne o 1 */

	struct list_head image_refresh;
#endif
};

struct view_state {
	int refcount;
	
	int view_pos;
	int view_posx;
	int current_link;	/* platny jen kdyz je <f_data->n_links */
	int frame_pos;
	int plain;
	struct form_state *form_info;
	int form_info_len;
	/*struct f_data_c *f;*/
	/*uchar url[1];*/
#ifdef G
	int g_display_link;
#endif
};

struct f_data_c {
	struct f_data_c *next;
	struct f_data_c *prev;
	struct f_data_c *parent;
	struct session *ses;
	struct location *loc;
	struct view_state *vs;
	struct f_data *f_data;
	int xw, yw; /* size of window */
	int xp, yp; /* pos of window on screen */
	int xl, yl; /* last pos of view in window */
	struct link_bg *link_bg;
	int link_bg_n;
	int depth;

	struct object_request *rq;
	uchar *goto_position;
	struct additional_files *af;

	struct list_head subframes;	/* struct f_data_c */

	ttime next_update;
	int done;
	int parsed_done;
	int script_t;	/* offset of next script to execute */

	int active;	/* temporary, for draw_doc */

	long id;	/* unique document identification for javascript */

	int marginwidth, marginheight;

	struct js_state *js;

	int image_timer;

	int refresh_timer;

#ifdef JS
	uchar *onload_frameset_code;
#endif
};

struct location {
	struct location *next;
	struct location *prev;
	struct location *parent;
	uchar *name;	/* frame name */
	uchar *url;
	uchar *prev_url;   /* allocated string with referrer */
	struct list_head subframes;	/* struct location */
	struct view_state *vs;
};

#define WTD_NO		0
#define WTD_FORWARD	1
#define WTD_IMGMAP	2
#define WTD_RELOAD	3
#define WTD_BACK	4

#define cur_loc(x) ((struct location *)((x)->history.next))

struct kbdprefix {
	int rep;
	int rep_num;
	int prefix;
};

struct download {
	struct download *next;
	struct download *prev;
	uchar *url;
	struct status stat;
	uchar *file;
	int last_pos;
	int handle;
	int redirect_cnt;
	uchar *prog;
	int prog_flags;
	time_t remotetime;
	struct session *ses;
	struct window *win;
	struct window *ask;
};

extern struct list_head downloads;

struct session {
	struct session *next;
	struct session *prev;
	struct list_head history;	/* struct location */
	struct terminal *term;
	struct window *win;
	int id;
	uchar *st;		/* status line string */
	uchar *default_status;	/* default value of the status line */
	struct f_data_c *screen;
	struct object_request *rq;
	void (*wtd)(struct session *);
	uchar *wtd_target;
	struct f_data_c *wtd_target_base;
	uchar *wanted_framename;
	int wtd_refresh;
	uchar *goto_position;
	struct document_setup ds;
	struct kbdprefix kbdprefix;
	int reloadlevel;
	struct object_request *tq;
	uchar *tq_prog;
	int tq_prog_flags;
	uchar *dn_url;
	uchar *search_word;
	uchar *last_search_word;
	int search_direction;
	int exit_query;
	struct list_head format_cache;	/* struct f_data */

	uchar *imgmap_href_base;
	uchar *imgmap_target_base;

	uchar *defered_url;
	uchar *defered_target;
	struct f_data_c *defered_target_base;
	int defered_data;	/* for submit: form number, jinak -1 */

	int locked_link;	/* for graphics - when link is locked on FIELD/AREA */

#ifdef G
	int scrolling;
	int scrolltype;
	int scrolloff;

	int back_size;
#endif
};

int get_file(struct object_request *o, uchar **start, uchar **end);

int f_is_finished(struct f_data *f);
void reinit_f_data_c(struct f_data_c *);
long formatted_info(int);
void init_fcache(void);
void html_interpret(struct f_data_c *);
void html_interpret_recursive(struct f_data_c *);
void fd_loaded(struct object_request *, struct f_data_c *);

extern struct list_head sessions;

time_t parse_http_date(char *);
uchar *encode_url(uchar *);
uchar *decode_url(uchar *);
uchar *subst_file(uchar *, uchar *);
int are_there_downloads(void);
void free_strerror_buf(void);
uchar *get_err_msg(int);
void print_screen_status(struct session *);
void change_screen_status(struct session *);
void print_error_dialog(struct session *, struct status *, uchar *);
void start_download(struct session *, uchar *);
void abort_all_downloads(void);
void display_download(struct terminal *, struct download *, struct session *);
int create_download_file(struct terminal *, uchar *, int);
void *create_session_info(int, uchar *, uchar *, int *);
void win_func(struct window *, struct event *, int);
void goto_url_f(struct session *, void (*)(struct session *), uchar *, uchar *, struct f_data_c *, int, int, int, int);
void goto_url(struct session *, uchar *);
void goto_url_not_from_dialog(struct session *, uchar *);
void goto_imgmap(struct session *ses, uchar *url, uchar *href, uchar *target);
void map_selected(struct terminal *term, struct link_def *ld, struct session *ses);
void go_back(struct session *);
void go_backwards(struct terminal *term, void *psteps, struct session *ses);
void reload(struct session *, int);
void destroy_session(struct session *);
void destroy_location(struct location *);
void ses_destroy_defered_jump(struct session *ses);
struct f_data_c *find_frame(struct session *ses, uchar *target, struct f_data_c *base);


/* Information about the current document */
uchar *get_current_url(struct session *, uchar *, size_t);
uchar *get_current_title(struct session *, uchar *, size_t);

uchar *get_current_link_url(struct session *, uchar *, size_t);
uchar *get_form_url(struct session *ses, struct f_data_c *f, struct form_control *form, int *onsubmit);

/* js.c */

struct javascript_context *js_create_context(void *, long);
void js_destroy_context(struct javascript_context *);
void js_execute_code(struct javascript_context *, uchar *, int, void (*)(void *));

/* jsint.c */

#define JS_OBJ_MASK 255
#define JS_OBJ_MASK_SIZE 8

#define JS_OBJ_T_UNKNOWN 0
#define JS_OBJ_T_DOCUMENT 1
#define JS_OBJ_T_FRAME 2	/* document a frame se tvari pro mne stejne  --Brain */
#define JS_OBJ_T_LINK 3
#define JS_OBJ_T_FORM 4
#define JS_OBJ_T_ANCHOR 5
#define JS_OBJ_T_IMAGE 6
/* form elements */
#define JS_OBJ_T_TEXT 7
#define JS_OBJ_T_PASSWORD 8
#define JS_OBJ_T_TEXTAREA 9
#define JS_OBJ_T_CHECKBOX 10
#define JS_OBJ_T_RADIO 11
#define JS_OBJ_T_SELECT 12
#define JS_OBJ_T_SUBMIT 13
#define JS_OBJ_T_RESET 14
#define JS_OBJ_T_HIDDEN 15
#define JS_OBJ_T_BUTTON 16


extern struct history js_get_string_history;
extern int js_manual_confirmation;

struct js_state {
	struct javascript_context *ctx;	/* kontext beziciho javascriptu??? */
	struct list_head queue;		/* struct js_request - list of javascripts to run */
	struct js_request *active;	/* request is running */
	uchar *src;		/* zdrojak beziciho javascriptu??? */	/* mikulas: ne. to je zdrojak stranky */
	int srclen;
	int wrote;
};

struct js_document_description {
	/* Pro Martina: TADY pridat nejake polozky popisujici dokument
	- jako treba jake tam jsou polozky formulare, jake obrazky, jake
	linky apod. Neni tady obsah tech polozek, jenom popis, zda
	existuji.

	vyroba struktury je v js_upcall_get_document_description
	ruseni je v jsint_destroy_document_description */

	int prazdnapolozkaabytadynecobylo;
};


/* funkce js_get_select_options vraci pole s temito polozkami */
struct js_select_item{
	/* index je poradi v poli, ktere vratim, takze se tu nemusi skladovat */
	int default_selected;
	int selected;
	uchar *text; 	/* text, ktery se zobrazuje */
	uchar *value; 	/* value, ktera se posila */
};

struct fax_me_tender_string{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	uchar *string;
};

struct fax_me_tender_int_string{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	signed int num;
	uchar *string;
};

struct fax_me_tender_string_2_longy{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	uchar *string;
	long doc_id,obj_id;
};

struct fax_me_tender_2_stringy{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	uchar *string1;
	uchar *string2;
};

struct fax_me_tender_nothing{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
};

void javascript_func(struct session *ses, uchar *code);
void jsint_execute_code(struct f_data_c *, uchar *, int, int, int, int);
void jsint_destroy(struct f_data_c *);
void jsint_run_queue(struct f_data_c *);
int jsint_get_source(struct f_data_c *, uchar **, uchar **);
void jsint_scan_script_tags(struct f_data_c *);
void jsint_destroy_document_description(struct f_data *);
long *jsint_resolve(void *context, long obj_id, char *takhle_tomu_u_nas_nadavame,int *n_items);
int jsint_object_type(long);
void jsint_set_cookies(struct f_data_c *fd, int final_flush);
struct f_data_c *jsint_find_document(long doc_id);

struct js_document_description *js_upcall_get_document_description(void *, long);
void js_upcall_document_write(void *p, uchar *str, int len);
void js_upcall_alert(void * struct_fax_me_tender_string);
uchar *js_upcall_get_title(void *data);
void js_upcall_set_title(void *data, uchar *title);
uchar *js_upcall_get_location(void *data);
uchar *js_upcall_get_useragent(void *data);
void js_upcall_confirm(void *struct_fax_me_tender_string);
void js_upcall_get_string(void *data);
uchar *js_upcall_get_referrer(void *data);
uchar *js_upcall_get_appname(void);
uchar *js_upcall_get_appcodename(void);
uchar *js_upcall_get_appversion(void);
long js_upcall_get_document_id(void *data);
long js_upcall_get_window_id(void *data);
void js_upcall_close_window(void *struct_fax_me_tender_nothing);
uchar *js_upcall_document_last_modified(void *data, long document_id);
uchar *js_upcall_get_window_name(void *data);
void js_upcall_clear_window(void *);
long *js_upcall_get_links(void *data, long document_id, int *len);
uchar *js_upcall_get_link_target(void *data, long document_id, long link_id);
long *js_upcall_get_forms(void *data, long document_id, int *len);
uchar *js_upcall_get_form_action(void *data, long document_id, long form_id);
uchar *js_upcall_get_form_target(void *data, long document_id, long form_id);
uchar *js_upcall_get_form_method(void *data, long document_id, long form_id);
uchar *js_upcall_get_form_encoding(void *data, long document_id, long form_id);
uchar *js_upcall_get_location_protocol(void *data);
uchar *js_upcall_get_location_port(void *data);
uchar *js_upcall_get_location_hostname(void *data);
uchar *js_upcall_get_location_host(void *data);
uchar *js_upcall_get_location_pathname(void *data);
uchar *js_upcall_get_location_search(void *data);
uchar *js_upcall_get_location_hash(void *data);
long *js_upcall_get_form_elements(void *data, long document_id, long form_id, int *len);
long *js_upcall_get_anchors(void *hej_Hombre, long document_id, int *len);
int js_upcall_get_checkbox_radio_checked(void *smirak, long document_id, long radio_tv_id);
void js_upcall_set_checkbox_radio_checked(void *smirak, long document_id, long radio_tv_id, int value);
int js_upcall_get_checkbox_radio_default_checked(void *smirak, long document_id, long radio_tv_id);
void js_upcall_set_checkbox_radio_default_checked(void *smirak, long document_id, long radio_tv_id, int value);
uchar *js_upcall_get_form_element_name(void *smirak, long document_id, long ksunt_id);
void js_upcall_set_form_element_name(void *smirak, long document_id, long ksunt_id, uchar *name);
uchar *js_upcall_get_form_element_default_value(void *smirak, long document_id, long ksunt_id);
void js_upcall_set_form_element_default_value(void *smirak, long document_id, long ksunt_id, uchar *name);
uchar *js_upcall_get_form_element_value(void *smirak, long document_id, long ksunt_id);
void js_upcall_set_form_element_value(void *smirak, long document_id, long ksunt_id, uchar *name);
void js_upcall_click(void *smirak, long document_id, long elem_id);
void js_upcall_focus(void *smirak, long document_id, long elem_id);
void js_upcall_blur(void *smirak, long document_id, long elem_id);
void js_upcall_submit(void *bidak, long document_id, long form_id);
void js_upcall_reset(void *bidak, long document_id, long form_id);
int js_upcall_get_radio_length(void *smirak, long document_id, long radio_id); /* radio.length */
int js_upcall_get_select_length(void *smirak, long document_id, long select_id); /* select.length */
int js_upcall_get_select_index(void *smirak, long document_id, long select_id); /* select.selectedIndex */
struct js_select_item* js_upcall_get_select_options(void *smirak, long document_id, long select_id, int *n);
void js_upcall_goto_url(void* struct_fax_me_tender_string);
int js_upcall_get_history_length(void *context);
void js_upcall_goto_history(void * data);
void js_upcall_set_default_status(void *context, uchar *tak_se_ukaz_Kolbene);
uchar* js_upcall_get_default_status(void *context);
void js_upcall_set_status(void *context, uchar *tak_se_ukaz_Kolbene);
uchar* js_upcall_get_status(void *context);
uchar * js_upcall_get_cookies(void *context);
long *js_upcall_get_images(void *smirak, long document_id, int *len);
long * js_upcall_get_all(void *context, long document_id, int *len);
int js_upcall_get_image_width(void *smirak, long document_id, long image_id);
int js_upcall_get_image_height(void *smirak, long document_id, long image_id);
int js_upcall_get_image_border(void *smirak, long document_id, long image_id);
int js_upcall_get_image_vspace(void *smirak, long document_id, long image_id);
int js_upcall_get_image_hspace(void *smirak, long document_id, long image_id);
uchar * js_upcall_get_image_name(void *smirak, long document_id, long image_id);
uchar * js_upcall_get_image_alt(void *smirak, long document_id, long image_id);
void js_upcall_set_image_name(void *smirak, long document_id, long image_id, uchar *name);
void js_upcall_set_image_alt(void *smirak, long document_id, long image_id, uchar *alt);
uchar * js_upcall_get_image_src(void *smirak, long document_id, long image_id);
void js_upcall_set_image_src(void *chuligane);
int js_upcall_image_complete(void *smirak, long document_id, long image_id);
long js_upcall_get_parent(void *smirak, long frame_id);
long js_upcall_get_frame_top(void *smirak, long frame_id);
long * js_upcall_get_subframes(void *smirak, long frame_id, int *count);


void js_downcall_vezmi_true(void *context);
void js_downcall_vezmi_false(void *context);
void js_downcall_vezmi_null(void *context);
void js_downcall_game_over(void *context);
void js_downcall_quiet_game_over(void *context);
void js_downcall_vezmi_int(void *context, int i);
void js_downcall_vezmi_float(void*context,double f);
/*void js_downcall_vezmi_float(void *context, float f);*/
void js_downcall_vezmi_string(void *context, uchar *string);

/* bfu.c */

extern unsigned long G_BFU_FG_COLOR, G_BFU_BG_COLOR, G_SCROLL_BAR_AREA_COLOR, G_SCROLL_BAR_BAR_COLOR, G_SCROLL_BAR_FRAME_COLOR;
extern struct style *bfu_style_wb, *bfu_style_bw, *bfu_style_wb_b, *bfu_style_bw_u, *bfu_style_bw_mono, *bfu_style_wb_mono, *bfu_style_wb_mono_u;
extern long bfu_bg_color, bfu_fg_color;

struct memory_list {
	int n;
	void *p[1];
};

struct memory_list *getml(void *, ...);
void add_to_ml(struct memory_list **, ...);
void freeml(struct memory_list *);

void iinit_bfu(void);
void init_bfu(void);
void shutdown_bfu(void);

#define DIALOG_LB	gf_val(DIALOG_LEFT_BORDER + DIALOG_LEFT_INNER_BORDER + 1, G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE + 1 + G_DIALOG_LEFT_INNER_BORDER)
#define DIALOG_TB	gf_val(DIALOG_TOP_BORDER + DIALOG_TOP_INNER_BORDER + 1, G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE + 1 + G_DIALOG_TOP_INNER_BORDER)

#define MENU_FUNC (void (*)(struct terminal *, void *, void *))

extern uchar m_bar;

#define M_BAR	(&m_bar)

struct menu_item {
	uchar *text;
	uchar *rtext;
	uchar *hotkey;
	void (*func)(struct terminal *, void *, void *);
	void *data;
	int in_m;
	int free_i;
};

struct menu {
	int selected;
	int view;
	int nview;
	int xp, yp;
	int x, y, xw, yw;
	int ni;
	void *data;
	struct window *win;
	struct menu_item *items;
#ifdef G
	uchar **hktxt1;
	uchar **hktxt2;
	uchar **hktxt3;
	int xl1, yl1, xl2, yl2;
#endif
};

struct mainmenu {
	int selected;
	int sp;
	int ni;
	void *data;
	struct window *win;
	struct menu_item *items;
#ifdef G
	int xl1, yl1, xl2, yl2;
#endif
};

struct history_item {
	struct history_item *next;
	struct history_item *prev;
	uchar d[1];
};

struct history {
	int n;
	struct list_head items;
};

#define D_END		0
#define D_CHECKBOX	1
#define D_FIELD		2
#define D_FIELD_PASS	3
#define D_BUTTON	4

#define B_ENTER		1
#define B_ESC		2

struct dialog_item_data;
struct dialog_data;

struct dialog_item {
	int type;
	int gid, gnum; /* for buttons: gid - flags B_XXX */	/* for fields: min/max */ /* for box: gid is box height */
	int (*fn)(struct dialog_data *, struct dialog_item_data *);
	struct history *history;
	int dlen;
	uchar *data;
	void *udata; /* for box: holds list */
	uchar *text;
};

struct dialog_item_data {
	int x, y, l;
	int vpos, cpos;
	int checked;
	struct dialog_item *item;
	struct list_head history;
	struct history_item *cur_hist;
	uchar *cdata;
};

#define	EVENT_PROCESSED		0
#define EVENT_NOT_PROCESSED	1

struct dialog {
	uchar *title;
	void (*fn)(struct dialog_data *);
	int (*handle_event)(struct dialog_data *, struct event *);
	void (*abort)(struct dialog_data *);
	void *udata;
	void *udata2;
	int align;
	void (*refresh)(void *);
	void *refresh_data;
	struct dialog_item items[1];
};

struct dialog_data {
	struct window *win;
	struct dialog *dlg;
	int x, y, xw, yw;
	int n;
	int selected;
	struct memory_list *ml;
#ifdef G
	struct rect_set *s;
	struct rect r;
	struct rect rr;
#endif
	struct dialog_item_data items[1];
};

struct menu_item *new_menu(int);
void add_to_menu(struct menu_item **, uchar *, uchar *, uchar *, void (*)(struct terminal *, void *, void *), void *, int);
void do_menu(struct terminal *, struct menu_item *, void *);
void do_menu_selected(struct terminal *, struct menu_item *, void *, int);
void do_mainmenu(struct terminal *, struct menu_item *, void *, int);
void do_dialog(struct terminal *, struct dialog *, struct memory_list *);
int check_number(struct dialog_data *, struct dialog_item_data *);
int check_hex_number(struct dialog_data *, struct dialog_item_data *);
int check_float(struct dialog_data *, struct dialog_item_data *);
int check_nonempty(struct dialog_data *, struct dialog_item_data *);
void max_text_width(struct terminal *, uchar *, int *, int);
void min_text_width(struct terminal *, uchar *, int *, int);
void dlg_format_text(struct dialog_data *, struct terminal *, uchar *, int, int *, int, int *, int, int);
void max_buttons_width(struct terminal *, struct dialog_item_data *, int, int *);
void min_buttons_width(struct terminal *, struct dialog_item_data *, int, int *);
void dlg_format_buttons(struct dialog_data *, struct terminal *, struct dialog_item_data *, int, int, int *, int, int *, int);
void checkboxes_width(struct terminal *, uchar **, int *, void (*)(struct terminal *, uchar *, int *, int));
void dlg_format_checkbox(struct dialog_data *, struct terminal *, struct dialog_item_data *, int, int *, int, int *, uchar *);
void dlg_format_checkboxes(struct dialog_data *, struct terminal *, struct dialog_item_data *, int, int, int *, int, int *, uchar **);
void dlg_format_field(struct dialog_data *, struct terminal *, struct dialog_item_data *, int, int *, int, int *, int);
void max_group_width(struct terminal *, uchar **, struct dialog_item_data *, int, int *);
void min_group_width(struct terminal *, uchar **, struct dialog_item_data *, int, int *);
void dlg_format_group(struct dialog_data *, struct terminal *, uchar **, struct dialog_item_data *, int, int, int *, int, int *);
void dlg_format_box(struct terminal *, struct terminal *, struct dialog_item_data *, int, int *, int, int *, int);
void checkbox_list_fn(struct dialog_data *);
void group_fn(struct dialog_data *);
void center_dlg(struct dialog_data *);
void draw_dlg(struct dialog_data *);
void display_dlg_item(struct dialog_data *, struct dialog_item_data *, int);
int check_dialog(struct dialog_data *);
void get_dialog_data(struct dialog_data *);
int ok_dialog(struct dialog_data *, struct dialog_item_data *);
int cancel_dialog(struct dialog_data *, struct dialog_item_data *);
void msg_box(struct terminal *, struct memory_list *, uchar *, int, /*uchar *, void *, int,*/ ...);
/* msg_box arguments:
 *		terminal,
 *		blocks to free,
 *		title,
 *		alignment (and/or optional AL_EXTD_TEXT),
 *		string (or optional several strings followed with NULL),
 *		data for function,
 *		number of buttons,
 *		button title, function, hotkey,
 *		... other buttons
 */
void input_field_fn(struct dialog_data *);
void input_field(struct terminal *, struct memory_list *, uchar *, uchar *, uchar *, uchar *, void *, struct history *, int, uchar *, int, int, int (*)(struct dialog_data *, struct dialog_item_data *), void (*)(void *, uchar *), void (*)(void *));
/* input_field arguments:
 * 		terminal,
 * 		blocks to free,
 * 		title,
 * 		question,
 * 		OK button text,
 * 		CANCEL button text,
 *		data for functions,
 *		history,
 *		length,
 *		string to fill the dialog with,
 *		minimal length,
 *		maximal length,
 *		check_function,
 *		ok function,
 *		cancel function
 */
void add_to_history(struct history *, uchar *);

void box_sel_move(struct dialog_item_data *, int ); 
void show_dlg_item_box(struct dialog_data *, struct dialog_item_data *);
void box_sel_set_visible(struct dialog_item_data *, int ); 

/* internal functions: do not call */
void menu_func(struct window *, struct event *, int);
void mainmenu_func(struct window *, struct event *, int);
void dialog_func(struct window *, struct event *, int);

/* menu.c */

void activate_bfu_technology(struct session *, int);
void dialog_goto_url(struct session *ses, char *url);
void dialog_save_url(struct session *ses);
void free_history_lists(void);
void query_file(struct session *, uchar *, void (*)(struct session *, uchar *), void (*)(struct session *));
void search_dlg(struct session *, struct f_data_c *, int);
void search_back_dlg(struct session *, struct f_data_c *, int);
void exit_prog(struct terminal *, void *, struct session *);
void really_exit_prog(struct session *ses);
void query_exit(struct session *ses);

#ifdef G

extern int gamma_stamp;
extern int display_optimize;	/*0=CRT, 1=LCD RGB, 2=LCD BGR */

#endif

/* charsets.c */

#include "codepage.h"

struct conv_table {
	int t;
	union {
		uchar *str;
		struct conv_table *tbl;
	} u;
};

struct conv_table *get_translation_table(int, int);
uchar *get_entity_string(uchar *, int, int);
uchar *convert_string(struct conv_table *, uchar *, int, struct document_options *);
int get_cp_index(uchar *);
uchar *get_cp_name(int);
uchar *get_cp_mime_name(int);
int is_cp_special(int);
void free_conv_table(void);
uchar *encode_utf_8(int);
int cp2u(uchar, int);

#ifdef G
int get_utf_8(uchar **p);
extern int utf8_2_uni_table[0x200];
#define GET_UTF_8(s, c)	do {if ((uchar)(s)[0] < 0x80) (c) = (s)++[0]; else if (((c) = utf8_2_uni_table[((uchar)(s)[0] << 2) + ((uchar)(s)[1] >> 6) - 0x200])) (c) += (uchar)(s)[1] & 0x3f, (s) += 2; else (c) = get_utf_8(&(s));} while (0)
#define FWD_UTF_8(s) do {if ((uchar)(s)[0] < 0x80) (s)++; else get_utf_8(&(s));} while (0)
#define BACK_UTF_8(p, b) do {while ((p) > (b)) {(p)--; if ((*(p) & 0xc0) != 0x80) break; }} while (0)
#endif

/* view.c */

uchar *utf8_add(uchar *t, int i);
int utf8_diff(uchar *t2, uchar *t1);

extern int ismap_link, ismap_x, ismap_y;

void frm_download(struct session *, struct f_data_c *);
void frm_download_image(struct session *, struct f_data_c *);
void frm_view_image(struct session *, struct f_data_c *);
struct form_state *find_form_state(struct f_data_c *, struct form_control *);
void fixup_select_state(struct form_control *fc, struct form_state *fs);
int enter(struct session *ses, struct f_data_c *f, int a);
int field_op(struct session *ses, struct f_data_c *f, struct link *l, struct event *ev, int rep);
int _area_cursor(struct form_control *form, struct form_state *fs);

struct line_info {
	uchar *st;
	uchar *en;
};

struct line_info *format_text(uchar *text, int width, int wrap);

int can_open_in_new(struct terminal *);
void open_in_new_window(struct terminal *, void (*)(struct terminal *, void (*)(struct terminal *, uchar *, uchar *), struct session *ses), struct session *);
void send_open_new_xterm(struct terminal *, void (*)(struct terminal *, uchar *, uchar *), struct session *);
void destroy_fc(struct form_control *);
void sort_links(struct f_data *);
struct view_state *create_vs(void);
void destroy_vs(struct view_state *);
int dump_to_file(struct f_data *, int);
void draw_doc(struct terminal *t, struct f_data_c *scr);
void draw_formatted(struct session *);
void draw_fd(struct f_data_c *);
void send_event(struct session *, struct event *);
void link_menu(struct terminal *, void *, struct session *);
void save_as(struct terminal *, void *, struct session *);
void save_url(struct session *, uchar *);
void menu_save_formatted(struct terminal *, void *, struct session *);
void copy_url_location(struct terminal *, void *, struct session *);
void selected_item(struct terminal *, void *, struct session *);
void toggle(struct session *, struct f_data_c *, int);
void do_for_frame(struct session *, void (*)(struct session *, struct f_data_c *, int), int);
int get_current_state(struct session *);
uchar *print_current_link(struct session *);
uchar *print_current_title(struct session *);
void loc_msg(struct terminal *, struct location *, struct f_data_c *);
void state_msg(struct session *);
void head_msg(struct session *);
void search_for(struct session *, uchar *);
void search_for_back(struct session *, uchar *);
void find_next(struct session *, struct f_data_c *, int);
void find_next_back(struct session *, struct f_data_c *, int);
void set_frame(struct session *, struct f_data_c *, int);
struct f_data_c *current_frame(struct session *);
void reset_form(struct f_data_c *f, int form_num);
void set_textarea(struct session *, struct f_data_c *, int);

void copy_js_event_spec(struct js_event_spec **, struct js_event_spec *);
void free_js_event_spec(struct js_event_spec *);
void create_js_event_spec(struct js_event_spec **);
int compare_js_event_spec(struct js_event_spec *, struct js_event_spec *);
uchar *print_js_event_spec(struct js_event_spec *);

/* font_include.c */

/* gif.c */

#ifdef G
#include "png.h"
struct gif_decoder;
struct png_decoder{
	png_structp png_ptr;
	png_infop info_ptr;
};

void gif_destroy_decoder(struct cached_image *);
void gif_start(struct cached_image *goi);
void gif_restart(uchar *data, int length);

void xbm_start(struct cached_image *goi);
void xbm_restart(struct cached_image *goi, uchar *data, int length);

#endif

/* png.c */

#ifdef G

void png_start(struct cached_image *cimg);
void png_restart(struct cached_image *cimg, uchar *data, int length);

#endif /* #ifdef G */

/* tiff.c */
#ifdef G
#ifdef HAVE_TIFF
struct tiff_decoder{
	uchar *tiff_data; /* undecoded data */
	int tiff_size;	/* size of undecoded file */
	int tiff_pos;
	int tiff_open;   /* 1 if tiff was open, means: tiff_data, tiff_size and tiff_pos are valid */
};

void tiff_start(struct cached_image *cimg);
void tiff_restart(struct cached_image *cimg, uchar *data, int length);
void tiff_finish(struct cached_image *cimg);
#endif /* #ifdef HAVE_TIFF */
#endif /* #ifdef G */

/* img.c */

#ifdef G

struct image_description {
	uchar *url;		/* url=completed url */
	int xsize, ysize;		/* -1 --- unknown size */
	int link_num;
	int link_order;
	uchar *name;
	uchar *alt;
	uchar *src;		/* reflects the src attribute */
	int border, vspace, hspace;
	int ismap;
	int insert_flag;		/* pokud je 1, ma se vlozit do seznamu obrazku ve f_data */

	uchar *usemap;
};

struct gif_table_entry
{
 uchar end_char;
 uchar garbage; /* This has nothing common to do with code table:
                           this is temporarily used for reverting strings :-) */
 short pointer; /* points onto another entry in table, number 0...4095.
                   number -1 means it end there, the end_char is the last
		   number -2 means that this entry is no filled in yet.
                */
		   
};

struct gif_decoder{
	uchar *color_map; /* NULL if no color map, otherwise a block of 768 bytes, red, green, blue,
				     in sRGB, describing color slots 0...255. */
	int state; /* State of the automatus finitus recognizing the GIF
	            * format.  0 is initial. */
	/* Image width, height, bits per pixel, bytes per line, number of bit planes */
	int im_width;
	int im_height;
	int im_bpp; /* Bits per pixel (in codestream) */
	int code_size;
	int initial_code_size;
	int remains; /* Used to skip unwanted blocks in raster data */
	struct gif_table_entry table[4096]; /* NULL when not present */
	uchar *actual_line; /* Points to actual line in goi->buffer */
	uchar tbuf[16]; /* For remembering headers and similar things. */
	int tlen; /* 0 in the beginning . tbuf length */
	int xoff, yoff;
	int interl_dist;
	int bits_read; /* How many bits are already read from the symbol
	                * Currently being read */
	int last_code; /* This is somehow used in the decompression algorithm */
	int read_code;
	int CC;
	int EOI;
	int table_pos;
	int first_code;
	int transparent;
};

struct decoded_image;
#endif
extern int dither_images;
#ifdef G
extern int end_callback_hit;
extern struct g_object_image *global_goi;
extern struct cached_image *global_cimg;

/* Below are internal functions shared with imgcache.c, gif.c, and xbm.c */
void img_release_decoded_image(struct decoded_image *);
void header_dimensions_known(struct cached_image *cimg);
void img_end(struct cached_image *cimg);
void compute_background_8(uchar *rgb, struct cached_image *cimg);
void buffer_to_bitmap_incremental(struct cached_image *cimg
	,uchar *buffer, int height, int yoff, int *dregs, int use_strip);

/* Below is external interface provided by img.c */
struct g_part;
void img_draw_decoded_image(struct graphics_device *, struct decoded_image *img, int, int, int, int, int, int);
struct g_object_image *insert_image(struct g_part *p, struct image_description *im);
void change_image (struct g_object_image *goi, uchar *url, uchar *src, struct f_data *fdata);
void img_destruct_cached_image(struct cached_image *img);

#endif

/* jpeg.c */
#ifdef G
#ifdef HAVE_JPEG
struct jpg_decoder{
	struct jpeg_decompress_struct *cinfo;
	struct jerr_struct *jerr;
	int state; /* 0: header 1: start 2: scanlines 3: end */
	int skip_bytes;
	uchar *jdata;
	uchar *scanlines[16]; 
	/* The first is either allocate or is NULL.
         *
	 * If the image is grayscale, as soon as the width is
	 * known, scanlines[0] is allocated and scanlines[1]
	 * throught scanlines[15] are pointers inside the block
	 * allocated into scanlines[0]. So do not call mem_free
	 * on scanlines[1] to scanlines [15] unless you desire a
	 * segfault.
	 *
         * If the image is color, nothing is allocated and
	 * they are filled with 16 consecutive rows (or less
         * if there are less of them), the function is called
         * and as soon as it returns the [0] is immediately
         * set to NULL.
	 */	 
};

/* Functions exported by jpeg.c for higher layers */
void jpeg_start(struct cached_image *cimg);
void jpeg_restart(struct cached_image *cimg, uchar *data, int length);

#endif /* #ifdef HAVE_JPEG */
#endif /* #ifdef G */

/* imgcache.c */

#ifdef G

void init_imgcache(void);
int imgcache_info(int type);
struct cached_image *find_cached_image(int bg, uchar *url, int xw, int
		yw, int scale, int aspect);
void add_image_to_cache(struct cached_image *ci);

#endif

/* view_gr.c */

#ifdef G

void g_release_background(struct background *bg);
void g_draw_background(struct graphics_device *dev, struct background *bg, int x, int y, int xw, int yw);
int g_forward_mouse(struct f_data_c *fd, struct g_object *a, int x, int y, int b);

void draw_vscroll_bar(struct graphics_device *dev, int x, int y, int yw, int total, int view, int pos);
void draw_hscroll_bar(struct graphics_device *dev, int x, int y, int xw, int total, int view, int pos);
void get_scrollbar_pos(int dsize, int total, int vsize, int vpos, int *start, int *end);


void get_parents(struct f_data *f, struct g_object *a);
void get_object_pos(struct g_object *o, int *x, int *y);

void g_dummy_mouse(struct f_data_c *, struct g_object *, int, int, int);
void g_text_mouse(struct f_data_c *, struct g_object_text *, int, int, int);
void g_line_mouse(struct f_data_c *, struct g_object_line *, int, int, int);
void g_area_mouse(struct f_data_c *, struct g_object_area *, int, int, int);

void g_dummy_draw(struct f_data_c *, struct g_object *, int, int);
void g_text_draw(struct f_data_c *, struct g_object_text *, int, int);
void g_line_draw(struct f_data_c *, struct g_object_line *, int, int);
void g_area_draw(struct f_data_c *, struct g_object_area *, int, int);

void g_tag_destruct(struct g_object *);
void g_text_destruct(struct g_object_text *);
void g_line_destruct(struct g_object_line *);
void g_area_destruct(struct g_object_area *);

void g_line_get_list(struct g_object_line *, void (*)(struct g_object *parent, struct g_object *child));
void g_area_get_list(struct g_object_area *, void (*)(struct g_object *parent, struct g_object *child));

void draw_one_object(struct f_data_c *fd, struct g_object *o);
void draw_title(struct f_data_c *f);
void draw_graphical_doc(struct terminal *t, struct f_data_c *scr, int active);
int g_frame_ev(struct session *ses, struct f_data_c *fd, struct event *ev);
void g_find_next(struct f_data_c *f, int);

void init_grview(void);

#endif

/* html.c */

#define AT_BOLD		1
#define AT_ITALIC	2
#define AT_UNDERLINE	4
#define AT_FIXED	8
#define AT_GRAPHICS	16

#define AL_LEFT		0
#define AL_CENTER	1
#define AL_RIGHT	2
#define AL_BLOCK	3
#define AL_NO		4

#define AL_MASK		0x3f

#define AL_MONO		0x40
#define AL_EXTD_TEXT	0x80
	/* DIRTY! for backward compatibility with old menu code */

struct text_attrib_beginning {
	int attr;
	struct rgb fg;
	struct rgb bg;
	int fontsize;
};

struct text_attrib {
	int attr;
	struct rgb fg;
	struct rgb bg;
	int fontsize;
	uchar *fontface;
	uchar *link;
	uchar *target;
	uchar *image;
	struct js_event_spec *js_event;
	struct form_control *form;
	struct rgb clink;
	struct rgb vlink;
	uchar *href_base;
	uchar *target_base;
	uchar *select;
	int select_disabled;
};

#define P_NUMBER	1
#define P_alpha		2
#define P_ALPHA		3
#define P_roman		4
#define P_ROMAN		5
#define P_STAR		1
#define P_O		2
#define P_PLUS		3
#define P_LISTMASK	7
#define P_COMPACT	8

struct par_attrib {
	int align;
	int leftmargin;
	int rightmargin;
	int width;
	int list_level;
	unsigned list_number;
	int dd_margin;
	int flags;
	struct rgb bgcolor;
};

struct html_element {
	struct html_element *next;
	struct html_element *prev;
	struct text_attrib attr;
	struct par_attrib parattr;
	int invisible;
	uchar *name;
	int namelen;
	uchar *options;
	int linebreak;
	int dontkill;
	struct frameset_desc *frameset;
};

extern struct list_head html_stack;
extern int line_breax;

extern int html_format_changed;

extern uchar *startf;
extern uchar *eofff;

#define format_ (((struct html_element *)html_stack.next)->attr)
#define par_format (((struct html_element *)html_stack.next)->parattr)
#define html_top (*(struct html_element *)html_stack.next)

extern void *ff;
extern void (*put_chars_f)(void *, uchar *, int);
extern void (*line_break_f)(void *);
extern void (*init_f)(void *);
extern void *(*special_f)(void *, int, ...);

void ln_break(int, void (*)(void *), void *);
void put_chrs(uchar *, int, void (*)(void *, uchar *, int), void *);

extern int table_level;
extern int empty_format;

extern struct form form;
extern uchar *last_form_tag;
extern uchar *last_form_attr;
extern uchar *last_input_tag;

extern uchar *last_link;
extern uchar *last_image;
extern uchar *last_target;
extern struct form_control *last_form;
extern struct js_event_spec *last_js_event;
extern int js_fun_depth;
extern int js_memory_limit;

int parse_element(uchar *, uchar *, uchar **, int *, uchar **, uchar **);
uchar *get_attr_val(uchar *, uchar *);
int has_attr(uchar *, uchar *);
int get_num(uchar *, uchar *);
int get_width(uchar *, uchar *, int);
int get_color(uchar *, uchar *, struct rgb *);
int get_bgcolor(uchar *, struct rgb *);
void html_stack_dup(void);
void kill_html_stack_item(struct html_element *);
uchar *skip_comment(uchar *, uchar *);
void parse_html(uchar *, uchar *, void (*)(void *, uchar *, int), void (*)(void *), void *(*)(void *, int, ...), void *, uchar *);
int get_image_map(uchar *, uchar *, uchar *, uchar *a, struct menu_item **, struct memory_list **, uchar *, uchar *, int, int, int, int gfx);
void scan_http_equiv(uchar *, uchar *, uchar **, int *, uchar **, uchar **, uchar **);

int decode_color(uchar *, struct rgb *);

#define SP_TAG		0
#define SP_CONTROL	1
#define SP_TABLE	2
#define SP_USED		3
#define SP_FRAMESET	4
#define SP_FRAME	5
#define SP_SCRIPT	6
#define SP_IMAGE	7
#define SP_NOWRAP	8
#define SP_REFRESH	9

struct frameset_param {
	struct frameset_desc *parent;
	int x, y;
	int *xw, *yw;
};

struct frame_param {
	struct frameset_desc *parent;
	uchar *name;
	uchar *url;
	int marginwidth;
	int marginheight;
};

struct refresh_param {
	uchar *url;
	int time;
};

void free_menu(struct menu_item *);
void do_select_submenu(struct terminal *, struct menu_item *, struct session *);

void clr_white(uchar *name);
void clr_spaces(uchar *name);

/* html_r.c */

extern int g_ctrl_num;

extern struct conv_table *convert_table;

struct part {
	int x, y;
	int xp, yp;
	int xmax;
	int xa;
	int cx, cy;
	struct f_data *data;
	int bgcolor;
	uchar *spaces;
	int spl;
	int link_num;
	struct list_head uf;
};

#ifdef G
struct g_part {
	int x, y;
	int xmax;
	int cx, cy;
	struct g_object_area *root;
	struct g_object_line *line;
	struct g_object_text *text;
	int pending_text_len;
	struct wrap_struct w;
	struct style *current_style;
	int current_font_size;
	struct f_data *data;
	int link_num;
	struct list_head uf;
};
#endif

struct sizes {
	int xmin, xmax, y;
};

extern struct f_data *current_f_data;

void free_additional_files(struct additional_files **);
void free_frameset_desc(struct frameset_desc *);
struct frameset_desc *copy_frameset_desc(struct frameset_desc *);

struct f_data *init_formatted(struct document_options *);
void destroy_formatted(struct f_data *);

/* d_opt je podle Mikulase nedefinovany mimo html parser, tak to jinde nepouzivejte 
 *
 * -- Brain
 */
extern struct document_options dd_opt;	
extern struct document_options *d_opt;	
extern int last_link_to_move;
extern int margin;

int xxpand_line(struct part *, int, int);
int xxpand_lines(struct part *, int);
void xset_hchar(struct part *, int, int, unsigned);
void xset_hchars(struct part *, int, int, int, unsigned);
void align_line(struct part *, int);
void html_tag(struct f_data *, uchar *, int, int);
void process_script(struct f_data *, uchar *);
void html_process_refresh(struct f_data *, uchar *, int );

void free_table_cache(void);

int compare_opt(struct document_options *, struct document_options *);
void copy_opt(struct document_options *, struct document_options *);

struct link *new_link(struct f_data *);
struct conv_table *get_convert_table(uchar *, int, int, int *, int *, int);
extern int format_cache_entries;
void count_format_cache(void);
void delete_unused_format_cache_entries(void);
void format_cache_reactivate(struct f_data *);
struct part *format_html_part(uchar *, uchar *, int, int, int, struct f_data *, int, int, uchar *, int);
void really_format_html(struct cache_entry *, uchar *, uchar *, struct f_data *, int frame);
void get_search_data(struct f_data *);

struct frameset_desc *create_frameset(struct f_data *fda, struct frameset_param *fp);
void create_frame(struct frame_param *fp);

/* html_gr.c */

#ifdef G

void g_free_table_cache(void);

void release_image_map(struct image_map *map);
int is_in_area(struct map_area *a, int x, int y);

struct background *get_background(uchar *bg, uchar *bgcolor);

void g_x_extend_area(struct g_object_area *a, int width, int height);
struct g_part *g_format_html_part(uchar *, uchar *, int, int, int, uchar *, int, uchar *, uchar *, struct f_data *);
void g_release_part(struct g_part *);
int g_get_area_width(struct g_object_area *o);
void add_object(struct g_part *pp, struct g_object *o);
void add_object_to_line(struct g_part *pp, struct g_object_line **lp,
	struct g_object *go);
void flush_pending_text_to_line(struct g_part *p);
void flush_pending_line_to_obj(struct g_part *p, int minheight);

#endif

/* html_tbl.c */

void format_table(uchar *, uchar *, uchar *, uchar **, void *);

/* default.c */

#define MAX_STR_LEN	1024

extern int ggr;
extern uchar ggr_drv[MAX_STR_LEN];
extern uchar ggr_mode[MAX_STR_LEN];
extern uchar ggr_display[MAX_STR_LEN];

extern uchar default_target[MAX_STR_LEN];

uchar *parse_options(int, uchar *[]);
void init_home(void);
void load_config(void);
void write_config(struct terminal *);
void write_html_config(struct terminal *);
void end_config(void);

int load_url_history(void);
int save_url_history(void);

struct driver_param {
	struct driver_param *next;
	struct driver_param *prev;
	int codepage;
	uchar *param;
	uchar *shell;
	uchar name[1];
};

struct driver_param *get_driver_param(uchar *);

extern int anonymous;

extern uchar system_name[];

extern uchar *links_home;
extern int first_use;

extern int created_home;

extern int no_connect;
extern int base_session;
extern int force_html;

#define D_DUMP		1
#define D_SOURCE	2
extern int dmp;

extern int async_lookup;
extern int download_utime;
extern int max_connections;
extern int max_connections_to_host;
extern int max_tries;
extern int receive_timeout;
extern int unrestartable_receive_timeout;

extern struct document_setup dds;

extern int max_format_cache_entries;
extern long memory_cache_size;
extern long image_cache_size;

extern struct rgb default_fg;
extern struct rgb default_bg;
extern struct rgb default_link;
extern struct rgb default_vlink;

#ifdef G
extern struct rgb default_fg_g;
extern struct rgb default_bg_g;
extern struct rgb default_link_g;
extern struct rgb default_vlink_g;
#endif

#define REFERER_NONE 0
#define REFERER_SAME_URL 1
#define REFERER_FAKE 2
#define REFERER_REAL 3

extern uchar http_proxy[];
extern uchar ftp_proxy[];
extern uchar fake_useragent[];
extern uchar fake_referer[];
extern int referer;
#ifdef JS
extern int js_enable;
extern int js_verbose_errors;
extern int js_verbose_warnings;
extern int js_all_conversions;
extern int js_global_resolve;
#endif
extern uchar download_dir[];

struct http_bugs {
	int http10;
	int allow_blacklist;
	int bug_302_redirect;
	int bug_post_no_keepalive;
	int no_accept_charset;
	int aggressive_cache;
};

extern struct http_bugs http_bugs;

extern uchar default_anon_pass[];

/* listedit.c */


#define TITLE_EDIT 0
#define TITLE_ADD 1

struct list{
	void *next;
	void *prev;
	uchar type;
	/*
	 * bit 0: 0=item, 1=directory 
	 * bit 1: directory is open (1)/close (0); for item unused
	 * bit 2: 1=item is selected 0=item is not selected
	 */
	int depth;
	void *fotr;   /* ignored when list is flat */
};

struct list_description{
	uchar type;  /* 0=flat, 1=tree */
	struct list* list;   /* head of the list */
	void *(*new_item)(void * /* data in internal format */);  /* creates new item, does NOT add to the list */
	void (*edit_item)(struct dialog_data *, void * /* item */, void(*)(struct dialog_data *,void *,void *,struct list_description *)/* ok function */, void * /* parameter for the ok_function */, uchar);  /* must call call delete_item on the item after all */
	void *(*default_value)(struct session *, uchar /* 0=item, 1=directory */);  /* called when add button is pressed, allocates memory, return value is passed to the new_item function, new_item fills the item with this data */
	void (*delete_item)(void *);  /* delete item, if next and prev are not NULL adjusts pointers in the list */
	void (*copy_item)(void * /* old */, void * /* new */);  /* gets 2 allocated items, copies all item data except pointers from first item to second one, old data (in second item) will be destroyed */
	uchar* (*type_item)(struct terminal *, void*, int /* 0=type whole item (e.g. when deleting item), 1=type only e.g title (in list window )*/);   /* alllocates buffer and writes item into it */
	void *(*find_item)(void *start_item, uchar *string, int direction /* 1 or -1 */); /* returns pointer to the first item matching given string or NULL if failed. Search starts at start_item including. */
	struct history *search_history;
	int codepage;	/* codepage of all string */
	int window_width;     /* main window width */
	int n_items;   /* number of items in main window */
	
	/* following items are string codes */
	int item_description;  /* e.g. "bookmark" or "extension" ... */
	int already_in_use;   /* e.g. "Bookmarks window is already open" */
	int window_title;   /* main window title */
	int delete_dialog_title;   /* e.g. "Delete bookmark dialog" */
	int button;  /* when there's no button button_fn is NULL */
	
	void (*button_fn)(struct session *, void *);  /* gets pointer to the item */

	/* internal variables, should not be modified, initially set to 0 */
	struct list *current_pos;
	struct list *win_offset;
	int win_pos;
	int open;  /* 0=closed, 1=open */
	int modified; /* listedit reports 1 when the list was modified by user (and should be e.g. saved) */
	struct dialog_data *dlg;  /* current dialog, valid only when open==1 */ 
	uchar *search_word;
	int search_direction;
};

extern int create_list_window(struct list_description *,struct list *,struct terminal *,struct session *);
/* following 2 functions should be called after calling create_list_window fn */
extern void redraw_list_window(struct list_description *ld);	/* redraws list window */
extern void reinit_list_window(struct list_description *ld);	/* reinitializes list window */

/* bookmarks.c */

/* Where all bookmarks are kept */
extern uchar bookmarks_file[];
extern int bookmarks_codepage;
extern struct list bookmarks;

void finalize_bookmarks(void);   /* called, when exiting links */
void init_bookmarks(void);   /* called at start */
void reinit_bookmarks(void);
void save_bookmarks(void);

/* Launches bookmark manager */
void menu_bookmark_manager(struct terminal *, void *, struct session *);

#endif /* #ifndef _LINKS_H */
