/* menu.c
 * (c) 2002 Mikulas Patocka, Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

/*static inline struct session *get_term_session(struct terminal *term)
{
	if ((void *)term->windows.prev == &term->windows) {
		internal("terminal has no windows");
		return NULL;
	}
	return ((struct window *)term->windows.prev)->data;
}*/

void menu_about(struct terminal *term, void *d, struct session *ses)
{
	msg_box(term, NULL, TEXT(T_ABOUT), AL_CENTER, TEXT(T_LINKS__LYNX_LIKE), NULL, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
}

void menu_keys(struct terminal *term, void *d, struct session *ses)
{
	msg_box(term, NULL, TEXT(T_KEYS), AL_LEFT | AL_MONO, TEXT(T_KEYS_DESC), NULL, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
}

void menu_copying(struct terminal *term, void *d, struct session *ses)
{
	msg_box(term, NULL, TEXT(T_COPYING), AL_CENTER, TEXT(T_COPYING_DESC), NULL, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
}

void menu_manual(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, LINKS_MANUAL_URL);
}

void menu_homepage(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, LINKS_HOMEPAGE_URL);
}

void menu_calibration(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, LINKS_CALIBRATION_URL);
}

void menu_for_frame(struct terminal *term, void (*f)(struct session *, struct f_data_c *, int), struct session *ses)
{
	do_for_frame(ses, f, 0);
}

void menu_goto_url(struct terminal *term, void *d, struct session *ses)
{
	dialog_goto_url(ses, "");
}

void menu_save_url_as(struct terminal *term, void *d, struct session *ses)
{
	dialog_save_url(ses);
}

void menu_save_bookmarks(struct terminal *term, void *d, struct session *ses)
{
	save_bookmarks();
}

void menu_go_back(struct terminal *term, void *d, struct session *ses)
{
	go_back(ses);
}

void menu_reload(struct terminal *term, void *d, struct session *ses)
{
	reload(ses, -1);
}

void really_exit_prog(struct session *ses)
{
	register_bottom_half((void (*)(void *))destroy_terminal, ses->term);
}

void dont_exit_prog(struct session *ses)
{
	ses->exit_query = 0;
}

void query_exit(struct session *ses)
{
	ses->exit_query = 1;
	msg_box(ses->term, NULL, TEXT(T_EXIT_LINKS), AL_CENTER, (ses->term->next == ses->term->prev && are_there_downloads()) ? TEXT(T_DO_YOU_REALLY_WANT_TO_EXIT_LINKS_AND_TERMINATE_ALL_DOWNLOADS) : TEXT(T_DO_YOU_REALLY_WANT_TO_EXIT_LINKS), ses, 2, TEXT(T_YES), (void (*)(void *))really_exit_prog, B_ENTER, TEXT(T_NO), dont_exit_prog, B_ESC);
}

void exit_prog(struct terminal *term, void *d, struct session *ses)
{
	if (!ses) {
		register_bottom_half((void (*)(void *))destroy_terminal, term);
		return;
	}
	if (!ses->exit_query && (!d || (term->next == term->prev && are_there_downloads()))) {
		query_exit(ses);
		return;
	}
	really_exit_prog(ses);
}

struct refresh {
	struct terminal *term;
	struct window *win;
	struct session *ses;
	void (*fn)(struct terminal *term, void *d, struct session *ses);
	void *data;
	int timer;
};

void refresh(struct refresh *r)
{
	struct refresh rr;
	r->timer = -1;
	memcpy(&rr, r, sizeof(struct refresh));
	rr.fn(rr.term, rr.data, rr.ses);
	delete_window(r->win);
}

void end_refresh(struct refresh *r)
{
	if (r->timer != -1) kill_timer(r->timer);
	mem_free(r);
}

void refresh_abort(struct dialog_data *dlg)
{
	end_refresh(dlg->dlg->udata2);
}

void cache_inf(struct terminal *term, void *d, struct session *ses)
{
	unsigned char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9, *a10, *a11, *a12, *a13, *a14, *a15, *a16;
#ifdef G
	unsigned char *b14, *b15, *b16, *b17;
	unsigned char *c14, *c15, *c16, *c17;
#endif
	int l = 0;
	struct refresh *r;
	if (!(r = mem_alloc(sizeof(struct refresh)))) return;
	r->term = term;
	r->win = NULL;
	r->ses = ses;
	r->fn = cache_inf;
	r->data = d;
	r->timer = -1;
	l = 0;
	l = 0, a1 = init_str(); add_to_str(&a1, &l, (unsigned char *)": "); add_num_to_str(&a1, &l, select_info(CI_FILES));add_to_str(&a1, &l, (unsigned char *)" ");
	l = 0, a2 = init_str(); add_to_str(&a2, &l, (unsigned char *)", "); add_num_to_str(&a2, &l, select_info(CI_TIMERS));add_to_str(&a2, &l, (unsigned char *)" ");
	l = 0, a3 = init_str(); add_to_str(&a3, &l, (unsigned char *)".\n");

	l = 0, a4 = init_str(); add_to_str(&a4, &l, (unsigned char *)": "); add_num_to_str(&a4, &l, connect_info(CI_FILES));add_to_str(&a4, &l, (unsigned char *)" ");
	l = 0, a5 = init_str(); add_to_str(&a5, &l, (unsigned char *)", "); add_num_to_str(&a5, &l, connect_info(CI_CONNECTING));add_to_str(&a5, &l, (unsigned char *)" ");
	l = 0, a6 = init_str(); add_to_str(&a6, &l, (unsigned char *)", "); add_num_to_str(&a6, &l, connect_info(CI_TRANSFER));add_to_str(&a6, &l, (unsigned char *)" ");
	l = 0, a7 = init_str(); add_to_str(&a7, &l, (unsigned char *)", "); add_num_to_str(&a7, &l, connect_info(CI_KEEP));add_to_str(&a7, &l, (unsigned char *)" ");
	l = 0, a8 = init_str(); add_to_str(&a8, &l, (unsigned char *)".\n");

	l = 0, a9 = init_str(); add_to_str(&a9, &l, (unsigned char *)": "); add_num_to_str(&a9, &l, cache_info(CI_BYTES));add_to_str(&a9, &l, (unsigned char *)" ");
	l = 0, a10 = init_str(); add_to_str(&a10, &l, (unsigned char *)", "); add_num_to_str(&a10, &l, cache_info(CI_FILES));add_to_str(&a10, &l, (unsigned char *)" ");
	l = 0, a11 = init_str(); add_to_str(&a11, &l, (unsigned char *)", "); add_num_to_str(&a11, &l, cache_info(CI_LOCKED));add_to_str(&a11, &l, (unsigned char *)" ");
	l = 0, a12 = init_str(); add_to_str(&a12, &l, (unsigned char *)", "); add_num_to_str(&a12, &l, cache_info(CI_LOADING));add_to_str(&a12, &l, (unsigned char *)" ");
	l = 0, a13 = init_str(); add_to_str(&a13, &l, (unsigned char *)".\n");

#ifdef G
	if (F) {
		l = 0, b14 = init_str();
		add_to_str(&b14, &l, (unsigned char *)", ");
		add_num_to_str(&b14, &l, imgcache_info(CI_BYTES));
		add_to_str(&b14, &l, (unsigned char *)" ");
		
		l = 0, b15 = init_str(); 
		add_to_str(&b15, &l, (unsigned char *)", ");
		add_num_to_str(&b15, &l, imgcache_info(CI_FILES));
		add_to_str(&b15, &l, (unsigned char *)" ");
		
		l = 0, b16 = init_str(); 
		add_to_str(&b16, &l, (unsigned char *)", ");
		add_num_to_str(&b16, &l, imgcache_info(CI_LOCKED));
		add_to_str(&b16, &l, (unsigned char *)" ");
		
		l = 0, b17 = init_str();
		add_to_str(&b17, &l, (unsigned char *)".\n");
		
		l = 0, c14 = init_str();
		add_to_str(&c14, &l, (unsigned char *)", ");
		add_num_to_str(&c14, &l, font_cache.bytes);
		add_to_str(&c14, &l, (unsigned char *)" ");
		
		l = 0, c15 = init_str(); 
		add_to_str(&c15, &l, (unsigned char *)", ");
		add_num_to_str(&c15, &l, font_cache.max_bytes);
		add_to_str(&c15, &l, (unsigned char *)" ");
		
		l = 0, c16 = init_str(); 
		add_to_str(&c16, &l, (unsigned char *)", ");
		add_num_to_str(&c16, &l, font_cache.items);
		add_to_str(&c16, &l, (unsigned char *)" ");
		
		l = 0, c17 = init_str();
		add_to_str(&c17, &l, (unsigned char *)".\n");
	}
#endif

	l = 0, a14 = init_str(); add_to_str(&a14, &l, (unsigned char *)": "); add_num_to_str(&a14, &l, formatted_info(CI_FILES));add_to_str(&a14, &l, (unsigned char *)" ");
	l = 0, a15 = init_str(); add_to_str(&a15, &l, (unsigned char *)", "); add_num_to_str(&a15, &l, formatted_info(CI_LOCKED));add_to_str(&a15, &l, (unsigned char *)" ");
	l = 0, a16 = init_str(); add_to_str(&a16, &l, (unsigned char *)".");

	if (!F) msg_box(term, getml(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, NULL), TEXT(T_RESOURCES), AL_LEFT | AL_EXTD_TEXT, TEXT(T_RESOURCES), a1, TEXT(T_HANDLES), a2, TEXT(T_TIMERS), a3, TEXT(T_CONNECTIONS), a4, TEXT(T_cONNECTIONS), a5, TEXT(T_CONNECTING), a6, TEXT(T_tRANSFERRING), a7, TEXT(T_KEEPALIVE), a8, TEXT(T_MEMORY_CACHE), a9, TEXT(T_BYTES), a10, TEXT(T_FILES), a11, TEXT(T_LOCKED), a12, TEXT(T_LOADING), a13, TEXT(T_FORMATTED_DOCUMENT_CACHE), a14, TEXT(T_DOCUMENTS), a15, TEXT(T_LOCKED), a16, NULL, r, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
#ifdef G
	else msg_box(term, getml(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
				a12, a13, b14, b15, b16, b17, a14, a15, a16,
				c14, c15, c16, c17, NULL), TEXT(T_RESOURCES),
			AL_LEFT | AL_EXTD_TEXT, TEXT(T_RESOURCES), a1,
			TEXT(T_HANDLES), a2, TEXT(T_TIMERS), a3,
			TEXT(T_CONNECTIONS), a4, TEXT(T_cONNECTIONS), a5,
			TEXT(T_CONNECTING), a6, TEXT(T_tRANSFERRING), a7,
			TEXT(T_KEEPALIVE), a8, TEXT(T_MEMORY_CACHE), a9,
			TEXT(T_BYTES), a10, TEXT(T_FILES), a11, TEXT(T_LOCKED),
			a12, TEXT(T_LOADING), a13, TEXT(T_IMAGE_CACHE), b14,
			TEXT(T_BYTES), b15, TEXT(T_IMAGES), b16, TEXT(T_LOCKED),
			b17, TEXT(T_FONT_CACHE), c14, TEXT(T_BYTES), c15,
			TEXT(T_BYTES_MAX), c16, TEXT(T_LETTERS), c17,
			TEXT(T_FORMATTED_DOCUMENT_CACHE), a14, TEXT(T_DOCUMENTS), a15, TEXT(T_LOCKED), a16, NULL, r, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
#endif
	r->win = term->windows.next;
	((struct dialog_data *)r->win->data)->dlg->abort = refresh_abort;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
}

#ifdef DEBUG

void list_cache(struct terminal *term, void *d, struct session *ses)
{
	unsigned char *a;
	int l = 0;
	struct refresh *r;
	struct cache_entry *ce, *cache;
	if (!(a = init_str())) return;
	if (!(r = mem_alloc(sizeof(struct refresh)))) {
		mem_free(a);
		return;
	}
	r->term = term;
	r->win = NULL;
	r->ses = ses;
	r->fn = list_cache;
	r->data = d;
	r->timer = -1;
	cache = (struct cache_entry *)cache_info(CI_LIST);
	add_to_str(&a, &l,(unsigned char *) ":");
	foreach(ce, *cache) {
		add_to_str(&a, &l, (unsigned char *)"\n");
		add_to_str(&a, &l, ce->url);
	}
	msg_box(term, getml(a, NULL), TEXT(T_CACHE_INFO), AL_LEFT | AL_EXTD_TEXT, TEXT(T_CACHE_CONTENT), a, NULL, r, 1, TEXT(T_OK), end_refresh, B_ENTER | B_ESC);
	r->win = term->windows.next;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
	/* !!! the refresh here is buggy */
}

#endif

#ifdef LEAK_DEBUG

void memory_cld(struct terminal *term, void *d)
{
	last_mem_amount = mem_amount;
}

#define MSG_BUF	2000
#define MSG_W	100

void memory_info(struct terminal *term, void *d, struct session *ses)
{
	char message[MSG_BUF];
	char *p;
	struct refresh *r;
	if (!(r = mem_alloc(sizeof(struct refresh)))) return;
	r->term = term;
	r->win = NULL;
	r->ses = ses;
	r->fn = memory_info;
	r->data = d;
	r->timer = -1;
	p = message;
	sprintf(( char *)p, "%ld %s", mem_amount, _(TEXT(T_MEMORY_ALLOCATED), term)), p += strlen(p);
	if (last_mem_amount != -1) sprintf(( char *)p, ", %s %ld, %s %ld", _(TEXT(T_LAST), term), last_mem_amount, _(TEXT(T_DIFFERENCE), term), mem_amount - last_mem_amount), p += strlen(( char *)p);
	sprintf(( char *)p, "."), p += strlen(( char *)p);
#if 0 && defined(MAX_LIST_SIZE)
	if (last_mem_amount != -1) {
		long i, j;
		int l = 0;
		for (i = 0; i < MAX_LIST_SIZE; i++) if (memory_list[i].p && memory_list[i].p != last_memory_list[i].p) {
			for (j = 0; j < MAX_LIST_SIZE; j++) if (last_memory_list[j].p == memory_list[i].p) goto b;
			if (!l) sprintf(( char *)p, "\n%s: ", _(TEXT(T_NEW_ADDRESSES), term)), p += strlen(p), l = 1;
			else sprintf(( char *)p, ", "), p += strlen(p);
			sprintf(p, "#%p of %d at %s:%d", memory_list[i].p, (int)memory_list[i].size, memory_list[i].file, memory_list[i].line), p += strlen(p);
			if (p - message >= MSG_BUF - MSG_W) {
				sprintf(p, ".."), p += strlen(p);
				break;
			}
			b:;
		}
		if (!l) sprintf(p, "\n%s", _(TEXT(T_NO_NEW_ADDRESSES), term)), p += strlen(p);
		sprintf(p, "."), p += strlen(p);
	}
#endif
	if (!(p = ( char *)stracpy((unsigned char *)message))) {
		mem_free(r);
		return;
	}
	msg_box(term, getml(p, NULL), TEXT(T_MEMORY_INFO), AL_CENTER, p, r, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
	r->win = term->windows.next;
	((struct dialog_data *)r->win->data)->dlg->abort = refresh_abort;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
}

#endif

void flush_caches(struct terminal *term, void *d, void *e)
{
	shrink_memory(SH_FREE_ALL);
}

/* jde v historii o psteps polozek dozadu */
void go_backwards(struct terminal *term, void *psteps, struct session *ses)
{
	int steps = (int) psteps;

	/*if (ses->tq_goto_position)
		--steps;
	if (ses->search_word)
		mem_free(ses->search_word), ses->search_word = NULL;*/

	while (steps > 1) {
		struct location *loc = ses->history.next;
		if ((void *) loc == &ses->history) return;
		loc = loc->next;
		if ((void *) loc == &ses->history) return;
		destroy_location(loc);

		--steps;
	}

	if (steps)
		go_back(ses);
}

struct menu_item no_hist_menu[] = {
	{ TEXT(T_NO_HISTORY), (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void history_menu(struct terminal *term, void *ddd, struct session *ses)
{
	struct location *l;
	struct menu_item *mi = NULL;
	int n = 0;
	foreach(l, ses->history) {
		if (n/* || ses->tq_goto_position*/) {
			if (!mi && !(mi = new_menu(3))) return;
			add_to_menu(&mi, stracpy(l->url), (unsigned char *)"", (unsigned char *)"", MENU_FUNC go_backwards, (void *) n, 0);
		}
		n++;
	}
	if (n <= 1) do_menu(term, no_hist_menu, ses);
	else do_menu(term, mi, ses);
}

struct menu_item no_downloads_menu[] = {
	{ TEXT(T_NO_DOWNLOADS), ( unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void downloads_menu(struct terminal *term, void *ddd, struct session *ses)
{
	struct download *d;
	struct menu_item *mi = NULL;
	int n = 0;
	foreachback(d, downloads) {
		unsigned char *u;
		if (!mi) if (!(mi = new_menu(3))) return;
		u = stracpy(d->url);
		if (strchr(( char *)u, POST_CHAR)) *strchr(( char *)u, POST_CHAR) = 0;
		add_to_menu(&mi, u, (unsigned char *)"", (unsigned char *)"", MENU_FUNC display_download, d, 0);
		n++;
	}
	if (!n) do_menu(term, no_downloads_menu, ses);
	else do_menu(term, mi, ses);
}

void menu_doc_info(struct terminal *term, void *ddd, struct session *ses)
{
	state_msg(ses);
}

void menu_head_info(struct terminal *term, void *ddd, struct session *ses)
{
	head_msg(ses);
}

void menu_toggle(struct terminal *term, void *ddd, struct session *ses)
{
	toggle(ses, ses->screen, 0);
}

void display_codepage(struct terminal *term, void *pcp, struct session *ses)
{
	int cp = (int)pcp;
	struct term_spec *t = new_term_spec(term->term);
	if (t) t->charset = cp;
	cls_redraw_all_terminals();
}

void assumed_codepage(struct terminal *term, void *pcp, struct session *ses)
{
	int cp = (int)pcp;
	ses->ds.assume_cp = cp;
	redraw_terminal_cls(term);
}

void charset_list(struct terminal *term, void *xxx, struct session *ses)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	for (i = 0; (n = get_cp_name(i)); i++) {
		if (is_cp_special(i)) continue;
		add_to_menu(&mi, get_cp_name(i), (unsigned char *)"", (unsigned char *)"", MENU_FUNC display_codepage, (void *)i, 0);
	}
	sel = ses->term->spec->charset;
	if (sel < 0) sel = 0;
	do_menu_selected(term, mi, ses, sel);
}

void set_val(struct terminal *term, void *ip, int *d)
{
	*d = (int)ip;
}

void charset_sel_list(struct terminal *term, struct session *ses, int *ptr)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	for (i = 0; (n = get_cp_name(i)); i++) {
		add_to_menu(&mi, get_cp_name(i), (unsigned char *)"", (unsigned char *)"", MENU_FUNC set_val, (void *)i, 0);
	}
	sel = *ptr;
	if (sel < 0) sel = 0;
	do_menu_selected(term, mi, ptr, sel);
}

void terminal_options_ok(void *p)
{
	cls_redraw_all_terminals();
}

unsigned char *td_labels[] = { TEXT(T_NO_FRAMES), TEXT(T_VT_100_FRAMES), TEXT(T_LINUX_OR_OS2_FRAMES), TEXT(T_KOI8R_FRAMES), TEXT(T_USE_11M), TEXT(T_RESTRICT_FRAMES_IN_CP850_852), TEXT(T_BLOCK_CURSOR), TEXT(T_COLOR), NULL };

void terminal_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	struct term_spec *ts = new_term_spec(term->term);
	if (!ts) return;
	if (!(d = mem_alloc(sizeof(struct dialog) + 11 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 11 * sizeof(struct dialog_item));
	d->title = TEXT(T_TERMINAL_OPTIONS);
	d->fn = checkbox_list_fn;
	d->udata = td_labels;
	d->refresh = (void (*)(void *))terminal_options_ok;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 1;
	d->items[0].gnum = TERM_DUMB;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&ts->mode;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 1;
	d->items[1].gnum = TERM_VT100;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&ts->mode;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 1;
	d->items[2].gnum = TERM_LINUX;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&ts->mode;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 1;
	d->items[3].gnum = TERM_KOI8;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&ts->mode;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 0;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&ts->m11_hack;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 0;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&ts->restrict_852;
	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 0;
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&ts->block_cursor;
	d->items[7].type = D_CHECKBOX;
	d->items[7].gid = 0;
	d->items[7].dlen = sizeof(int);
	d->items[7].data = (void *)&ts->col;
	d->items[8].type = D_BUTTON;
	d->items[8].gid = B_ENTER;
	d->items[8].fn = ok_dialog;
	d->items[8].text = TEXT(T_OK);
	d->items[9].type = D_BUTTON;
	d->items[9].gid = B_ESC;
	d->items[9].fn = cancel_dialog;
	d->items[9].text = TEXT(T_CANCEL);
	d->items[10].type = D_END;
 	do_dialog(term, d, getml(d, NULL));
}

#ifdef JS

unsigned char *jsopt_labels[] = { TEXT(T_KILL_ALL_SCRIPTS), TEXT(T_ENABLE_JAVASCRIPT), TEXT(T_VERBOSE_JS_ERRORS), TEXT(T_VERBOSE_JS_WARNINGS), TEXT(T_ENABLE_ALL_CONVERSIONS), TEXT(T_ENABLE_GLOBAL_NAME_RESOLUTION), TEXT(T_MANUAL_JS_CONTROL), TEXT(T_JS_RECURSION_DEPTH), TEXT(T_JS_MEMORY_LIMIT_KB), NULL };

static int __kill_script_opt;
static unsigned char js_fun_depth_str[7];
static unsigned char js_memory_limit_str[7];


static void inline __kill_js_recursively(struct f_data_c *fd)
{
	struct f_data_c *f;

	if (fd->js)js_downcall_game_over(fd->js->ctx);
	foreach(f,fd->subframes)__kill_js_recursively(f);
}


static void inline __quiet_kill_js_recursively(struct f_data_c *fd)
{
	struct f_data_c *f;

	if (fd->js)js_downcall_game_over(fd->js->ctx);
	foreach(f,fd->subframes)__quiet_kill_js_recursively(f);
}


void refresh_javascript(struct session *ses)
{
	if (ses->screen->f_data)jsint_scan_script_tags(ses->screen);
	if (__kill_script_opt)
		__kill_js_recursively(ses->screen);
	if (!js_enable) /* vypnuli jsme skribt */
	{
		if (ses->default_status)mem_free(ses->default_status),ses->default_status=NULL;
		__quiet_kill_js_recursively(ses->screen);
	}

	js_fun_depth=strtol((char *)js_fun_depth_str,0,10);
	js_memory_limit=strtol((char *)js_memory_limit_str,0,10);

	/* reparse document (muze se zmenit hodne veci) */
	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
}


void javascript_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	__kill_script_opt=0;
	snprintf((char *)js_fun_depth_str,7,"%d",js_fun_depth);
	snprintf((char *)js_memory_limit_str,7,"%d",js_memory_limit);
	if (!(d = mem_alloc(sizeof(struct dialog) + 12 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 12 * sizeof(struct dialog_item));
	d->title = TEXT(T_JAVASCRIPT_OPTIONS);
	d->fn = group_fn;
	d->refresh = (void (*)(void *))refresh_javascript;
	d->refresh_data=ses;
	d->udata = jsopt_labels;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 0;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&__kill_script_opt;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 0;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&js_enable;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 0;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&js_verbose_errors;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 0;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&js_verbose_warnings;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 0;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&js_all_conversions;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 0;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&js_global_resolve;
	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 0;
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&js_manual_confirmation;
	d->items[7].type = D_FIELD;
	d->items[7].dlen = 7;
	d->items[7].data = js_fun_depth_str;
	d->items[7].fn = check_number;
	d->items[7].gid = 1;
	d->items[7].gnum = 999999;
	d->items[8].type = D_FIELD;
	d->items[8].dlen = 7;
	d->items[8].data = js_memory_limit_str;
	d->items[8].fn = check_number;
	d->items[8].gid = 1024;
	d->items[8].gnum = 30*1024;
	d->items[9].type = D_BUTTON;
	d->items[9].gid = B_ENTER;
	d->items[9].fn = ok_dialog;
	d->items[9].text = TEXT(T_OK);
	d->items[10].type = D_BUTTON;
	d->items[10].gid = B_ESC;
	d->items[10].fn = cancel_dialog;
	d->items[10].text = TEXT(T_CANCEL);
	d->items[11].type = D_END;
 	do_dialog(term, d, getml(d, NULL));
}

#endif

unsigned char *http_labels[] = { TEXT(T_USE_HTTP_10), TEXT(T_ALLOW_SERVER_BLACKLIST), TEXT(T_BROKEN_302_REDIRECT), TEXT(T_NO_KEEPALIVE_AFTER_POST_REQUEST), TEXT(T_DO_NOT_SEND_ACCEPT_CHARSET), TEXT(T_REFERER_NONE), TEXT(T_REFERER_SAME_URL), TEXT(T_REFERER_REAL), TEXT(T_REFERER_FAKE), TEXT(T_FAKE_USERAGENT), TEXT(T_FAKE_REFERER) };

void httpopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	checkboxes_width(term, dlg->dlg->udata, &max, max_text_width);
	checkboxes_width(term, dlg->dlg->udata, &min, min_text_width);
	max_text_width(term, http_labels[8], &max, AL_LEFT);
	min_text_width(term, http_labels[8], &min, AL_LEFT);
	max_text_width(term, http_labels[9], &max, AL_LEFT);
	min_text_width(term, http_labels[9], &min, AL_LEFT);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_checkboxes(dlg, NULL, dlg->items, dlg->n - 4, 0, &y, w, &rw, dlg->dlg->udata);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, NULL, http_labels[9], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, NULL, http_labels[10], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, 2 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, term, dlg->items, dlg->n - 4, dlg->x + DIALOG_LB, &y, w, NULL, dlg->dlg->udata);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, http_labels[dlg->n - 4], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items + dlg->n - 4, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	dlg_format_text(dlg, term, http_labels[dlg->n - 3], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items + dlg->n - 3, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}


int dlg_http_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	struct http_bugs *bugs = (struct http_bugs *)di->cdata;
	struct dialog *d;
	if (!(d = mem_alloc(sizeof(struct dialog) + 14 * sizeof(struct dialog_item)))) return 0;
	memset(d, 0, sizeof(struct dialog) + 14 * sizeof(struct dialog_item));
	d->title = TEXT(T_HTTP_BUG_WORKAROUNDS);
	d->fn = httpopt_fn;
	d->udata = http_labels;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 0;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&bugs->http10;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 0;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&bugs->allow_blacklist;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 0;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&bugs->bug_302_redirect;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 0;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&bugs->bug_post_no_keepalive;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 0;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&bugs->no_accept_charset;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 1;
	d->items[5].gnum = REFERER_NONE;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&referer;
	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 1;
	d->items[6].gnum = REFERER_SAME_URL;
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&referer;
	d->items[7].type = D_CHECKBOX;
	d->items[7].gid = 1;
	d->items[7].gnum = REFERER_REAL;
	d->items[7].dlen = sizeof(int);
	d->items[7].data = (void *)&referer;
	d->items[8].type = D_CHECKBOX;
	d->items[8].gid = 1;
	d->items[8].gnum = REFERER_FAKE;
	d->items[8].dlen = sizeof(int);
	d->items[8].data = (void *)&referer;
	d->items[9].type = D_FIELD;
	d->items[9].dlen = MAX_STR_LEN;
	d->items[9].data = fake_useragent;
	d->items[10].type = D_FIELD;
	d->items[10].dlen = MAX_STR_LEN;
	d->items[10].data = fake_referer;
	d->items[11].type = D_BUTTON;
	d->items[11].gid = B_ENTER;
	d->items[11].fn = ok_dialog;
	d->items[11].text = TEXT(T_OK);
	d->items[12].type = D_BUTTON;
	d->items[12].gid = B_ESC;
	d->items[12].fn = cancel_dialog;
	d->items[12].text = TEXT(T_CANCEL);
	d->items[13].type = D_END;
 	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

unsigned char *ftp_texts[] = { TEXT(T_PASSWORD_FOR_ANONYMOUS_LOGIN), TEXT(T_USE_PASSIVE_FTP), TEXT(T_USE_FAST_FTP), NULL };

void ftpopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	max_text_width(term, ftp_texts[0], &max, AL_LEFT);
	min_text_width(term, ftp_texts[0], &min, AL_LEFT);
	checkboxes_width(term, ftp_texts + 1, &max, max_text_width);
	checkboxes_width(term, ftp_texts + 1, &min, min_text_width);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_text(dlg, NULL, ftp_texts[0], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, NULL, dlg->items + 1, 2, 0, &y, w, &rw, ftp_texts + 1);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, ftp_texts[0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, term, dlg->items + 1, 2, dlg->x + DIALOG_LB, &y, w, NULL, ftp_texts + 1);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}


int dlg_ftp_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	struct dialog *d;
	if (!(d = mem_alloc(sizeof(struct dialog) + 6 * sizeof(struct dialog_item)))) return 0;
	memset(d, 0, sizeof(struct dialog) + 6 * sizeof(struct dialog_item));
	d->title = TEXT(T_FTP_OPTIONS);
	d->fn = ftpopt_fn;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_STR_LEN;
	d->items[0].data = di->cdata;
	d->items[1].type = D_CHECKBOX;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void*)&passive_ftp;
	d->items[1].gid = 0;
	d->items[2].type = D_CHECKBOX;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void*)&fast_ftp;
	d->items[3].type = D_BUTTON;
	d->items[3].gid = B_ENTER;
	d->items[3].fn = ok_dialog;
	d->items[3].text = TEXT(T_OK);
	d->items[4].type = D_BUTTON;
	d->items[4].gid = B_ESC;
	d->items[4].fn = cancel_dialog;
	d->items[4].text = TEXT(T_CANCEL);
	d->items[5].type = D_END;
 	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

#ifdef G

#define VO_GAMMA_LEN 9
unsigned char disp_red_g[VO_GAMMA_LEN];
unsigned char disp_green_g[VO_GAMMA_LEN];
unsigned char disp_blue_g[VO_GAMMA_LEN];
unsigned char user_g[VO_GAMMA_LEN];
unsigned char aspect_str[VO_GAMMA_LEN];
int gamma_stamp; /* stamp counter for gamma changes */

extern struct list_head terminals;
extern void t_redraw(struct graphics_device *, struct rect *);

void refresh_video(struct session *ses)
{
	struct rect r = {0, 0, 0, 0};
	struct terminal *term;

	display_red_gamma=atof((char *)disp_red_g);
	display_green_gamma=atof((char *)disp_green_g);
	display_blue_gamma=atof((char *)disp_blue_g);
	user_gamma=atof((char *)user_g);
	bfu_aspect=atof((char *)aspect_str);
	update_aspect();
	gamma_stamp++;
	
	/* Flush font cache */
	destroy_font_cache();

	/* Flush dip_get_color cache */
	gamma_cache_rgb = -2;

	/* Recompute dithering tables for the new gamma */
	init_dither(drv->depth);

	shutdown_bfu();
	init_bfu();
	init_grview();

	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
	/* Redraw all terminals */
	foreach(term, terminals){
		memcpy(&r, &term->dev->size, sizeof r);
		t_redraw(term->dev, &r);
	}

}

unsigned char *video_msg[] = {
	TEXT(T_VIDEO_OPTIONS_TEXT),
	TEXT(T_RED_DISPLAY_GAMMA),
	TEXT(T_GREEN_DISPLAY_GAMMA),
	TEXT(T_BLUE_DISPLAY_GAMMA),
	TEXT(T_USER_GAMMA),
	TEXT(T_ASPECT_RATIO),
	TEXT(T_ASPECT_CORRECTION_ON),
	TEXT(T_DISPLAY_OPTIMIZATION_CRT),
	TEXT(T_DISPLAY_OPTIMIZATION_LCD_RGB),
	TEXT(T_DISPLAY_OPTIMIZATION_LCD_BGR),
	TEXT(T_DITHER_LETTERS),
	TEXT(T_DITHER_IMAGES),
	(unsigned char *)"",
	(unsigned char *)"",
};

void videoopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, video_msg[0], &max, AL_LEFT);
	min_text_width(term, video_msg[0], &min, AL_LEFT);
	max_group_width(term, video_msg + 1, dlg->items, 11, &max);
	min_group_width(term, video_msg + 1, dlg->items, 11, &min);
	max_buttons_width(term, dlg->items + 11, 2, &max);
	min_buttons_width(term, dlg->items + 11, 2, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, video_msg[0], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_group(dlg, NULL, video_msg + 1, dlg->items, 5, 0, &y, w, &rw);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, NULL, dlg->items+5, 6, dlg->x + DIALOG_LB, &y, w, &rw, video_msg+6);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + 11, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, video_msg[0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE);
	dlg_format_group(dlg, term, video_msg + 1, dlg->items, 5, dlg->x + DIALOG_LB, &y, w, NULL);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, term, dlg->items+5, 6, dlg->x + DIALOG_LB, &y, w, NULL, video_msg+6);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[11], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

void remove_zeroes(unsigned char *string)
{
	int l=strlen((char *)string);

	while(l&&(string[l-1]=='0')){
		l--;
		string[l]=0;
	}
}

void video_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	snprintf((char *)disp_red_g, VO_GAMMA_LEN, "%f", display_red_gamma);
	remove_zeroes(disp_red_g);
	snprintf((char *)disp_green_g, VO_GAMMA_LEN, "%f", display_green_gamma);
	remove_zeroes(disp_green_g);
	snprintf((char *)disp_blue_g, VO_GAMMA_LEN, "%f", display_blue_gamma);
	remove_zeroes(disp_blue_g);
	snprintf((char *)user_g, VO_GAMMA_LEN, "%f", user_gamma);
	remove_zeroes(user_g);
	snprintf((char *)aspect_str, VO_GAMMA_LEN, "%f", bfu_aspect);
	remove_zeroes(aspect_str);
	if (!(d = mem_alloc(sizeof(struct dialog) + 14 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 14 * sizeof(struct dialog_item));
	d->title = TEXT(T_VIDEO_OPTIONS);
	d->fn = videoopt_fn;
	d->refresh = (void (*)(void *))refresh_video;
	d->refresh_data = ses;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = VO_GAMMA_LEN;
	d->items[0].data = disp_red_g;
	d->items[0].fn = check_float;
	d->items[0].gid = 1;
	d->items[0].gnum = 10000;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = VO_GAMMA_LEN;
	d->items[1].data = disp_green_g;
	d->items[1].fn = check_float;
	d->items[1].gid = 1;
	d->items[1].gnum = 10000;
	d->items[2].type = D_FIELD;
	d->items[2].dlen = VO_GAMMA_LEN;
	d->items[2].data = disp_blue_g;
	d->items[2].fn = check_float;
	d->items[2].gid = 1;
	d->items[2].gnum = 10000;
	d->items[3].type = D_FIELD;
	d->items[3].dlen = VO_GAMMA_LEN;
	d->items[3].data = user_g;
	d->items[3].fn = check_float;
	d->items[3].gid = 1;
	d->items[3].gnum = 10000;

	d->items[4].type = D_FIELD;
	d->items[4].dlen = VO_GAMMA_LEN;
	d->items[4].data = aspect_str;
	d->items[4].fn = check_float;
	d->items[4].gid = 25;
	d->items[4].gnum = 400;

	d->items[5].type = D_CHECKBOX;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&aspect_on;

	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 1;
	d->items[6].gnum = 0;	/* CRT */
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&display_optimize;
	d->items[7].type = D_CHECKBOX;
	d->items[7].gid = 1;
	d->items[7].gnum = 1;	/* LCD RGB */
	d->items[7].dlen = sizeof(int);
	d->items[7].data = (void *)&display_optimize;
	d->items[8].type = D_CHECKBOX;
	d->items[8].gid = 1;
	d->items[8].gnum = 2;	/* LCD BGR*/
	d->items[8].dlen = sizeof(int);
	d->items[8].data = (void *)&display_optimize;

	
	d->items[9].type = D_CHECKBOX;
	d->items[9].gid = 0;
	d->items[9].dlen = sizeof(int);
	d->items[9].data = (void *)&dither_letters;
	d->items[10].type = D_CHECKBOX;
	d->items[10].gid = 0;
	d->items[10].dlen = sizeof(int);
	d->items[10].data = (void *)&dither_images;
	d->items[11].type = D_BUTTON;
	d->items[11].gid = B_ENTER;
	d->items[11].fn = ok_dialog;
	d->items[11].text = TEXT(T_OK);
	d->items[12].type = D_BUTTON;
	d->items[12].gid = B_ESC;
	d->items[12].fn = cancel_dialog;
	d->items[12].text = TEXT(T_CANCEL);
	d->items[13].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

#endif

unsigned char max_c_str[3];
unsigned char max_cth_str[3];
unsigned char max_t_str[3];
unsigned char time_str[5];
unsigned char unrtime_str[5];

void refresh_net(void *xxx)
{
	/*abort_all_connections();*/
	max_connections = atoi(( char *)max_c_str);
	max_connections_to_host = atoi(( char *)max_cth_str);
	max_tries = atoi(( char *)max_t_str);
	receive_timeout = atoi(( char *)time_str);
	unrestartable_receive_timeout = atoi(( char *)unrtime_str);
	register_bottom_half((void (*)(void *))check_queue, NULL);
}

unsigned char *net_msg[] = {
	TEXT(T_HTTP_PROXY__HOST_PORT),
	TEXT(T_FTP_PROXY__HOST_PORT),
	TEXT(T_MAX_CONNECTIONS),
	TEXT(T_MAX_CONNECTIONS_TO_ONE_HOST),
	TEXT(T_RETRIES),
	TEXT(T_RECEIVE_TIMEOUT_SEC),
	TEXT(T_TIMEOUT_WHEN_UNRESTARTABLE),
	TEXT(T_ASYNC_DNS_LOOKUP),
	TEXT(T_SET_TIME_OF_DOWNLOADED_FILES),
	(unsigned char *)"",
	(unsigned char *)"",
};

void netopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, net_msg[0], &max, AL_LEFT);
	min_text_width(term, net_msg[0], &min, AL_LEFT);
	max_text_width(term, net_msg[1], &max, AL_LEFT);
	min_text_width(term, net_msg[1], &min, AL_LEFT);
	max_group_width(term, net_msg + 2, dlg->items + 2, 9, &max);
	min_group_width(term, net_msg + 2, dlg->items + 2, 9, &min);
	max_buttons_width(term, dlg->items + 11, 2, &max);
	min_buttons_width(term, dlg->items + 11, 2, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, net_msg[0], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_text(dlg, NULL, net_msg[1], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_group(dlg, NULL, net_msg + 2, dlg->items + 2, 9, 0, &y, w, &rw);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + 11, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, net_msg[0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, net_msg[1], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[1], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_group(dlg, term, net_msg + 2, &dlg->items[2], 9, dlg->x + DIALOG_LB, &y, w, NULL);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[11], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

void net_options(struct terminal *term, void *xxx, void *yyy)
{
	struct dialog *d;
	snprint(max_c_str, 3, max_connections);
	snprint(max_cth_str, 3, max_connections_to_host);
	snprint(max_t_str, 3, max_tries);
	snprint(time_str, 5, receive_timeout);
	snprint(unrtime_str, 5, unrestartable_receive_timeout);
	if (!(d = mem_alloc(sizeof(struct dialog) + 14 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 14 * sizeof(struct dialog_item));
	d->title = TEXT(T_NETWORK_OPTIONS);
	d->fn = netopt_fn;
	d->refresh = (void (*)(void *))refresh_net;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_STR_LEN;
	d->items[0].data = http_proxy;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = MAX_STR_LEN;
	d->items[1].data = ftp_proxy;
	d->items[2].type = D_FIELD;
	d->items[2].data = max_c_str;
	d->items[2].dlen = 3;
	d->items[2].fn = check_number;
	d->items[2].gid = 1;
	d->items[2].gnum = 99;
	d->items[3].type = D_FIELD;
	d->items[3].data = max_cth_str;
	d->items[3].dlen = 3;
	d->items[3].fn = check_number;
	d->items[3].gid = 1;
	d->items[3].gnum = 99;
	d->items[4].type = D_FIELD;
	d->items[4].data = max_t_str;
	d->items[4].dlen = 3;
	d->items[4].fn = check_number;
	d->items[4].gid = 0;
	d->items[4].gnum = 16;
	d->items[5].type = D_FIELD;
	d->items[5].data = time_str;
	d->items[5].dlen = 5;
	d->items[5].fn = check_number;
	d->items[5].gid = 1;
	d->items[5].gnum = 1800;
	d->items[6].type = D_FIELD;
	d->items[6].data = unrtime_str;
	d->items[6].dlen = 5;
	d->items[6].fn = check_number;
	d->items[6].gid = 1;
	d->items[6].gnum = 1800;
	d->items[7].type = D_CHECKBOX;
	d->items[7].data = (unsigned char *)&async_lookup;
	d->items[7].dlen = sizeof(int);
	d->items[8].type = D_CHECKBOX;
	d->items[8].data = (unsigned char *)&download_utime;
	d->items[8].dlen = sizeof(int);
	d->items[9].type = D_BUTTON;
	d->items[9].gid = 0;
	d->items[9].fn = dlg_http_options;
	d->items[9].text = TEXT(T_HTTP_OPTIONS);
	d->items[9].data = (unsigned char *)&http_bugs;
	d->items[9].dlen = sizeof(struct http_bugs);
	d->items[10].type = D_BUTTON;
	d->items[10].gid = 0;
	d->items[10].fn = dlg_ftp_options;
	d->items[10].text = TEXT(T_FTP_OPTIONS);
	d->items[10].data = default_anon_pass;
	d->items[10].dlen = MAX_STR_LEN;
	d->items[11].type = D_BUTTON;
	d->items[11].gid = B_ENTER;
	d->items[11].fn = ok_dialog;
	d->items[11].text = TEXT(T_OK);
	d->items[12].type = D_BUTTON;
	d->items[12].gid = B_ESC;
	d->items[12].fn = cancel_dialog;
	d->items[12].text = TEXT(T_CANCEL);
	d->items[13].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

unsigned char *prg_msg[] = {
	TEXT(T_MAILTO_PROG),
	TEXT(T_SHELL_PROG),
	TEXT(T_TELNET_PROG),
	TEXT(T_TN3270_PROG),
	(unsigned char *)""
};
void netprog_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	int a;
	max_text_width(term, prg_msg[0], &max, AL_LEFT);
	min_text_width(term, prg_msg[0], &min, AL_LEFT);
	a=2;
#ifdef G
	if (F)
	{
		a=1;
		max_text_width(term, prg_msg[a], &max, AL_LEFT);
		min_text_width(term, prg_msg[a++], &min, AL_LEFT);
		max_text_width(term, prg_msg[a], &max, AL_LEFT);
		min_text_width(term, prg_msg[a++], &min, AL_LEFT);
		max_text_width(term, prg_msg[a], &max, AL_LEFT);
		min_text_width(term, prg_msg[a++], &min, AL_LEFT);
		max_buttons_width(term, dlg->items + 4, 2, &max);
		min_buttons_width(term, dlg->items + 4, 2, &min);
	}
	else
#endif
	{
		max_text_width(term, prg_msg[a], &max, AL_LEFT);
		min_text_width(term, prg_msg[a++], &min, AL_LEFT);
		max_text_width(term, prg_msg[a], &max, AL_LEFT);
		min_text_width(term, prg_msg[a++], &min, AL_LEFT);
		max_buttons_width(term, dlg->items + 3, 2, &max);
		min_buttons_width(term, dlg->items + 3, 2, &min);
	}
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, prg_msg[0], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	a=2;
#ifdef G
	if (F)
	{
		a=1;
		dlg_format_text(dlg, NULL, prg_msg[a++], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(2, G_BFU_FONT_SIZE * 2);
		dlg_format_text(dlg, NULL, prg_msg[a++], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(2, G_BFU_FONT_SIZE * 2);
		dlg_format_text(dlg, NULL, prg_msg[a++], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(2, G_BFU_FONT_SIZE * 2);
		dlg_format_buttons(dlg, NULL, dlg->items + 4, 2, 0, &y, w, &rw, AL_CENTER);
	}
	else
#endif
	{
		dlg_format_text(dlg, NULL, prg_msg[a++], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(2, G_BFU_FONT_SIZE * 2);
		dlg_format_text(dlg, NULL, prg_msg[a++], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(2, G_BFU_FONT_SIZE * 2);
		dlg_format_buttons(dlg, NULL, dlg->items + 3, 2, 0, &y, w, &rw, AL_CENTER);
	}
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, prg_msg[0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	a=2;
#ifdef G
	if (F)
	{	
		a=1;
		dlg_format_text(dlg, term, prg_msg[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, &dlg->items[a++], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, term, prg_msg[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, &dlg->items[a++], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, term, prg_msg[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, &dlg->items[a++], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_buttons(dlg, term, &dlg->items[4], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	}
	else
#endif
	{
		dlg_format_text(dlg, term, prg_msg[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, &dlg->items[a++-1], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, term, prg_msg[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, &dlg->items[a++-1], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_buttons(dlg, term, &dlg->items[3], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	}
}

void net_programs(struct terminal *term, void *xxx, void *yyy)
{
	struct dialog *d;
	int a;
	if (!(d = mem_alloc(sizeof(struct dialog) + 7 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 7 * sizeof(struct dialog_item));
#ifdef G
	if (F) d->title = TEXT(T_MAIL_TELNET_AND_SHELL_PROGRAMS);
	else
#endif
		d->title = TEXT(T_MAIL_AND_TELNET_PROGRAMS);

	d->fn = netprog_fn;
	d->items[a=0].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&mailto_prog);
#ifdef G
	if (F)
	{
		d->items[a].type = D_FIELD;
		d->items[a].dlen = MAX_STR_LEN;
		d->items[a++].data = drv->shell;
	}
#endif
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&telnet_prog);
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&tn3270_prog);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a++].text = TEXT(T_OK);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a++].text = TEXT(T_CANCEL);
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

/*void net_opt_ask(struct terminal *term, void *xxx, void *yyy)
{
	if (list_empty(downloads)) {
		net_options(term, xxx, yyy);
		return;
	}
	msg_box(term, NULL, _("Network options"), AL_CENTER, _("Warning: configuring network will terminate all running downloads. Do you really want to configure network?"), term, 2, _("Yes"), (void (*)(void *))net_options, B_ENTER, _("No"), NULL, B_ESC);
}*/

unsigned char mc_str[8];
#ifdef G
unsigned char ic_str[8];
#endif
unsigned char doc_str[4];

void cache_refresh(void *xxx)
{
	memory_cache_size = atoi(( char *)mc_str) * 1024;
#ifdef G
	if (F)image_cache_size = atoi(( char *)ic_str) * 1024;
#endif
	max_format_cache_entries = atoi(( char *)doc_str);
	shrink_memory(SH_CHECK_QUOTA);
}

unsigned char *cache_texts[] = { TEXT(T_MEMORY_CACHE_SIZE__KB), TEXT(T_NUMBER_OF_FORMATTED_DOCUMENTS), TEXT(T_AGGRESSIVE_CACHE) };
#ifdef G
unsigned char *g_cache_texts[] = { TEXT(T_MEMORY_CACHE_SIZE__KB), TEXT(T_IMAGE_CACHE_SIZE__KB), TEXT(T_NUMBER_OF_FORMATTED_DOCUMENTS), TEXT(T_AGGRESSIVE_CACHE) };
#endif

void cache_opt(struct terminal *term, void *xxx, void *yyy)
{
	struct dialog *d;
	int a;
	snprint(mc_str, 8, memory_cache_size / 1024);
#ifdef G
	if(F)snprint(ic_str, 8, image_cache_size / 1024);
#endif
	snprint(doc_str, 4, max_format_cache_entries);
#ifdef G
	if (F)
	{
		if (!(d = mem_alloc(sizeof(struct dialog) + 7 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 7 * sizeof(struct dialog_item));
	}else
#endif
	{
		if (!(d = mem_alloc(sizeof(struct dialog) + 6 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 6 * sizeof(struct dialog_item));
	}
	a=0;
	d->title = TEXT(T_CACHE_OPTIONS);
	d->fn = group_fn;
#ifdef G
	if (F)d->udata = g_cache_texts;
	else
#endif
	d->udata=cache_texts;
	d->refresh = (void (*)(void *))cache_refresh;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = 8;
	d->items[a].data = mc_str;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a].gnum = MAXINT;
	a++;
#ifdef G
	if (F)
	{
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 8;
		d->items[a].data = ic_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 0;
		d->items[a].gnum = MAXINT;
		a++;
	}
#endif
	d->items[a].type = D_FIELD;
	d->items[a].dlen = 4;
	d->items[a].data = doc_str;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a].gnum = 256;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&http_bugs.aggressive_cache;
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

void menu_shell(struct terminal *term, void *xxx, void *yyy)
{
	unsigned char *sh;
	if (!(sh = (unsigned char *)GETSHELL)) sh = (unsigned char *)DEFAULT_SHELL;
	exec_on_terminal(term, sh, (unsigned char *)"", 1);
}

void menu_kill_background_connections(struct terminal *term, void *xxx, void *yyy)
{
	abort_background_connections();
}

void menu_kill_all_connections(struct terminal *term, void *xxx, void *yyy)
{
	abort_all_connections();
}

void menu_save_html_options(struct terminal *term, void *xxx, struct session *ses)
{
	memcpy(&dds, &ses->ds, sizeof(struct document_setup));
	write_html_config(term);
}

unsigned char marg_str[2];
#ifdef G
unsigned char html_font_str[4];
unsigned char image_scale_str[6];
#endif

void html_refresh(struct session *ses)
{
	ses->ds.margin = atoi(( char *)marg_str);
#ifdef G
	if (F)
	{
		ses->ds.font_size = atoi(( char *)html_font_str);
		ses->ds.image_scale = atoi(( char *)image_scale_str);
	}
#endif
	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
}

#ifdef G
unsigned char *html_texts_g[] = { TEXT(T_DISPLAY_TABLES), TEXT(T_DISPLAY_FRAMES), TEXT(T_DISPLAY_LINKS_TO_IMAGES), TEXT(T_DISPLAY_IMAGE_FILENAMES), TEXT(T_DISPLAY_IMAGES), TEXT(T_AUTO_REFRESH), TEXT(T_TARGET_IN_NEW_WINDOW), TEXT(T_TEXT_MARGIN), (unsigned char *)"", TEXT(T_IGNORE_CHARSET_INFO_SENT_BY_SERVER), TEXT(T_USER_FONT_SIZE), TEXT(T_SCALE_ALL_IMAGES_BY) };
#endif

unsigned char *html_texts[] = { TEXT(T_DISPLAY_TABLES), TEXT(T_DISPLAY_FRAMES), TEXT(T_DISPLAY_LINKS_TO_IMAGES), TEXT(T_DISPLAY_IMAGE_FILENAMES), TEXT(T_LINK_ORDER_BY_COLUMNS), TEXT(T_NUMBERED_LINKS), TEXT(T_AUTO_REFRESH), TEXT(T_TARGET_IN_NEW_WINDOW), TEXT(T_TEXT_MARGIN), (unsigned char *)"", TEXT(T_IGNORE_CHARSET_INFO_SENT_BY_SERVER) };

int dlg_assume_cp(struct dialog_data *dlg, struct dialog_item_data *di)
{
	charset_sel_list(dlg->win->term, dlg->dlg->udata2, (int *)di->cdata);
	return 0;
}

void menu_html_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int a;
	
	snprint(marg_str, 2, ses->ds.margin);
	if (!F){
		if (!(d = mem_alloc(sizeof(struct dialog) + 14 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 14 * sizeof(struct dialog_item));
#ifdef G
	}else{
		if (!(d = mem_alloc(sizeof(struct dialog) + 16 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 16 * sizeof(struct dialog_item));
		snprintf((char *)html_font_str,4,"%d",ses->ds.font_size);
		snprintf((char *)image_scale_str,6,"%d",ses->ds.image_scale);
#endif
	}
	d->title = TEXT(T_HTML_OPTIONS);
	d->fn = group_fn;
	d->udata = gf_val(html_texts, html_texts_g);
	d->udata2 = ses;
	d->refresh = (void (*)(void *))html_refresh;
	d->refresh_data = ses;
	a=0;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.tables;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.frames;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.images;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.image_names;
	d->items[a].dlen = sizeof(int);
	a++;
#ifdef G
	if (F)
	{
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.display_images;
		d->items[a].dlen = sizeof(int);
		a++;
	}
#endif
	if (!F) {
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.table_order;
		d->items[a].dlen = sizeof(int);
		a++;
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.num_links;
		d->items[a].dlen = sizeof(int);
		a++;
	}
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.auto_refresh;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.target_in_new_window;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = 2;
	d->items[a].data = marg_str;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a].gnum = 9;
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_assume_cp;
	d->items[a].text = TEXT(T_DEFAULT_CODEPAGE);
	d->items[a].data = (unsigned char *) &ses->ds.assume_cp;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.hard_assume;
	d->items[a].dlen = sizeof(int);
	a++;
	if (!F){
		d->items[a].type = D_BUTTON;
		d->items[a].gid = B_ENTER;
		d->items[a].fn = ok_dialog;
		d->items[a].text = TEXT(T_OK);
		a++;
		d->items[a].type = D_BUTTON;
		d->items[a].gid = B_ESC;
		d->items[a].fn = cancel_dialog;
		d->items[a].text = TEXT(T_CANCEL);
		a++;
		d->items[a].type = D_END;
#ifdef G
	}else{
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 4;
		d->items[a].data = html_font_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 1;
		d->items[a].gnum = 999;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 6;
		d->items[a].data = image_scale_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 1;
		d->items[a].gnum = 500;
		a++;
		d->items[a].type = D_BUTTON;
		d->items[a].gid = B_ENTER;
		d->items[a].fn = ok_dialog;
		d->items[a].text = TEXT(T_OK);
		a++;
		d->items[a].type = D_BUTTON;
		d->items[a].gid = B_ESC;
		d->items[a].fn = cancel_dialog;
		d->items[a].text = TEXT(T_CANCEL);
		a++;
		d->items[a].type = D_END;
#endif
	}
	do_dialog(term, d, getml(d, NULL));
}

static unsigned char new_bookmarks_file[MAX_STR_LEN];
static int new_bookmarks_codepage;

#ifdef G
static unsigned char menu_font_str[4];
static unsigned char bg_color_str[7];
static unsigned char fg_color_str[7];
static unsigned char scroll_area_color_str[7];
static unsigned char scroll_bar_color_str[7];
static unsigned char scroll_frame_color_str[7];

void refresh_misc(void *ignore)
{
	if (F)
	{
		struct session *ses;

		menu_font_size=strtol((char *)menu_font_str,0,10);
		G_BFU_FG_COLOR=strtol((char *)fg_color_str,0,16);
		G_BFU_BG_COLOR=strtol((char *)bg_color_str,0,16);
		G_SCROLL_BAR_AREA_COLOR=strtol((char *)scroll_area_color_str,0,16);
		G_SCROLL_BAR_BAR_COLOR=strtol((char *)scroll_bar_color_str,0,16);
		G_SCROLL_BAR_FRAME_COLOR=strtol((char *)scroll_frame_color_str,0,16);
		shutdown_bfu();
		init_bfu();
		init_grview();
		foreach(ses, sessions) {
			ses->term->dev->resize_handler(ses->term->dev);
		}
	}
	if (strcmp((char *)new_bookmarks_file,(char *)bookmarks_file)||new_bookmarks_codepage!=bookmarks_codepage)
	{
		save_bookmarks();
		strncpy((char *)bookmarks_file,(char *)new_bookmarks_file,MAX_STR_LEN);
		bookmarks_codepage=new_bookmarks_codepage;
		reinit_bookmarks();
	}
}

unsigned char *miscopt_labels_g[] = { TEXT(T_MENU_FONT_SIZE), TEXT(T_ENTER_COLORS_AS_RGB_TRIPLETS), TEXT(T_MENU_FOREGROUND_COLOR), TEXT(T_MENU_BACKGROUND_COLOR), TEXT(T_SCROLL_BAR_AREA_COLOR), TEXT(T_SCROLL_BAR_BAR_COLOR), TEXT(T_SCROLL_BAR_FRAME_COLOR), TEXT(T_BOOKMARKS_FILE), NULL };
#endif
unsigned char *miscopt_labels[] = { TEXT(T_BOOKMARKS_FILE), NULL };


void miscopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	unsigned char **labels=dlg->dlg->udata;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	int a=0;
	int bmk=!anonymous;

#ifdef G
	if (F&&((drv->flags)&GD_NEED_CODEPAGE))a=1;
#endif

	max_text_width(term, labels[F?7:0], &max, AL_LEFT);
	min_text_width(term, labels[F?7:0], &min, AL_LEFT);
	if (F)
	{
		max_text_width(term, labels[1], &max, AL_LEFT);
		min_text_width(term, labels[1], &min, AL_LEFT);
		max_group_width(term, labels, dlg->items, 1, &max);
		min_group_width(term, labels, dlg->items, 1, &min);
		max_group_width(term, labels, dlg->items+2, 5, &max);
		min_group_width(term, labels, dlg->items+2, 5, &min);
	}
	if (bmk)
	{
		max_buttons_width(term, dlg->items + dlg->n - 3 - a, 1, &max);
		min_buttons_width(term, dlg->items + dlg->n - 3 - a, 1, &min);
	}
	if (a)
	{
		max_buttons_width(term, dlg->items + dlg->n - 3, 1, &max);
		min_buttons_width(term, dlg->items + dlg->n - 3, 1, &min);
	}
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;

	if (F)
	{
		dlg_format_group(dlg, NULL, labels, dlg->items,1,dlg->x + DIALOG_LB, &y, w, &rw);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, NULL, labels[1], dlg->x + DIALOG_LB, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_group(dlg, NULL, labels+2, dlg->items+1,5,dlg->x + DIALOG_LB, &y, w, &rw);
		y += gf_val(1, G_BFU_FONT_SIZE);
	}
	if (bmk)
	{
		dlg_format_text(dlg, NULL, labels[F?7:0], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, NULL, dlg->items + dlg->n - 4 - a, dlg->x + DIALOG_LB, &y, w, &rw, AL_LEFT);
	}
	y += gf_val(1, G_BFU_FONT_SIZE);
	if (bmk)dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 3 - a, 1, 0, &y, w, &rw, AL_LEFT);
	if (a) dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 3, 1, 0, &y, w, &rw, AL_LEFT);
	dlg_format_buttons(dlg, NULL, dlg->items +dlg->n-2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	if (F)
	{
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_group(dlg, term, labels, dlg->items,1,dlg->x + DIALOG_LB, &y, w, NULL);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, term, labels[1], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_group(dlg, term, labels+2, dlg->items+1,5,dlg->x + DIALOG_LB, &y, w, NULL);
		y += gf_val(1, G_BFU_FONT_SIZE);
	} else y += gf_val(1, G_BFU_FONT_SIZE);
	if (bmk)
	{
		dlg_format_text(dlg, term, labels[F?7:0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, dlg->items + dlg->n - 4 - a, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_buttons(dlg, term, dlg->items + dlg->n - 3 - a, 1, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	}
	if (a) dlg_format_buttons(dlg, term, dlg->items + dlg->n - 3, 1, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	dlg_format_buttons(dlg, term, dlg->items+dlg->n-2, 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}


void miscelaneous_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int a=0;

	if (anonymous&&!F) return;	/* if you add something into text mode (or both text and graphics), remove this */

	strncpy(( char *)new_bookmarks_file,( char *)bookmarks_file,MAX_STR_LEN);
	new_bookmarks_codepage=bookmarks_codepage;
	if (!F)
	{
		if (!(d = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
#ifdef G
	}
	else
	{
		if (!(d = mem_alloc(sizeof(struct dialog) + 12 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 12 * sizeof(struct dialog_item));
		snprintf((char *)menu_font_str,4,"%d",menu_font_size);
		snprintf((char *)fg_color_str,7,"%06x",(unsigned) G_BFU_FG_COLOR);
		snprintf((char *)bg_color_str,7,"%06x",(unsigned) G_BFU_BG_COLOR);
		snprintf((char *)scroll_bar_color_str,7,"%06x",(unsigned) G_SCROLL_BAR_BAR_COLOR);
		snprintf((char *)scroll_area_color_str,7,"%06x",(unsigned) G_SCROLL_BAR_AREA_COLOR);
		snprintf((char *)scroll_frame_color_str,7,"%06x",(unsigned) G_SCROLL_BAR_FRAME_COLOR);
#endif
	}
	d->title = TEXT(T_MISCELANEOUS_OPTIONS);
#ifdef G
	d->refresh = (void (*)(void *))refresh_misc;
#endif
	d->fn=miscopt_fn;
	if (!F)
		d->udata = miscopt_labels;
#ifdef G
	else 
		d->udata = miscopt_labels_g;
#endif
	d->udata2 = ses;
#ifdef G
	if (F)
	{
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 4;
		d->items[a].data = menu_font_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 1;
		d->items[a].gnum = 999;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = fg_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = bg_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = scroll_area_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = scroll_bar_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = scroll_frame_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
	}
#endif
	if (!anonymous)
	{
		d->items[a].type = D_FIELD;
		d->items[a].dlen = MAX_STR_LEN;
		d->items[a].data = new_bookmarks_file;
		a++;
		d->items[a].type = D_BUTTON;
		d->items[a].gid = 0;
		d->items[a].fn = dlg_assume_cp;
		d->items[a].text = TEXT(T_BOOKMARKS_ENCODING);
		d->items[a].data = (unsigned char *) &new_bookmarks_codepage;
		d->items[a].dlen = sizeof(int);
		a++;
	}
#ifdef G
		if (F&&((drv->flags)&GD_NEED_CODEPAGE))
		{
			d->items[a].type = D_BUTTON;
			d->items[a].gid = 0;
			d->items[a].fn = dlg_assume_cp;
			d->items[a].text = TEXT(T_KEYBOARD_CODEPAGE);
			d->items[a].data = (unsigned char *) &(drv->codepage);
			d->items[a].dlen = sizeof(int);
			a++;
		}
#endif
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT(T_CANCEL);
	a++;
	d->items[a].type = D_END;
 	do_dialog(term, d, getml(d, NULL));
}

void menu_set_language(struct terminal *term, void *pcp, struct session *ses)
{
	set_language((int)pcp);
	cls_redraw_all_terminals();
}

void menu_language_list(struct terminal *term, void *xxx, struct session *ses)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	for (i = 0; i < n_languages(); i++) {
		n = language_name(i);
		add_to_menu(&mi, n, (unsigned char *)"", (unsigned char *)"", MENU_FUNC menu_set_language, (void *)i, 0);
	}
	sel = current_language;
	do_menu_selected(term, mi, ses, sel);
}

unsigned char *resize_texts[] = { TEXT(T_COLUMNS), TEXT(T_ROWS) };

unsigned char x_str[4];
unsigned char y_str[4];

void do_resize_terminal(struct terminal *term)
{
	unsigned char str[8];
	strcpy(( char *)str, ( char *)x_str);
	strcat(( char *)str, ",");
	strcat(( char *)str, ( char *)y_str);
	do_terminal_function(term, TERM_FN_RESIZE, str);
}

void dlg_resize_terminal(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int x = term->x > 999 ? 999 : term->x;
	int y = term->y > 999 ? 999 : term->y;
	sprintf(( char *)x_str, "%d", x);
	sprintf(( char *)y_str, "%d", y);
	if (!(d = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
	d->title = TEXT(T_RESIZE_TERMINAL);
	d->fn = group_fn;
	d->udata = resize_texts;
	d->refresh = (void (*)(void *))do_resize_terminal;
	d->refresh_data = term;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = 4;
	d->items[0].data = x_str;
	d->items[0].fn = check_number;
	d->items[0].gid = 1;
	d->items[0].gnum = 999;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = 4;
	d->items[1].data = y_str;
	d->items[1].fn = check_number;
	d->items[1].gid = 1;
	d->items[1].gnum = 999;
	d->items[2].type = D_BUTTON;
	d->items[2].gid = B_ENTER;
	d->items[2].fn = ok_dialog;
	d->items[2].text = TEXT(T_OK);
	d->items[3].type = D_BUTTON;
	d->items[3].gid = B_ESC;
	d->items[3].fn = cancel_dialog;
	d->items[3].text = TEXT(T_CANCEL);
	d->items[4].type = D_END;
	do_dialog(term, d, getml(d, NULL));

}

struct menu_item file_menu11[] = {
	{ TEXT(T_GOTO_URL), (unsigned char *)"g", TEXT(T_HK_GOTO_URL), MENU_FUNC menu_goto_url, (void *)0, 0, 0 },
	{ TEXT(T_GO_BACK), (unsigned char *)"z", TEXT(T_HK_GO_BACK), MENU_FUNC menu_go_back, (void *)0, 0, 0 },
	{ TEXT(T_HISTORY), (unsigned char *)">", TEXT(T_HK_HISTORY), MENU_FUNC history_menu, (void *)0, 1, 0 },
	{ TEXT(T_RELOAD), (unsigned char *)"Ctrl-R", TEXT(T_HK_RELOAD), MENU_FUNC menu_reload, (void *)0, 0, 0 },
};

#ifdef G
struct menu_item file_menu111[] = {
	{ TEXT(T_GOTO_URL), (unsigned char *)"g", TEXT(T_HK_GOTO_URL), MENU_FUNC menu_goto_url, (void *)0, 0, 0 },
	{ TEXT(T_GO_BACK), (unsigned char *)"z", TEXT(T_HK_GO_BACK), MENU_FUNC menu_go_back, (void *)0, 0, 0 },
	{ TEXT(T_HISTORY), (unsigned char *)">", TEXT(T_HK_HISTORY), MENU_FUNC history_menu, (void *)0, 1, 0 },
	{ TEXT(T_RELOAD), (unsigned char *)"Ctrl-R", TEXT(T_HK_RELOAD), MENU_FUNC menu_reload, (void *)0, 0, 0 },
};
#endif

struct menu_item file_menu12[] = {
	{ TEXT(T_BOOKMARKS), (unsigned char *)"s", TEXT(T_HK_BOOKMARKS), MENU_FUNC menu_bookmark_manager, (void *)0, 0, 0 },
	{ TEXT(T_SAVE_BOOKMARKS), (unsigned char *)"", TEXT(T_HK_SAVE_BOOKMARKS), MENU_FUNC menu_save_bookmarks, (void *)0, 0, 0 },
	/*{ TEXT(T_ADD_BOOKMARK),(unsigned char *) "a", TEXT(T_HK_ADD_BOOKMARK), MENU_FUNC menu_bookmark_manager, (void *)0, 0, 0 },*/
};

struct menu_item file_menu21[] = {
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_SAVE_AS), (unsigned char *)"", TEXT(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TEXT(T_SAVE_URL_AS), (unsigned char *)"", TEXT(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
	{ TEXT(T_SAVE_FORMATTED_DOCUMENT), (unsigned char *)"", TEXT(T_HK_SAVE_FORMATTED_DOCUMENT), MENU_FUNC menu_save_formatted, (void *)0, 0, 0 },
};

#ifdef G
struct menu_item file_menu211[] = {
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_SAVE_AS), (unsigned char *)"", TEXT(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TEXT(T_SAVE_URL_AS), (unsigned char *)"", TEXT(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
};
#endif

#ifdef G
struct menu_item file_menu211_clipb[] = {
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_SAVE_AS), (unsigned char *)"", TEXT(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TEXT(T_SAVE_URL_AS), (unsigned char *)"", TEXT(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
	{ TEXT(T_COPY_URL_LOCATION), (unsigned char *)"", TEXT(T_HK_COPY_URL_LOCATION), MENU_FUNC copy_url_location, (void *)0, 0, 0 },
};
#endif

struct menu_item file_menu22[] = {
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0} ,
	{ TEXT(T_KILL_BACKGROUND_CONNECTIONS), (unsigned char *)"", TEXT(T_HK_KILL_BACKGROUND_CONNECTIONS), MENU_FUNC menu_kill_background_connections, (void *)0, 0, 0 },
	{ TEXT(T_KILL_ALL_CONNECTIONS), (unsigned char *)"Ctrl-S", TEXT(T_HK_KILL_ALL_CONNECTIONS), MENU_FUNC menu_kill_all_connections, (void *)0, 0, 0 },
	{ TEXT(T_FLUSH_ALL_CACHES), (unsigned char *)"", TEXT(T_HK_FLUSH_ALL_CACHES), MENU_FUNC flush_caches, (void *)0, 0, 0 },
	{ TEXT(T_RESOURCE_INFO),(unsigned char *) "", TEXT(T_HK_RESOURCE_INFO), MENU_FUNC cache_inf, (void *)0, 0, 0 },
#if 0
	TEXT(T_CACHE_INFO), (unsigned char *)"", TEXT(T_HK_CACHE_INFO), MENU_FUNC list_cache, (void *)0, 0, 0,
#endif
#ifdef LEAK_DEBUG
	{ TEXT(T_MEMORY_INFO), (unsigned char *)"", TEXT(T_HK_MEMORY_INFO), MENU_FUNC memory_info, (void *)0, 0, 0 },
#endif
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
};

struct menu_item file_menu3[] = {
	{(unsigned char *) "", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_EXIT), (unsigned char *)"q", TEXT(T_HK_EXIT), MENU_FUNC exit_prog, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void do_file_menu(struct terminal *term, void *xxx, struct session *ses)
{
	int x;
	int o;
	struct menu_item *file_menu, *e, *f;
	if (!(file_menu = mem_alloc(sizeof(file_menu11) + sizeof(file_menu12) + sizeof(file_menu21) + sizeof(file_menu22) + sizeof(file_menu3) + 3 * sizeof(struct menu_item)))) return;
	e = file_menu;
	if (!F) {
		memcpy(e, file_menu11, sizeof(file_menu11));
		e += sizeof(file_menu11) / sizeof(struct menu_item);
#ifdef G
	} else {
		memcpy(e, file_menu111, sizeof(file_menu111));
		e += sizeof(file_menu111) / sizeof(struct menu_item);
#endif
	}
	if (!anonymous) {
		memcpy(e, file_menu12, sizeof(file_menu12));
		e += sizeof(file_menu12) / sizeof(struct menu_item);
	}
	if ((o = can_open_in_new(term))) {
		e->text = TEXT(T_NEW_WINDOW);
		e->rtext = (unsigned char *)(o - 1 ? ">" : "");
		e->hotkey = TEXT(T_HK_NEW_WINDOW);
		e->func = MENU_FUNC open_in_new_window;
		e->data = send_open_new_xterm;
		e->in_m = o - 1;
		e->free_i = 0;
		e++;
	}
	if (!anonymous) {
		if (!F) {
			memcpy(e, file_menu21, sizeof(file_menu21));
			e += sizeof(file_menu21) / sizeof(struct menu_item);
#ifdef G
		} else {
			if(F && term && term->dev && term->dev->drv && !strcmp((char *)term->dev->drv->name,"x")) 
			{
				memcpy(e, file_menu211_clipb, sizeof(file_menu211_clipb));
				e += sizeof(file_menu211_clipb) / sizeof(struct menu_item);
			}
			else
			{
				memcpy(e, file_menu211, sizeof(file_menu211));
				e += sizeof(file_menu211) / sizeof(struct menu_item);
			}
#endif
		}
	}
	memcpy(e, file_menu22, sizeof(file_menu22));
	e += sizeof(file_menu22) / sizeof(struct menu_item);
	/*"", "", M_BAR, NULL, NULL, 0, 0,
	TEXT(T_OS_SHELL), "", TEXT(T_HK_OS_SHELL), MENU_FUNC menu_shell, NULL, 0, 0,*/
	x = 1;
	if (!anonymous && can_open_os_shell(term->environment)) {
		e->text = TEXT(T_OS_SHELL);
		e->rtext = (unsigned char *)"";
		e->hotkey = TEXT(T_HK_OS_SHELL);
		e->func = MENU_FUNC menu_shell;
		e->data = NULL;
		e->in_m = 0;
		e->free_i = 0;
		e++;
		x = 0;
	}
	if (can_resize_window(term->environment)) {
		e->text = TEXT(T_RESIZE_TERMINAL);
		e->rtext = (unsigned char *)"";
		e->hotkey = TEXT(T_HK_RESIZE_TERMINAL);
		e->func = MENU_FUNC dlg_resize_terminal;
		e->data = NULL;
		e->in_m = 0;
		e->free_i = 0;
		e++;
		x = 0;
	}
	memcpy(e, file_menu3 + x, sizeof(file_menu3) - x * sizeof(struct menu_item));
	e += sizeof(file_menu3) / sizeof(struct menu_item);
	for (f = file_menu; f < e; f++) f->free_i = 1;
	do_menu(term, file_menu, ses);
}

struct menu_item view_menu[] = {
	{ TEXT(T_SEARCH), (unsigned char *)"/", TEXT(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TEXT(T_SEARCH_BACK), (unsigned char *)"?", TEXT(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TEXT(T_FIND_NEXT), (unsigned char *)"n", TEXT(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TEXT(T_FIND_PREVIOUS), (unsigned char *)"N", TEXT(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{(unsigned char *) "",(unsigned char *) "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_TOGGLE_HTML_PLAIN), (unsigned char *)"\\", TEXT(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TEXT(T_DOCUMENT_INFO), (unsigned char *)"=", TEXT(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TEXT(T_HEADER_INFO), (unsigned char *)"|", TEXT(T_HK_HEADER_INFO), MENU_FUNC menu_head_info, NULL, 0, 0 },
	{ TEXT(T_FRAME_AT_FULL_SCREEN), (unsigned char *)"f", TEXT(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)set_frame, 0, 0 },
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_HTML_OPTIONS), (unsigned char *)"", TEXT(T_HK_HTML_OPTIONS), MENU_FUNC menu_html_options, (void *)0, 0, 0 },
	{ TEXT(T_SAVE_HTML_OPTIONS), (unsigned char *)"", TEXT(T_HK_SAVE_HTML_OPTIONS), MENU_FUNC menu_save_html_options, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item view_menu_anon[] = {
	{ TEXT(T_SEARCH), (unsigned char *)"/", TEXT(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TEXT(T_SEARCH_BACK), (unsigned char *)"?", TEXT(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TEXT(T_FIND_NEXT), (unsigned char *)"n", TEXT(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TEXT(T_FIND_PREVIOUS), (unsigned char *)"N", TEXT(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_TOGGLE_HTML_PLAIN), (unsigned char *)"\\", TEXT(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TEXT(T_DOCUMENT_INFO), (unsigned char *)"=", TEXT(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TEXT(T_FRAME_AT_FULL_SCREEN), (unsigned char *)"f", TEXT(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)NULL, 0, 0 },
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_HTML_OPTIONS), (unsigned char *)"", TEXT(T_HK_HTML_OPTIONS), MENU_FUNC menu_html_options, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item help_menu[] = {
	{ TEXT(T_ABOUT), (unsigned char *)"", TEXT(T_HK_ABOUT), MENU_FUNC menu_about, (void *)0, 0, 0 },
	{ TEXT(T_KEYS),(unsigned char *) "", TEXT(T_HK_KEYS), MENU_FUNC menu_keys, (void *)0, 0, 0 },
	{ TEXT(T_MANUAL), (unsigned char *)"", TEXT(T_HK_MANUAL), MENU_FUNC menu_manual, (void *)0, 0, 0 },
	{ TEXT(T_HOMEPAGE),(unsigned char *) "", TEXT(T_HK_HOMEPAGE), MENU_FUNC menu_homepage, (void *)0, 0, 0 },
	{ TEXT(T_CALIBRATION),(unsigned char *) "", TEXT(T_HK_CALIBRATION), MENU_FUNC menu_calibration, (void *)0, 0, 0 },
	{ TEXT(T_COPYING), (unsigned char *)"", TEXT(T_HK_COPYING), MENU_FUNC menu_copying, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item setup_menu[] = {
	{ TEXT(T_LANGUAGE), (unsigned char *)">", TEXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT(T_CHARACTER_SET), (unsigned char *)">", TEXT(T_HK_CHARACTER_SET), MENU_FUNC charset_list, (void *)1, 1, 0 },
	{ TEXT(T_TERMINAL_OPTIONS), (unsigned char *)"", TEXT(T_HK_TERMINAL_OPTIONS), MENU_FUNC terminal_options, NULL, 0, 0 },
	{ TEXT(T_NETWORK_OPTIONS), (unsigned char *)"", TEXT(T_HK_NETWORK_OPTIONS), MENU_FUNC net_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT(T_JAVASCRIPT_OPTIONS),(unsigned char *)"", TEXT(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT(T_MISCELANEOUS_OPTIONS),(unsigned char *)"", TEXT(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ TEXT(T_CACHE), (unsigned char *)"", TEXT(T_HK_CACHE), MENU_FUNC cache_opt, NULL, 0, 0 },
	{ TEXT(T_MAIL_AND_TELNEL), (unsigned char *)"", TEXT(T_HK_MAIL_AND_TELNEL), MENU_FUNC net_programs, NULL, 0, 0 },
	{ TEXT(T_ASSOCIATIONS),(unsigned char *) "", TEXT(T_HK_ASSOCIATIONS), MENU_FUNC menu_assoc_manager, NULL, 0, 0 },
	{ TEXT(T_FILE_EXTENSIONS), (unsigned char *)"", TEXT(T_HK_FILE_EXTENSIONS), MENU_FUNC menu_ext_manager, NULL, 0, 0 },
	{(unsigned char *) "", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_SAVE_OPTIONS), (unsigned char *)"", TEXT(T_HK_SAVE_OPTIONS), MENU_FUNC write_config, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item setup_menu_anon[] = {
	{ TEXT(T_LANGUAGE), (unsigned char *)">", TEXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT(T_CHARACTER_SET), (unsigned char *)">", TEXT(T_HK_CHARACTER_SET), MENU_FUNC charset_list, (void *)1, 1, 0 },
	{ TEXT(T_TERMINAL_OPTIONS),(unsigned char *) "", TEXT(T_HK_TERMINAL_OPTIONS), MENU_FUNC terminal_options, NULL, 0, 0 },
	{ TEXT(T_NETWORK_OPTIONS), (unsigned char *)"", TEXT(T_HK_NETWORK_OPTIONS), MENU_FUNC net_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT(T_JAVASCRIPT_OPTIONS),(unsigned char *)"", TEXT(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT(T_MISCELANEOUS_OPTIONS),(unsigned char *)"", TEXT(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#ifdef G

struct menu_item setup_menu_g[] = {
	{ TEXT(T_LANGUAGE),(unsigned char *) ">", TEXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT(T_VIDEO_OPTIONS), (unsigned char *)"", TEXT(T_HK_VIDEO_OPTIONS), MENU_FUNC video_options, NULL, 0, 0 },
	{ TEXT(T_NETWORK_OPTIONS), (unsigned char *)"", TEXT(T_HK_NETWORK_OPTIONS), MENU_FUNC net_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT(T_JAVASCRIPT_OPTIONS),(unsigned char *)"", TEXT(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT(T_MISCELANEOUS_OPTIONS),(unsigned char *)"", TEXT(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ TEXT(T_CACHE), (unsigned char *)"", TEXT(T_HK_CACHE), MENU_FUNC cache_opt, NULL, 0, 0 },
	{ TEXT(T_MAIL_TELNET_AND_SHELL), (unsigned char *)"", TEXT(T_HK_MAIL_AND_TELNEL), MENU_FUNC net_programs, NULL, 0, 0 },
	{ TEXT(T_ASSOCIATIONS),(unsigned char *) "", TEXT(T_HK_ASSOCIATIONS), MENU_FUNC menu_assoc_manager, NULL, 0, 0 },
	{ TEXT(T_FILE_EXTENSIONS), (unsigned char *)"", TEXT(T_HK_FILE_EXTENSIONS), MENU_FUNC menu_ext_manager, NULL, 0, 0 },
	{ (unsigned char *)"", (unsigned char *)"", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT(T_SAVE_OPTIONS), (unsigned char *)"", TEXT(T_HK_SAVE_OPTIONS), MENU_FUNC write_config, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item setup_menu_anon_g[] = {
	{ TEXT(T_LANGUAGE), (unsigned char *)">", TEXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT(T_VIDEO_OPTIONS), (unsigned char *)"", TEXT(T_HK_VIDEO_OPTIONS), MENU_FUNC video_options, NULL, 0, 0 },
	{ TEXT(T_NETWORK_OPTIONS), (unsigned char *)"", TEXT(T_HK_NETWORK_OPTIONS), MENU_FUNC net_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT(T_JAVASCRIPT_OPTIONS),(unsigned char *)"", TEXT(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT(T_MISCELANEOUS_OPTIONS),(unsigned char *)"", TEXT(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#endif

void do_view_menu(struct terminal *term, void *xxx, struct session *ses)
{
	if (!anonymous) do_menu(term, view_menu, ses);
	else do_menu(term, view_menu_anon, ses);
}

void do_setup_menu(struct terminal *term, void *xxx, struct session *ses)
{
#ifdef G
	if (F) if (!anonymous) do_menu(term, setup_menu_g, ses);
	else do_menu(term, setup_menu_anon_g, ses);
	else
#endif
	if (!anonymous) do_menu(term, setup_menu, ses);
	else do_menu(term, setup_menu_anon, ses);
}

struct menu_item main_menu[] = {
	{ TEXT(T_FILE),(unsigned char *) "", TEXT(T_HK_FILE), MENU_FUNC do_file_menu, NULL, 1, 1 },
	{ TEXT(T_VIEW),(unsigned char *) "", TEXT(T_HK_VIEW), MENU_FUNC do_view_menu, NULL, 1, 1 },
	{ TEXT(T_LINK), (unsigned char *)"", TEXT(T_HK_LINK), MENU_FUNC link_menu, NULL, 1, 1 },
	{ TEXT(T_DOWNLOADS), (unsigned char *)"", TEXT(T_HK_DOWNLOADS), MENU_FUNC downloads_menu, NULL, 1, 1 },
	{ TEXT(T_SETUP), (unsigned char *)"", TEXT(T_HK_SETUP), MENU_FUNC do_setup_menu, NULL, 1, 1 },
	{ TEXT(T_HELP),(unsigned char *) "", TEXT(T_HK_HELP), MENU_FUNC do_menu, help_menu, 1, 1 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

/*
#ifdef G
struct menu_item main_menu_g[] = {
	{ TEXT(T_FILE), "", TEXT(T_HK_FILE), MENU_FUNC do_file_menu, NULL, 1, 1 },
	{ TEXT(T_VIEW), "", TEXT(T_HK_VIEW), MENU_FUNC do_view_menu, NULL, 1, 1 },
	{ TEXT(T_DOWNLOADS), "", TEXT(T_HK_DOWNLOADS), MENU_FUNC downloads_menu, NULL, 1, 1 },
	{ TEXT(T_SETUP), "", TEXT(T_HK_SETUP), MENU_FUNC do_setup_menu, NULL, 1, 1 },
	{ TEXT(T_HELP), "", TEXT(T_HK_HELP), MENU_FUNC do_menu, help_menu, 1, 1 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};
#endif
*/

/* lame technology rulez ! */

void activate_bfu_technology(struct session *ses, int item)
{
	struct terminal *term = ses->term;
	do_mainmenu(term, /*gf_val(*/main_menu/*, main_menu_g)*/, ses, item);
}

struct history goto_url_history = { 0, { &goto_url_history.items, &goto_url_history.items } };

void dialog_goto_url(struct session *ses, char *url)
{
	input_field(ses->term, NULL, TEXT(T_GOTO_URL), TEXT(T_ENTER_URL), TEXT(T_OK), TEXT(T_CANCEL), ses, &goto_url_history, MAX_INPUT_URL_LEN,(unsigned char *) url, 0, 0, NULL, (void (*)(void *, unsigned char *)) goto_url, NULL);
}

void dialog_save_url(struct session *ses)
{
	input_field(ses->term, NULL, TEXT(T_SAVE_URL), TEXT(T_ENTER_URL), TEXT(T_OK), TEXT(T_CANCEL), ses, &goto_url_history, MAX_INPUT_URL_LEN, (unsigned char *)"", 0, 0, NULL, (void (*)(void *, unsigned char *)) save_url, NULL);
}

struct history file_history = { 0, { &file_history.items, &file_history.items } };

void query_file(struct session *ses, unsigned char *url, void (*std)(struct session *, unsigned char *), void (*cancel)(struct session *))
{
	unsigned char *file, *def;
	int dfl = 0;
	int l;
	get_filename_from_url(url, &file, &l);
	def = init_str();
	add_to_str(&def, &dfl, download_dir);
	if (*def && !dir_sep(def[strlen((char *)def) - 1])) add_chr_to_str(&def, &dfl, '/');
	add_bytes_to_str(&def, &dfl, file, l);
	input_field(ses->term, NULL, TEXT(T_DOWNLOAD), TEXT(T_SAVE_TO_FILE), TEXT(T_OK), TEXT(T_CANCEL), ses, &file_history, MAX_INPUT_URL_LEN, def, 0, 0, NULL, (void (*)(void *, unsigned char *))std, (void (*)(void *))cancel);
	mem_free(def);
}

struct history search_history = { 0, { &search_history.items, &search_history.items } };

void search_back_dlg(struct session *ses, struct f_data_c *f, int a)
{
	input_field(ses->term, NULL, TEXT(T_SEARCH_BACK), TEXT(T_SEARCH_FOR_TEXT), TEXT(T_OK), TEXT(T_CANCEL), ses, &search_history, MAX_INPUT_URL_LEN, (unsigned char *)"", 0, 0, NULL, (void (*)(void *, unsigned char *)) search_for_back, NULL);
}

void search_dlg(struct session *ses, struct f_data_c *f, int a)
{
	input_field(ses->term, NULL, TEXT(T_SEARCH), TEXT(T_SEARCH_FOR_TEXT), TEXT(T_OK), TEXT(T_CANCEL), ses, &search_history, MAX_INPUT_URL_LEN, (unsigned char *)"", 0, 0, NULL, (void (*)(void *, unsigned char *)) search_for, NULL);
}

void free_history_lists(void)
{
	free_list(goto_url_history.items);
	free_list(file_history.items);
	free_list(search_history.items);
#ifdef JS
	free_list(js_get_string_history.items);   /* is in jsint.c */
#endif
}

