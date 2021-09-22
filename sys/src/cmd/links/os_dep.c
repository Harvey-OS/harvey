/*
 * Plan 9 OS-dependent code (other than plan9.c).
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"
#include <sys/ioctl.h>

#define SIGWINCH 28

int is_safe_in_shell(unsigned char c)
{
	return c == '@' || c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= 'a' && c <= 'z');
}

void check_shell_security(unsigned char **cmd)
{
	unsigned char *c = *cmd;
	while (*c) {
		if (!is_safe_in_shell(*c)) *c = '_';
		c++;
	}
}

int get_e(char *env)
{
	char *v;
	if ((v = getenv(env))) return atoi(v);
	return 0;
}


/* Terminal size */

void sigwinch(void *s)
{
	((void (*)(void))s)();
}

void handle_terminal_resize(int fd, void (*fn)(void))
{
	install_signal_handler(SIGWINCH, sigwinch, fn, 0);
}

void unhandle_terminal_resize(int fd)
{
	install_signal_handler(SIGWINCH, NULL, NULL, 0);
}

int get_terminal_size(int fd, int *x, int *y)
{

	return 0;
}

/* Pipe */

void set_bin(int fd)
{
}

int c_pipe(int *fd)
{
	return pipe(fd);
}

/* Filename */

int check_file_name(unsigned char *file)
{
	return 1;		/* !!! FIXME */
}

/* Exec */

int can_twterm(void) /* Check if it make sense to call a twterm. */
{
	static int xt = -1;
	if (xt == -1) xt = !!getenv("TWDISPLAY");
	return xt;
}

int is_xterm(void)
{
	static int xt = -1;
	if (xt == -1) xt = getenv("DISPLAY") && *getenv("DISPLAY");
	return xt;
}

tcount resize_count = 0;

void close_fork_tty()
{
	struct terminal *t;
	foreach (t, terminals) if (t->fdin > 0) close(t->fdin);
}

int
exe(char *path, int fg)
{
	int p, s;

	fg=fg;  /* ignore flag */
	if (!(p = fork())) {
		setpgid(0, 0);
		system(path);
		_exit(0);
	}
	if (p > 0)
		waitpid(p, &s, 0);
	else
		return system(path);
	return 0;
}

/* clipboard -> links */
unsigned char *
get_clipboard_text(void)	/* !!! FIXME */
{
	unsigned char *ret = 0;

	if ((ret = mem_alloc(1)))
		ret[0] = 0;
	return ret;
}

/* links -> clipboard */
void
set_clipboard_text(struct terminal * term, unsigned char *data)
{
	/* !!! FIXME */
}

void set_window_title(unsigned char *url)
{
	/* !!! FIXME */
}

unsigned char *get_window_title(void)
{
	/* !!! FIXME */
	return NULL;
}

int resize_window(int x, int y)
{
	return -1;
}

/* Threads */

struct tdata {
	void	(*fn)(void *, int);
	int	h;
	uchar	data[1];
};

void
terminate_osdep(void)
{
}

void
block_stdin(void)
{
}

void
unblock_stdin(void)
{
}

int
start_thread(void (*fn)(void *, int), void *ptr, int l)
{
	int f, p[2];

	if (c_pipe(p) < 0)
		return -1;

	/* danger, will robinson! */
	fcntl(p[0], F_SETFL, O_NONBLOCK);
	fcntl(p[1], F_SETFL, O_NONBLOCK);

	if (!(f = fork())) {
		close_fork_tty();
		close(p[0]);
		(*fn)(ptr, p[1]);	/* invoke fn in the subprocess */
		write(p[1], "x", 1);
		close(p[1]);
		_exit(0);
	}
	if (f == -1) {
		close(p[0]);
		close(p[1]);
		return -1;
	}
	close(p[1]);
	return p[0];
}

void want_draw(void) {}
void done_draw(void) {}
int get_output_handle(void) { return 1; }
int get_ctl_handle(void) { return 0; }

int
get_input_handle(void)
{
	return 0;
}

void
os_cfmakeraw(struct termios *t)
{
	t->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	t->c_oflag &= ~OPOST;
	t->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	t->c_cflag &= ~(CSIZE|PARENB);
	t->c_cflag |= CS8;
	t->c_cc[VMIN] = 1;
	t->c_cc[VTIME] = 0;
}

void *
handle_mouse(int cons, void (*fn)(void *, uchar *, int), void *data)
{
	return NULL;
}

void
unhandle_mouse(void *data)
{
}

int
get_system_env(void)
{
	return 0;
}

void exec_new_links(struct terminal *term, unsigned char *xterm, unsigned char *exe, unsigned char *param)
{
	unsigned char *str;
	if (!(str = mem_alloc(strlen((char *)xterm) + 1 + strlen((char *)exe) + 1 + strlen((char *)param) + 1))) return;
	sprintf((char *)str, "%s %s %s", xterm, exe, param);
	exec_on_terminal(term, str, (unsigned char *)"", 2);
	mem_free(str);
}

void open_in_new_twterm(struct terminal *term, unsigned char *exe, unsigned char *param)
{
	unsigned char *twterm;
	if (!(twterm = (unsigned char *)getenv("LINKS_TWTERM"))) twterm = (unsigned char *)"twterm -e";
	exec_new_links(term, twterm, exe, param);
}

void open_in_new_xterm(struct terminal *term, unsigned char *exe, unsigned char *param)
{
	unsigned char *xterm;
	if (!(xterm = (unsigned char *)getenv("LINKS_XTERM"))) xterm = (unsigned char *)"xterm -e";
	exec_new_links(term, xterm, exe, param);
}

void open_in_new_screen(struct terminal *term, unsigned char *exe, unsigned char *param)
{
	exec_new_links(term, (unsigned char *)"screen", exe, param);
}

#ifdef G
void
open_in_new_g(struct terminal *term, unsigned char *exe, unsigned char *param)
{
	void *info;
	unsigned char *target=NULL;
	int len;
	int base = 0;
	unsigned char *url = (unsigned char *)"";
	if (!cmpbeg(param, (unsigned char *)"-target "))
	{
		unsigned char *p;
		target=param+strlen("-target ");
		for (p=target;*p!=' '&&*p;p++);
		*p=0;
		param=p+1;
	}
	if (!cmpbeg(param, (unsigned char *)"-base-session ")) {
		base = atoi((char *)param + strlen("-base-session "));
	} else {
		url = param;
	}
	if ((info = create_session_info(base, url, target, &len))) attach_g_terminal(info, len);
}
#endif

struct {
	int	env;
	void	(*fn)(struct terminal *term, unsigned char *, unsigned char *);
	uchar	*text;
	uchar	*hk;
} oinw[] = {
	{ENV_XWIN, open_in_new_xterm, TEXT(T_XTERM), TEXT(T_HK_XTERM)},
	{ENV_TWIN, open_in_new_twterm, TEXT(T_TWTERM), TEXT(T_HK_TWTERM)},
	{ENV_SCREEN, open_in_new_screen, TEXT(T_SCREEN), TEXT(T_HK_SCREEN)},
#ifdef G
	{ENV_G, open_in_new_g, TEXT(T_WINDOW), TEXT(T_HK_WINDOW)},
#endif
	{0, NULL, NULL}
};

struct open_in_new *
get_open_in_new(int environment)
{
	int i;
	struct open_in_new *oin = DUMMY;
	int noin = 0;
	if (environment & ENV_G) environment = ENV_G;
	for (i = 0; oinw[i].env; i++) if ((environment & oinw[i].env) == oinw[i].env) {
		struct open_in_new *x;
		if (!(x = mem_realloc(oin, (noin + 2) * sizeof(struct open_in_new)))) continue;
		oin = x;
		oin[noin].text = oinw[i].text;
		oin[noin].hk = oinw[i].hk;
		oin[noin].fn = oinw[i].fn;
		noin++;
		oin[noin].text = NULL;
		oin[noin].hk = NULL;
		oin[noin].fn = NULL;
	}
	if (oin == DUMMY) return NULL;
	return oin;
}

int can_resize_window(int environment)
{
	if (environment & ENV_OS2VIO) return 1;
	return 0;
}

int can_open_os_shell(int environment)
{
	return 1;
}

void
set_highpri(void)
{
}

