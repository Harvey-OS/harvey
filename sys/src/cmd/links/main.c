/* main.c
 * main()
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

int retval = RET_OK;
int shootme;

void unhandle_basic_signals(struct terminal *);

void sig_terminate(struct terminal *t)
{
	unhandle_basic_signals(t);
	terminate_loop = 1;
	retval = RET_SIGNAL;
}

void sig_intr(struct terminal *t)
{
	if (!t) {
		unhandle_basic_signals(t);
		terminate_loop = 1;
	} else {
		unhandle_basic_signals(t);
		exit_prog(t, NULL, NULL);
	}
}

void sig_ctrl_c(struct terminal *t)
{
	if (!is_blocked()) kbd_ctrl_c();
}

void sig_ign(void *x)
{
}

void sig_tstp(struct terminal *t)
{
#ifdef SIGSTOP
	int pid = getpid();
	if (!F) block_itrm(0);
#ifdef G
	else drv->block(NULL);
#endif
#if defined (SIGCONT) && defined(SIGTTOU)
	if (!fork()) {
		sleep(1);
		kill(pid, SIGCONT);
		exit(0);
	}
#endif
	raise(SIGSTOP);
#endif
}

void sig_cont(struct terminal *t)
{
	if (!F) {
		if (!unblock_itrm(0)) resize_terminal();
#ifdef G
	} else {
		drv->unblock(NULL);
#endif
	}
	/*else register_bottom_half(raise, SIGSTOP);*/
}

void handle_basic_signals(struct terminal *term)
{
	install_signal_handler(SIGHUP, (void (*)(void *))sig_intr, term, 0);
	if (!F) install_signal_handler(SIGINT, (void (*)(void *))sig_ctrl_c, term, 0);
	/*install_signal_handler(SIGTERM, (void (*)(void *))sig_terminate, term, 0);*/
#ifdef SIGTSTP
	if (!F) install_signal_handler(SIGTSTP, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTIN
	if (!F) install_signal_handler(SIGTTIN, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, (void (*)(void *))sig_ign, term, 0);
#endif
#ifdef SIGCONT
	if (!F) install_signal_handler(SIGCONT, (void (*)(void *))sig_cont, term, 0);
#endif
}

/*void handle_slave_signals(struct terminal *term)
{
	install_signal_handler(SIGHUP, (void (*)(void *))sig_terminate, term, 0);
	install_signal_handler(SIGINT, (void (*)(void *))sig_terminate, term, 0);
	install_signal_handler(SIGTERM, (void (*)(void *))sig_terminate, term, 0);
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTIN
	install_signal_handler(SIGTTIN, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, (void (*)(void *))sig_ign, term, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, (void (*)(void *))sig_cont, term, 0);
#endif
}*/

void unhandle_terminal_signals(struct terminal *term)
{
	install_signal_handler(SIGHUP, NULL, NULL, 0);
	if (!F) install_signal_handler(SIGINT, NULL, NULL, 0);
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, NULL, NULL, 0);
#endif
#ifdef SIGTTIN
	install_signal_handler(SIGTTIN, NULL, NULL, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, NULL, NULL, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, NULL, NULL, 0);
#endif
}

void unhandle_basic_signals(struct terminal *term)
{
	install_signal_handler(SIGHUP, NULL, NULL, 0);
	if (!F) install_signal_handler(SIGINT, NULL, NULL, 0);
	/*install_signal_handler(SIGTERM, NULL, NULL, 0);*/
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, NULL, NULL, 0);
#endif
#ifdef SIGTTIN
	install_signal_handler(SIGTTIN, NULL, NULL, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, NULL, NULL, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, NULL, NULL, 0);
#endif
}

int terminal_pipe[2];

int attach_terminal(int in, int out, int ctl, void *info, int len)
{
	struct terminal *term;

	/* danger, will robinson! */
	fcntl(terminal_pipe[0], F_SETFL, O_NONBLOCK);
	fcntl(terminal_pipe[1], F_SETFL, O_NONBLOCK);

	handle_trm(in, out, out, terminal_pipe[1], ctl, info, len);
	mem_free(info);
	if ((term = init_term(terminal_pipe[0], out, win_func))) {
		handle_basic_signals(term);
		/*
		 * OK, this is race condition, but it must be so;
		 * GPM installs it's own buggy TSTP handler.
		 */
		return terminal_pipe[1];
	}
	close(terminal_pipe[0]);
	close(terminal_pipe[1]);
	return -1;
}

#ifdef G

int attach_g_terminal(void *info, int len)
{
	struct terminal *term;
	term = init_gfx_term(win_func, info, len);
	mem_free(info);
	return term ? 0 : -1;
}

#endif

struct object_request *dump_obj;
int dump_pos;

void end_dump(struct object_request *r, void *p)
{
	struct cache_entry *ce;
	int oh;
	if (!r->state || (r->state == 1 && dmp != D_SOURCE)) return;
	if ((oh = get_output_handle()) == -1) return;
	ce = r->ce;
	if (dmp == D_SOURCE) {
		if (ce) {
			struct fragment *frag;
nextfrag:
			foreach(frag, ce->frag) if (frag->offset <= dump_pos && frag->offset + frag->length > dump_pos) {
				int l = frag->length - (dump_pos - frag->offset);
				int w = hard_write(oh, frag->data + dump_pos - frag->offset, l);
				if (w != l) {
					detach_object_connection(r, dump_pos);
					if (w < 0) fprintf(stderr, "Error writing to stdout: %s.\n", strerror(errno));
					else fprintf(stderr, "Can't write to stdout.\n");
					retval = RET_ERROR;
					goto terminate;
				}
				dump_pos += w;
				detach_object_connection(r, dump_pos);
				goto nextfrag;
			}
		}
		if (r->state >= 0) return;
	} else if (ce) {
		/* !!! FIXME */
#ifdef notdef
		struct document_options o;
		struct view_state *vs;
		struct f_data_c *fd;

		if (!(vs = create_vs())) goto terminate;
		if (!(vs = create_vs())) goto terminate;
		memset(&o, 0, sizeof(struct document_options));
		o.xp = 0;
		o.yp = 1;
		o.xw = 80;
		o.yw = 25;
		o.col = 0;
		o.cp = 0;
		ds2do(&dds, &o);
		o.plain = 0;
		o.frames = 0;
		memcpy(&o.default_fg, &default_fg, sizeof(struct rgb));
		memcpy(&o.default_bg, &default_bg, sizeof(struct rgb));
		memcpy(&o.default_link, &default_link, sizeof(struct rgb));
		memcpy(&o.default_vlink, &default_vlink, sizeof(struct rgb));
		o.framename = "";
		init_vs(vs, stat->ce->url);
		cached_format_html(vs, &fd, &o);
		dump_to_file(fd.f_data, oh);
		detach_formatted(&fd);
		destroy_vs(vs);
#endif
	}
	if (r->state != O_OK) {
		unsigned char *m = get_err_msg(r->stat.state);
		fprintf(stderr, "%s\n", get_english_translation(m));
		retval = RET_ERROR;
	}
terminate:
	terminate_loop = 1;
}

int g_argc;
unsigned char **g_argv;
unsigned char *path_to_exe;
int init_b = 0;

void initialize_all_subsystems(void);
void initialize_all_subsystems_2(void);

void
init(void)
{
	int uh, len;
	void *info;
	unsigned char *u;

#ifdef TEST
	{
		int test(void);

		test();
		return;
	}
#endif
	initialize_all_subsystems();

/* OS/2 has some stupid bug and the pipe must be created before socket :-/ */
	if (c_pipe(terminal_pipe)) {
		error((unsigned char *)"ERROR: can't create pipe for internal communication");
		goto ttt;
	}

	/* parse options once */
	if (!(u = parse_options(g_argc - 1, g_argv + 1)))
		goto ttt;

	/* hardcoded graphics */
	ggr = 1;
	if (ggr_drv[0] || ggr_mode[0])
		ggr = 1;
	if (dmp)
		ggr = 0;

	if (!ggr && !no_connect && (uh = bind_to_af_unix()) != -1) {
		/* communicate with already-running links via unix-dom. sock. */
		close(terminal_pipe[0]);
		close(terminal_pipe[1]);
		if (!(info = create_session_info(base_session, u, default_target, &len))) {
			close(uh);
			goto ttt;
		}
		initialize_all_subsystems_2();
		handle_trm(get_input_handle(), get_output_handle(), uh, uh,
			get_ctl_handle(), info, len);
		/*
		 * OK, this is race condition, but it must be so;
		 * GPM installs it's own buggy TSTP handler.
		 */
		handle_basic_signals(NULL);
		mem_free(info);
		return;
	}

	/* normal startup of a new links */
	if ((dds.assume_cp = get_cp_index((uchar *)"ISO-8859-1")) == -1)
		dds.assume_cp = 0;
	load_config();
	init_b = 1;
	init_bookmarks();
	create_initial_extensions();
	load_url_history();
	init_cookies();
	/* parse options a second time; ghod only knows why */
	u = parse_options(g_argc - 1, g_argv + 1);
	if (!u) {
ttt:
		initialize_all_subsystems_2();
tttt:
		terminate_loop = 1;
		retval = RET_SYNTAX;
		return;
	}

	if (dmp) {
		unsigned char *uu, *wd;

		initialize_all_subsystems_2();
		close(terminal_pipe[0]);
		close(terminal_pipe[1]);
		if (!*u) {
			fprintf(stderr, "URL expected after %s\n.",
				dmp == D_DUMP? "-dump": "-source");
			goto tttt;
		}
		if (!(uu = translate_url(u, wd = get_cwd())))
			uu = stracpy(u);
		request_object(NULL, uu, NULL, PRI_MAIN, NC_RELOAD, end_dump,
			NULL, &dump_obj);
		mem_free(uu);
		if (wd)
			mem_free(wd);
		return;
	}

	/* optional graphical startup */
	if (ggr) {
#ifdef G
		unsigned char *r;

		r = init_graphics(ggr_drv, ggr_mode, ggr_display);
		if (r) {
			fprintf(stderr, "%s", (char *)r);
			mem_free(r);
			goto ttt;
		}
		handle_basic_signals(NULL);
		init_dither(drv->depth);
		F = 1;
#else
		fprintf(stderr, "links: graphics compiled out\n");
		goto ttt;
#endif
	}

	initialize_all_subsystems_2();
	info = create_session_info(base_session, u, default_target, &len);
	if (!info || gf_val(attach_terminal(get_input_handle(),
	    get_output_handle(), get_ctl_handle(), info, len),
	    attach_g_terminal(info, len)) == -1) {
		retval = RET_FATAL;
		terminate_loop = 1;
	}
}

/*
 * don't initialise all subsystems.
 * is called before graphics driver init.
 */
void
initialize_all_subsystems(void)
{
	init_trans();
	set_sigcld();
	init_home();
	init_dns();
	init_cache();
	iinit_bfu();
}

/*
 * don't initialise different subsystems.
 * is called sometimes after and sometimes before graphics driver init.
 */
void
initialize_all_subsystems_2(void)
{
	GF(init_dip());
	init_bfu();
	GF(init_imgcache());
	init_fcache();
	GF(init_grview());
}

void
terminate_all_subsystems(void)
{
	if (!F)
		af_unix_close();
	check_bottom_halves();
	abort_all_downloads();
#ifdef HAVE_SSL
	ssl_finish();
#endif
	check_bottom_halves();
	destroy_all_terminals();
	check_bottom_halves();
	shutdown_bfu();
	GF(shutdown_dip());
	if (!F)
		free_all_itrms();
	release_object(&dump_obj);
	abort_all_connections();

	free_all_caches();
	if (init_b)
		save_url_history();
	free_history_lists();
	free_term_specs();
	free_types();
	if (init_b)
		finalize_bookmarks();
	free_conv_table();
	free_blacklist();
	if (init_b)
		cleanup_cookies();
	cleanup_auth();
	check_bottom_halves();
	end_config();
	free_strerror_buf();
	shutdown_trans();
	GF(shutdown_graphics());
	terminate_osdep();
	if (shootme)
		kill(shootme, SIGKILL);
}

int
main(int argc, char *argv[])
{
	setbuf(stderr, NULL);			/* print errors promptly */
	path_to_exe = (unsigned char *)argv[0];
	g_argc = argc;
	g_argv = (unsigned char **)argv;
	select_loop(init);
	terminate_all_subsystems();
	check_memory_leaks();
	return retval;
}

/*
 * test rubbish from here on
 */

#ifdef TEST
#define G_S	16

unsigned char *txt;
int txtl;
struct style *gst;
struct bitmap xb;
struct bitmap *xbb[16] = {
	&xb, &xb, &xb, &xb, &xb, &xb, &xb, &xb,
	&xb, &xb, &xb, &xb, &xb, &xb, &xb, &xb,
};

void
rdr(struct graphics_device *dev, struct rect *r)
{
	struct style *st;
	int i;
	drv->set_clip_area(dev, r);

	{
		int x, y;
		for (y = 0; y < dev->size.y2; y+=G_S)
			for (x = 0; x < dev->size.x2; ) {
				g_print_text(drv, dev, x, y, gst, "Toto je pokus. ", &x);
				//drv->draw_bitmap(dev, &xb, x, y); x += xb.x;
				//drv->draw_bitmaps(dev, xbb, 16, x, y); x += xb.x << 4;
			}
	}
	st = g_get_style(0x000000, 0xffffff, 20, "", 0);
	g_print_text(drv, dev, 0, dev->size.y2 / 2, st, txt, NULL);
	g_free_style(st);
}

void
rsz(struct graphics_device *dev)
{
	rdr(dev, &dev->size);
}

int dv = 2;

void
key(struct graphics_device *dev, int x, int y)
{
	if (x == 'q') {
		drv->shutdown_device(dev);
		if (--dv) return;
		mem_free(txt);
		drv->unregister_bitmap(&xb);
		terminate_loop = 1;
		g_free_style(gst);
		return;
	}
	add_to_str(&txt, &txtl, "(");
	add_num_to_str(&txt, &txtl, x);
	add_to_str(&txt, &txtl, ",");
	add_num_to_str(&txt, &txtl, y);
	add_to_str(&txt, &txtl, ") ");
	rdr(dev, &dev->size);
}

int
test(void)
{
	struct graphics_device *dev, *dev2;
	unsigned char *msg;

	initialize_all_subsystems();
	F = 1;
	initialize_all_subsystems_2();
	printf("i\n");fflush(stdout);
	msg = init_graphics("", "", "");
	if (msg) {
		printf("-%s-\n", msg);fflush(stdout);
		mem_free(msg);
		terminate_loop = 1;
		return 0;
	}
	printf("ok\n");
	fflush(stdout);
	gst = g_get_style(0x000000, 0xffffff, G_S, "", 0);
	txt = init_str();
	txtl = 0;
	dev = drv->init_device(); dev->redraw_handler = rdr; dev->resize_handler = rsz; dev->keyboard_handler = key;
	dev2 = drv->init_device(); dev2->redraw_handler = rdr; dev2->resize_handler = rsz; dev2->keyboard_handler = key;
	{
		int x, y;
		xb.x = G_S / 2;
		xb.y = G_S;
		drv->get_empty_bitmap(&xb);
		for (y = 0; y < xb.y; y++)
			for (x = 0; x < xb.x; x++)
				((char *)xb.data)[y * xb.skip + x] = x + y;
		drv->register_bitmap(&xb);
	}
	return 0;
}
#endif
