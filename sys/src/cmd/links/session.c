/* session.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct list_head downloads = {&downloads, &downloads};

int are_there_downloads(void)
{
	int d = 0;
	struct download *down;
	foreach(down, downloads) if (!down->prog) d = 1;
	return d;
}

struct list_head sessions = {&sessions, &sessions};

struct strerror_val {
	struct strerror_val *next;
	struct strerror_val *prev;
	unsigned char msg[1];
};

struct list_head strerror_buf = { &strerror_buf, &strerror_buf };

void free_strerror_buf(void)
{
	free_list(strerror_buf);
}

unsigned char *get_err_msg(int state)
{
	unsigned char *e;
	struct strerror_val *s;
	if (state <= S_OK || state >= S_WAIT) {
		int i;
		for (i = 0; msg_dsc[i].msg; i++)
			if (msg_dsc[i].n == state) return msg_dsc[i].msg;
		unk:
		return TEXT(T_UNKNOWN_ERROR);
	}
	if (!(e = (unsigned char *)strerror(-state)) || !*e) goto unk;
	foreach(s, strerror_buf) if (!strcmp(( char *)s->msg, ( char *)e)) return s->msg;
	if (!(s = mem_alloc(sizeof(struct strerror_val) + strlen(( char *)e) + 1))) goto unk;
	strcpy(( char *)s->msg, ( char *)e);
	add_to_list(strerror_buf, s);
	return s->msg;
}

void add_xnum_to_str(unsigned char **s, int *l, int n)
{
	unsigned char suff = 0;
	int d = -1;
	if (n >= 1000000) suff = 'M', d = (n / 100000) % 10, n /= 1000000;
	else if (n >= 1000) suff = 'k', d = (n / 100) % 10, n /= 1000;
	add_num_to_str(s, l, n);
	if (n < 10 && d != -1) add_chr_to_str(s, l, '.'), add_num_to_str(s, l, d);
	add_chr_to_str(s, l, ' ');
	if (suff) add_chr_to_str(s, l, suff);
	add_chr_to_str(s, l, 'B');
}

void add_time_to_str(unsigned char **s, int *l, ttime t)
{
	unsigned char q[64];
	t /= 1000;
	t &= 0xffffffff;
	if (t < 0) t = 0;
	if (t >= 86400) sprintf(( char *)q, "%dd ", (int)(t / 86400)), add_to_str(s, l, q);
	if (t >= 3600) t %= 86400, sprintf(( char *)q, "%d:%02d", (int)(t / 3600), (int)(t / 60 % 60)), add_to_str(s, l, q);
	else sprintf(( char *)q, "%d", (int)(t / 60)), add_to_str(s, l, q);
	sprintf(( char *)q, ":%02d", (int)(t % 60)), add_to_str(s, l, q);
}

unsigned char *get_stat_msg(struct status *stat, struct terminal *term)
{
	if (stat->state == S_TRANS && stat->prg->elapsed / 100) {
		unsigned char *m = init_str();
		int l = 0;
		add_to_str(&m, &l, _(TEXT(T_RECEIVED), term));
		add_to_str(&m, &l, (unsigned char *)" ");
		add_xnum_to_str(&m, &l, stat->prg->pos);
		if (stat->prg->size >= 0)
			add_to_str(&m, &l, (unsigned char *)" "), add_to_str(&m, &l, _(TEXT(T_OF), term)), add_to_str(&m, &l, (unsigned char *)" "), add_xnum_to_str(&m, &l, stat->prg->size);
		add_to_str(&m, &l, (unsigned char *)", ");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME)
			add_to_str(&m, &l, _(TEXT(T_AVG), term)), add_to_str(&m, &l, (unsigned char *)" ");
		add_xnum_to_str(&m, &l, stat->prg->loaded * 10 / (stat->prg->elapsed / 100));
		add_to_str(&m, &l, (unsigned char *)"/s");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME) 
			add_to_str(&m, &l, (unsigned char *)", "), add_to_str(&m, &l, _(TEXT(T_CUR), term)), add_to_str(&m, &l, (unsigned char *)" "),
			add_xnum_to_str(&m, &l, stat->prg->cur_loaded / (CURRENT_SPD_SEC * SPD_DISP_TIME / 1000)),
			add_to_str(&m, &l, (unsigned char *)"/s");
		return m;
	}
	return stracpy(_(get_err_msg(stat->state), term));
}

void change_screen_status(struct session *ses)
{
	struct status *stat = NULL;
	if (ses->rq) stat = &ses->rq->stat;
	else {
		struct f_data_c *fd = current_frame(ses);
		if (fd->rq) stat = &fd->rq->stat;
		if (stat&& stat->state == S_OK && fd->af) {
			struct additional_file *af;
			foreach(af, fd->af->af) {
				if (af->rq && af->rq->stat.state >= 0) stat = &af->rq->stat;
			}
		}
	}
	if (ses->st) mem_free(ses->st);

	/* default status se ukazuje, kdyz 
	 * 			a) by se jinak ukazovalo prazdno
	 * 			b) neni NULL a ukazovalo by se OK
	 */
	ses->st = NULL;
	if (stat) {
		if (stat->state == S_OK)ses->st = print_current_link(ses);
		if (!ses->st) ses->st = ses->default_status?stracpy(ses->default_status):get_stat_msg(stat, ses->term);
	} else ses->st = stracpy(ses->default_status);
}

void _print_screen_status(struct terminal *term, struct session *ses)
{
	unsigned char *m;
	if (!F) {
		fill_area(term, 0, term->y - 1, term->x, 1, COLOR_STATUS_BG);
		if (ses->st) print_text(term, 0, term->y - 1, strlen(( char *)ses->st), ses->st, COLOR_STATUS);
		fill_area(term, 0, 0, term->x, 1, COLOR_TITLE_BG);
		if ((m = print_current_title(ses))) {
			int p = term->x - 1 - strlen(( char *)m);
			if (p < 0) p = 0;
			print_text(term, p, 0, strlen(( char *)m), m, COLOR_TITLE);
			mem_free(m);
		}
#ifdef G
	} else {
		int l = 0;
		if (ses->st) g_print_text(drv, term->dev, 0, term->y - G_BFU_FONT_SIZE, bfu_style_wb_mono, ses->st, &l);
		drv->fill_area(term->dev, l, term->y - G_BFU_FONT_SIZE, term->x, term->y, bfu_bg_color);
#endif
	}
}

void print_screen_status(struct session *ses)
{
	unsigned char *m;
	draw_to_window(ses->win, (void (*)(struct terminal *, void *))_print_screen_status, ses);
	if ((m = stracpy((unsigned char *)"Links"))) {
		if (ses->screen && ses->screen->f_data && ses->screen->f_data->title && ses->screen->f_data->title[0]) add_to_strn(&m, (unsigned char *)" - "), add_to_strn(&m, ses->screen->f_data->title);
		set_terminal_title(ses->term, m);
		/*mem_free(m); -- set_terminal_title frees it */
	}
}

void print_error_dialog(struct session *ses, struct status *stat, unsigned char *title)
{
	unsigned char *t = get_err_msg(stat->state);
	unsigned char *u = stracpy(title);
	if (strchr(( char *)u, POST_CHAR)) *strchr(( char *)u, POST_CHAR) = 0;
	if (!t) return;
	msg_box(ses->term, getml(u, NULL), TEXT(T_ERROR), AL_CENTER | AL_EXTD_TEXT, TEXT(T_ERROR_LOADING), " ", u, ":\n\n", t, NULL, ses, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC/*, _("Retry"), NULL, 0 !!! FIXME: retry */);
}

static inline unsigned char hx(int a)
{
	return a >= 10 ? a + 'A' - 10 : a + '0';
}

static inline int unhx(unsigned char a)
{
	if (a >= '0' && a <= '9') return a - '0';
	if (a >= 'A' && a <= 'F') return a - 'A' + 10;
	if (a >= 'a' && a <= 'f') return a - 'a' + 10;
	return -1;
}

unsigned char *encode_url(unsigned char *url)
{
	unsigned char *u = init_str();
	int l = 0;
	for (; *url; url++) {
		if (is_safe_in_shell(*url) && *url != '+') add_chr_to_str(&u, &l, *url);
		else add_chr_to_str(&u, &l, '+'), add_chr_to_str(&u, &l, hx(*url >> 4)), add_chr_to_str(&u, &l, hx(*url & 0xf));
	}
	return u;
}

unsigned char *decode_url(unsigned char *url)
{
	unsigned char *u = init_str();
	int l = 0;
	for (; *url; url++) {
		if (*url != '+' || unhx(url[1]) == -1 || unhx(url[2]) == -1) add_chr_to_str(&u, &l, *url);
		else add_chr_to_str(&u, &l, (unhx(url[1]) << 4) + unhx(url[2])), url += 2;
	}
	return u;
}

struct session *get_download_ses(struct download *down)
{
	struct session *ses;
	foreach(ses, sessions) if (ses == down->ses) return ses;
	if (!list_empty(sessions)) return sessions.next;
	return NULL;
}

void abort_download(struct download *down)
{
	if (down->win) delete_window(down->win);
	if (down->ask) delete_window(down->ask);
	if (down->stat.state >= 0) change_connection(&down->stat, NULL, PRI_CANCEL);
	mem_free(down->url);
	if (down->handle != -1) prealloc_truncate(down->handle, down->last_pos), close(down->handle);
	if (down->prog) {
		unlink(( char *)down->file);
		mem_free(down->prog);
	}
	mem_free(down->file);
	del_from_list(down);
	mem_free(down);
}

void abort_and_delete_download(struct download *down)
{
	if (down->win) delete_window(down->win);
	if (down->ask) delete_window(down->ask);
	if (down->stat.state >= 0) change_connection(&down->stat, NULL, PRI_CANCEL);
	mem_free(down->url);
	if (down->handle != -1) prealloc_truncate(down->handle, down->last_pos), close(down->handle);
	unlink(( char *)down->file);
	if (down->prog)
		mem_free(down->prog);
	mem_free(down->file);
	del_from_list(down);
	mem_free(down);
}

void kill_downloads_to_file(unsigned char *file)
{
	struct download *down;
	foreach(down, downloads) if (!strcmp(( char *)down->file,( char *) file)) down = down->prev, abort_download(down->next);
}

void undisplay_download(struct download *down)
{
	if (down->win) delete_window(down->win);
}

int dlg_abort_download(struct dialog_data *dlg, struct dialog_item_data *di)
{
	register_bottom_half((void (*)(void *))abort_download, dlg->dlg->udata);
	return 0;
}

int dlg_abort_and_delete_download(struct dialog_data *dlg, struct dialog_item_data *di)
{
	register_bottom_half((void (*)(void *))abort_and_delete_download, dlg->dlg->udata);
	return 0;
}

int dlg_undisplay_download(struct dialog_data *dlg, struct dialog_item_data *di)
{
	register_bottom_half((void (*)(void *))undisplay_download, dlg->dlg->udata);
	return 0;
}

void download_abort_function(struct dialog_data *dlg)
{
	struct download *down = dlg->dlg->udata;
	down->win = NULL;
}

void download_window_function(struct dialog_data *dlg)
{
	struct download *down = dlg->dlg->udata;
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, x, y;
	int t = 0;
	unsigned char *m, *u;
	struct status *stat = &down->stat;
	if (!F) redraw_below_window(dlg->win);
	down->win = dlg->win;
	if (stat->state == S_TRANS && stat->prg->elapsed / 100) {
		int l = 0;
		m = init_str();
		t = 1;
		add_to_str(&m, &l, _(TEXT(T_RECEIVED), term));
		add_to_str(&m, &l, (unsigned char *)" ");
		add_xnum_to_str(&m, &l, stat->prg->pos);
		if (stat->prg->size >= 0)
			add_to_str(&m, &l, (unsigned char *)" "), add_to_str(&m, &l, _(TEXT(T_OF),term)), add_to_str(&m, &l,(unsigned char *) " "), add_xnum_to_str(&m, &l, stat->prg->size), add_to_str(&m, &l, (unsigned char *)" ");
		add_to_str(&m, &l, (unsigned char *)"\n");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME)
			add_to_str(&m, &l, _(TEXT(T_AVERAGE_SPEED), term));
		else add_to_str(&m, &l, _(TEXT(T_SPEED), term));
		add_to_str(&m, &l, (unsigned char *)" ");
		add_xnum_to_str(&m, &l, (longlong)stat->prg->loaded * 10 / (stat->prg->elapsed / 100));
		add_to_str(&m, &l, (unsigned char *)"/s");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME) 
			add_to_str(&m, &l, (unsigned char *)", "), add_to_str(&m, &l, _(TEXT(T_CURRENT_SPEED), term)), add_to_str(&m, &l, (unsigned char *)" "),
			add_xnum_to_str(&m, &l, stat->prg->cur_loaded / (CURRENT_SPD_SEC * SPD_DISP_TIME / 1000)),
			add_to_str(&m, &l, (unsigned char *)"/s");
		add_to_str(&m, &l, (unsigned char *)"\n");
		add_to_str(&m, &l, _(TEXT(T_ELAPSED_TIME), term));
		add_to_str(&m, &l, (unsigned char *)" ");
		add_time_to_str(&m, &l, stat->prg->elapsed);
		if (stat->prg->size >= 0 && stat->prg->loaded > 0) {
			add_to_str(&m, &l, (unsigned char *)", ");
			add_to_str(&m, &l, _(TEXT(T_ESTIMATED_TIME), term));
			add_to_str(&m, &l, (unsigned char *)" ");
			/*add_time_to_str(&m, &l, stat->prg->elapsed / 1000 * stat->prg->size / stat->prg->loaded * 1000 - stat->prg->elapsed);*/
			add_time_to_str(&m, &l, (stat->prg->size - stat->prg->pos) / ((longlong)stat->prg->loaded * 10 / (stat->prg->elapsed / 100)) * 1000);
		}
	} else m = stracpy(_(get_err_msg(stat->state), term));
	if (!m) return;
	u = stracpy(down->url);
	if (strchr(( char *)u, POST_CHAR)) *strchr(( char *)u, POST_CHAR) = 0;
	max_text_width(term, u, &max, AL_LEFT);
	min_text_width(term, u, &min, AL_LEFT);
	max_text_width(term, m, &max, AL_LEFT);
	min_text_width(term, m, &min, AL_LEFT);
	max_buttons_width(term, dlg->items, dlg->n, &max);
	min_buttons_width(term, dlg->items, dlg->n, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (t && stat->prg->size >= 0) {
		if (w < DOWN_DLG_MIN) w = DOWN_DLG_MIN;
	} else {
		if (w > max) w = max;
	}
	if (w < 1) w = 1;
	y = 0;
	dlg_format_text(dlg, NULL, u, 0, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	if (t && stat->prg->size >= 0) y += gf_val(2, 2 * G_BFU_FONT_SIZE);
	dlg_format_text(dlg, NULL, m, 0, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items, dlg->n, 0, &y, w, NULL, AL_CENTER);
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	x = dlg->x + DIALOG_LB;
	dlg_format_text(dlg, term, u, x, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	if (t && stat->prg->size >= 0) {
		if (!F) {
			unsigned char q[64];
			int p = w - 6;
			y++;
			set_only_char(term, x, y, '[');
			set_only_char(term, x + w - 5, y, ']');
			fill_area(term, x + 1, y, (int)((longlong)p * (longlong)stat->prg->pos / (longlong)stat->prg->size), 1, COLOR_DIALOG_METER);
			sprintf(( char *)q, "%3d%%", (int)((longlong)100 * (longlong)stat->prg->pos / (longlong)stat->prg->size));
			print_text(term, x + w - 4, y, strlen(( char *)q), q, COLOR_DIALOG_TEXT);
			y++;
#ifdef G
		} else {
			unsigned char q[64];
			int p, s, ss, m;
			y += G_BFU_FONT_SIZE;
			sprintf((char *)q, "] %3d%%", (int)((longlong)100 * (longlong)stat->prg->pos / (longlong)stat->prg->size));
			s = g_text_width(bfu_style_bw_mono, (unsigned char *)"[");
			ss = g_text_width(bfu_style_bw_mono, q);
			p = w - s - ss;
			if (p < 0) p = 0;
			m = (int)((longlong)p * (longlong)stat->prg->pos / (longlong)stat->prg->size);
			g_print_text(drv, term->dev, x, y, bfu_style_bw_mono, (unsigned char *)"[", NULL);
			drv->fill_area(term->dev, x + s, y, x + s + m, y + G_BFU_FONT_SIZE, bfu_fg_color);
			drv->fill_area(term->dev, x + s + m, y, x + s + p, y + G_BFU_FONT_SIZE, bfu_bg_color);
			g_print_text(drv, term->dev, x + w - ss, y, bfu_style_bw_mono, q, NULL);
			if (dlg->s) exclude_from_set(&dlg->s, x, y, x + w, y + G_BFU_FONT_SIZE);
			y += G_BFU_FONT_SIZE;
#endif
		}
	}
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, m, x, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items, dlg->n, x, &y, w, NULL, AL_CENTER);
	mem_free(u);
	mem_free(m);
}


void display_download(struct terminal *term, struct download *down, struct session *ses)
{
	struct dialog *dlg;
	struct download *dd;
	foreach(dd, downloads) if (dd == down) goto found;
	return;
	found:
	if (!(dlg = mem_alloc(sizeof(struct dialog) + 4 * sizeof(struct dialog_item)))) return;
	memset(dlg, 0, sizeof(struct dialog) + 4 * sizeof(struct dialog_item));
	undisplay_download(down);
	down->ses = ses;
	dlg->title = TEXT(T_DOWNLOAD);
	dlg->fn = download_window_function;
	dlg->abort = download_abort_function;
	dlg->udata = down;
	dlg->align = AL_CENTER;
	dlg->items[0].type = D_BUTTON;
	dlg->items[0].gid = B_ENTER | B_ESC;
	dlg->items[0].fn = dlg_undisplay_download;
	dlg->items[0].text = TEXT(T_BACKGROUND);
	dlg->items[1].type = D_BUTTON;
	dlg->items[1].gid = 0;
	dlg->items[1].fn = dlg_abort_download;
	dlg->items[1].text = TEXT(T_ABORT);
	if (!down->prog) {
		dlg->items[2].type = D_BUTTON;
		dlg->items[2].gid = 0;
		dlg->items[2].fn = dlg_abort_and_delete_download;
		dlg->items[2].text = TEXT(T_ABORT_AND_DELETE_FILE);
		dlg->items[3].type = D_END;
	} else {
		dlg->items[2].type = D_END;
	}
	do_dialog(term, dlg, getml(dlg, NULL));
}

time_t parse_http_date(char *date)	/* this functions is bad !!! */
{
	static char *months[12] =
		{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	time_t t = 0;
	/* Mon, 03 Jan 2000 21:29:33 GMT */
	int y;
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));

	date = strchr(date, ' ');
	if (!date) return 0;
	date++;
	if (*date >= '0' && *date <= '9') {
			/* Sun, 06 Nov 1994 08:49:37 GMT */
			/* Sunday, 06-Nov-94 08:49:37 GMT */
		y = 0;
		if (date[0] < '0' || date[0] > '9') return 0;
		if (date[1] < '0' || date[1] > '9') return 0;
		tm.tm_mday = (date[0] - '0') * 10 + date[1] - '0';
		date += 2;
		if (*date != ' ' && *date != '-') return 0;
		date += 1;
		for (tm.tm_mon = 0; tm.tm_mon < 12; tm.tm_mon++)
			if (!casecmp((unsigned char *)date, (unsigned char *)months[tm.tm_mon], 3)) goto f1;
		return 0;
		f1:
		date += 3;
		if (*date == ' ') {
				/* Sun, 06 Nov 1994 08:49:37 GMT */
			date++;
			if (date[0] < '0' || date[0] > '9') return 0;
			if (date[1] < '0' || date[1] > '9') return 0;
			if (date[2] < '0' || date[2] > '9') return 0;
			if (date[3] < '0' || date[3] > '9') return 0;
			tm.tm_year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 + (date[2] - '0') * 10 + date[3] - '0' - 1900;
			date += 4;
		} else if (*date == '-') {
				/* Sunday, 06-Nov-94 08:49:37 GMT */
			date++;
			if (date[0] < '0' || date[0] > '9') return 0;
			if (date[1] < '0' || date[1] > '9') return 0;
			tm.tm_year = (date[0] >= '7' ? 1900 : 2000) + (date[0] - '0') * 10 + date[1] - '0' - 1900;
			date += 2;
		} else return 0;
		if (*date != ' ') return 0;
		date++;
	} else {
			/* Sun Nov  6 08:49:37 1994 */
		y = 1;
		for (tm.tm_mon = 0; tm.tm_mon < 12; tm.tm_mon++)
			if (!casecmp((unsigned char *)date, (unsigned char *)months[tm.tm_mon], 3)) goto f2;
		return 0;
		f2:
		date += 3;
		while (*date == ' ') date++;
		if (date[0] < '0' || date[0] > '9') return 0;
		tm.tm_mday = date[0] - '0';
		date++;
		if (*date != ' ') {
			if (date[0] < '0' || date[0] > '9') return 0;
			tm.tm_mday = tm.tm_mday * 10 + date[0] - '0';
			date++;
		}
		if (*date != ' ') return 0;
		date++;
	}

	if (date[0] < '0' || date[0] > '9') return 0;
	if (date[1] < '0' || date[1] > '9') return 0;
	tm.tm_hour = (date[0] - '0') * 10 + date[1] - '0';
	date += 2;
	if (*date != ':') return 0;
	date++;
	if (date[0] < '0' || date[0] > '9') return 0;
	if (date[1] < '0' || date[1] > '9') return 0;
	tm.tm_min = (date[0] - '0') * 10 + date[1] - '0';
	date += 2;
	if (*date != ':') return 0;
	date++;
	if (date[0] < '0' || date[0] > '9') return 0;
	if (date[1] < '0' || date[1] > '9') return 0;
	tm.tm_sec = (date[0] - '0') * 10 + date[1] - '0';
	date += 2;
	if (y) {
		if (*date != ' ') return 0;
		date++;
		if (date[0] < '0' || date[0] > '9') return 0;
		if (date[1] < '0' || date[1] > '9') return 0;
		if (date[2] < '0' || date[2] > '9') return 0;
		if (date[3] < '0' || date[3] > '9') return 0;
		tm.tm_year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 + (date[2] - '0') * 10 + date[3] - '0' - 1900;
		date += 4;
	}
	if (*date != ' ' && *date) return 0;

	t = mktime(&tm);
	if (t == (time_t) -1) return 0;
	return t;
}

void download_data(struct status *stat, struct download *down)
{
	struct cache_entry *ce;
	struct fragment *frag;
	if (stat->state >= S_WAIT && stat->state < S_TRANS) goto end_store;
	if (!(ce = stat->ce)) goto end_store;
	if (ce->last_modified)
	down->remotetime = parse_http_date(( char *)ce->last_modified);
/*	  fprintf(stderr,"\nFEFE date %s\n",ce->last_modified); */
	if (ce->redirect && down->redirect_cnt++ < MAX_REDIRECTS) {
		unsigned char *u, *p;
		if (stat->state >= 0) change_connection(&down->stat, NULL, PRI_CANCEL);
		u = join_urls(down->url, ce->redirect);
		if (!u) goto x;
		if (!http_bugs.bug_302_redirect) if (!ce->redirect_get && (p = (unsigned char *)strchr(( char *)down->url, POST_CHAR))) add_to_strn(&u, p);
		mem_free(down->url);
		down->url = u;
		down->stat.state = S_WAIT_REDIR;
		if (down->win) {
			struct event ev = { EV_REDRAW, 0, 0, 0 };
			ev.x = down->win->term->x;
			ev.y = down->win->term->y;
			down->win->handler(down->win, &ev, 0);
		}
		/*if (!strchr(down->url, POST_CHAR)) {*/
			load_url(down->url, NULL, &down->stat, PRI_DOWNLOAD, NC_CACHE);
			return;
		/*} else {
			unsigned char *msg = init_str();
			int l = 0;
			add_bytes_to_str(&msg, &l, down->url, (unsigned char *)strchr(down->url, POST_CHAR) - down->url);
			msg_box(get_download_ses(down)->term, getml(msg, NULL), TEXT(T_WARNING), AL_CENTER | AL_EXTD_TEXT, TEXT(T_DO_YOU_WANT_TO_FOLLOW_REDIRECT_AND_POST_FORM_DATA_TO_URL), "", msg, "?", NULL, down, 3, TEXT(T_YES), down_post_yes, B_ENTER, TEXT(T_NO), down_post_no, 0, TEXT(T_CANCEL), down_post_cancel, B_ESC);
		}*/
	}
	x:
	foreach(frag, ce->frag) if (frag->offset <= down->last_pos && frag->offset + frag->length > down->last_pos) {
		int w;
#ifdef HAVE_OPEN_PREALLOC
		if (!down->last_pos && (!down->stat.prg || down->stat.prg->size > 0)) {
			close(down->handle);
			down->handle = open_prealloc(down->file, O_CREAT|O_WRONLY|O_TRUNC, 0666, down->stat.prg ? down->stat.prg->size : ce->length);
			if (down->handle == -1) goto error;
			set_bin(down->handle);
		}
#endif
		w = write(down->handle, frag->data + down->last_pos - frag->offset, frag->length - (down->last_pos - frag->offset));
		if (w == -1) {
#ifdef HAVE_OPEN_PREALLOC
			error:
#endif
			detach_connection(stat, down->last_pos);
			if (!list_empty(sessions)) {
				unsigned char *msg = stracpy(down->file);
				unsigned char *emsg = stracpy((unsigned char *)strerror(errno));
				msg_box(get_download_ses(down)->term, getml(msg, emsg, NULL), TEXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT, TEXT(T_COULD_NOT_WRITE_TO_FILE), " ", msg, ": ", emsg, NULL, NULL, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC);
			}
			abort_download(down);
			return;
		}
		down->last_pos += w;
	}
	detach_connection(stat, down->last_pos);
	end_store:;
	if (stat->state < 0) {
		if (stat->state != S_OK) {
			unsigned char *t = get_err_msg(stat->state);
			if (t) {
				unsigned char *tt = stracpy(down->url);
				if (strchr(( char *)tt, POST_CHAR)) *strchr(( char *)tt, POST_CHAR) = 0;
				msg_box(get_download_ses(down)->term, getml(tt, NULL), TEXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT, TEXT(T_ERROR_DOWNLOADING), " ", tt, ":\n\n", t, NULL, get_download_ses(down), 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC/*, TEXT(T_RETRY), NULL, 0 !!! FIXME: retry */);
			}
		} else {
			if (down->prog) {
				prealloc_truncate(down->handle, down->last_pos);
				close(down->handle), down->handle = -1;
				exec_on_terminal(get_download_ses(down)->term, down->prog, down->file, !!down->prog_flags);
				mem_free(down->prog), down->prog = NULL;
			} else if (down->remotetime && download_utime) {
				struct utimbuf foo;
				foo.actime = foo.modtime = down->remotetime;
				utime(( char *)down->file, &foo);
			}
		}
		abort_download(down);
		return;
	}
	if (down->win) {
		struct event ev = { EV_REDRAW, 0, 0, 0 };
		ev.x = down->win->term->x;
		ev.y = down->win->term->y;
		down->win->handler(down->win, &ev, 0);
	}
}

int create_download_file(struct terminal *term, unsigned char *fi, int safe)
{
	unsigned char *file = fi;
	unsigned char *wd;
	int h;
	int i;
#ifdef NO_FILE_SECURITY
	int sf = 0;
#else
	int sf = safe;
#endif
	wd = get_cwd();
	set_cwd(term->cwd);
	if (file[0] == '~' && dir_sep(file[1])) {
		unsigned char *h = (unsigned char *)getenv("HOME");
		if (h) {
			int fl = 0;
			if ((file = init_str())) {
				add_to_str(&file, &fl, h);
				add_to_str(&file, &fl, fi + 1);
			}
		}
	}
	h = open(( char *)file, O_CREAT|O_WRONLY|O_TRUNC|(sf?O_EXCL:0), sf ? 0600 : 0666);
	if (wd) set_cwd(wd), mem_free(wd);
	/* FIXME */
	#if 0
	if (h == -1&&errno==EEXIST) {
		unsigned char *msg = stracpy(file);
		unsigned char *msge = stracpy(strerror(errno));
		msg_box(term, getml(msg, msge, NULL), TEXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT, TEXT(T_COULD_NOT_CREATE_FILE), " ", msg, ": ", msge, NULL, NULL, 2,  TEXT(T_YES), NULL, B_ENTER, TEXT(T_NO), NULL, B_ESC);
		if (file != fi) mem_free(file);
		return -1;
	}
	#endif
	if (h == -1) {
		unsigned char *msg = stracpy(file);
		unsigned char *msge = stracpy((unsigned char *)strerror(errno));
		msg_box(term, getml(msg, msge, NULL), TEXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT, TEXT(T_COULD_NOT_CREATE_FILE), " ", msg, ": ", msge, NULL, NULL, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		if (file != fi) mem_free(file);
		return -1;
	}
	set_bin(h);
	if (safe) goto x;
	if (strlen(( char *)file) > MAX_STR_LEN) memcpy(download_dir, file, MAX_STR_LEN - 1), download_dir[MAX_STR_LEN - 1] = 0;
	else strcpy(( char *)download_dir, ( char *)file);
	for (i = strlen(( char *)download_dir) - 1; i >= 0; i--) if (dir_sep(download_dir[i])) {
		download_dir[i + 1] = 0;
		goto x;
	}
	download_dir[0] = 0;
	x:
	if (file != fi) mem_free(file);
	return h;
}

unsigned char *get_temp_name(unsigned char *url)
{
	int l, nl;
	unsigned char *name, *fn, *fnn, *fnnn, *s;
	unsigned char *nm = (unsigned char *)tmpnam(0);
	if (!nm) return NULL;
	name = init_str();
	nl = 0;
	add_to_str(&name, &nl, nm);
	free(nm);
	get_filename_from_url(url, &fn, &l);
	fnnn = NULL;
	for (fnn = fn; fnn < fn + l; fnn++) if (*fnn == '.') fnnn = fnn;
	if (fnnn && (s = memacpy(fnnn, l - (fnnn - fn)))) {
		check_shell_security(&s);
		add_to_str(&name, &nl, s);
		mem_free(s);
	}
	return name;
}

unsigned char *subst_file(unsigned char *prog, unsigned char *file)
{
	unsigned char *n = init_str();
	int l = 0;
	while (*prog) {
		int p;
		for (p = 0; prog[p] && prog[p] != '%'; p++) ;
		add_bytes_to_str(&n, &l, prog, p);
		prog += p;
		if (*prog == '%') {
#if defined(HAVE_CYGWIN_CONV_TO_FULL_WIN32_PATH)
#ifdef MAX_PATH
			unsigned char new_path[MAX_PATH];
#else
			unsigned char new_path[1024];
#endif
			cygwin_conv_to_full_win32_path(file, new_path);
			add_to_str(&n, &l, new_path);
#else
			add_to_str(&n, &l, file);
#endif
			prog++;
		}
	}
	return n;
}

int plumb(char *file)
{
	unsigned char *wd;
	char *msg;
	int fd,n;
	int msgmaxlen=500;
	wd = get_cwd();
    if(!(msg=mem_alloc(strlen(file)+msgmaxlen))) return;
	sprintf(msg,"links\n\n%s\ntext\n\n%d\n%s",wd,strlen(file),file);
	fd=open("/mnt/plumb/send",O_WRONLY);
	if(fd<0){
		return -1;
	}
	n=write(fd,msg,strlen(msg));
	close(fd);
	if(n<strlen(msg)){
		return -1;
	}
	return 0;
}

void start_download(struct session *ses, unsigned char *file)
{
	struct download *down;
	int h;
	unsigned char *url = ses->dn_url;
	if (!url) return;
	kill_downloads_to_file(url);
	if ((h = create_download_file(ses->term, file, 0)) == -1) return;
	if (!(down = mem_alloc(sizeof(struct download)))) return;
	memset(down, 0, sizeof(struct download));
	down->url = stracpy(url);
	down->stat.end = (void (*)(struct status *, void *))download_data;
	down->stat.data = down;
	down->last_pos = 0;
	down->file = stracpy(file);
	down->handle = h;
	down->ses = ses;
	down->remotetime = 0;
	add_to_list(downloads, down);
	load_url(url, NULL, &down->stat, PRI_DOWNLOAD, NC_CACHE);
	display_download(ses->term, down, ses);
	return;

}

void abort_all_downloads(void)
{
	while (!list_empty(downloads)) abort_download(downloads.next);
}

int f_is_finished(struct f_data *f)
{
	struct additional_file *af;
	if (!f || f->rq->state >= 0) return 0;
	if (f->fd && f->fd->rq && f->fd->rq->state >= 0) return 0;
	if (f->af) foreach(af, f->af->af) if (af->rq->state >= 0) return 0;
	return 1;
}

int f_need_reparse(struct f_data *f)
{
	struct additional_file *af;
	if (!f || f->rq->state >= 0) return 1;
	if (f->af) foreach(af, f->af->af) if (af->need_reparse) return 1;
	return 0;
}

struct f_data *format_html(struct f_data_c *fd, struct object_request *rq, unsigned char *url, struct document_options *opt, int *cch)
{
	struct f_data *f;
	pr(
	if (cch) *cch = 0;
	if (!rq->ce || !(f = init_formatted(opt))) goto nul;
	f->fd = fd;
	f->ses = fd->ses;
	f->time_to_get = -get_time();
	clone_object(rq, &f->rq);
	if (f->rq->ce) {
		unsigned char *start; unsigned char *end;
		int stl = -1;
		struct additional_file *af;

		if (fd->af) foreach(af, fd->af->af) af->need_reparse = 0;

		get_file(rq, &start, &end);
		if (jsint_get_source(fd, &start, &end)) f->uncacheable = 1;
		if (opt->plain == 2) {
			start = init_str();
			stl = 0;
			add_to_str(&start, &stl, (unsigned char *)"<img src=\"");
			add_to_str(&start, &stl, f->rq->ce->url);
			add_to_str(&start, &stl, (unsigned char *)"\">");
			end = start + stl;
		}
		really_format_html(f->rq->ce, start, end, f, fd != fd->ses->screen);
		if (stl != -1) mem_free(start);
		f->use_tag = f->rq->ce->count;
		if (f->af) foreach(af, f->af->af) if (af->rq->ce) af->use_tag = af->rq->ce->count;
	} else f->use_tag = 0;
	f->time_to_get += get_time();
	) nul:return NULL;
	return f;
}

void count_frames(struct f_data_c *fd, int *i)
{
	struct f_data_c *sub;
	if (!fd) return;
	if (fd->f_data) (*i)++;
	foreach(sub, fd->subframes) count_frames(sub, i);
}

long formatted_info(int type)
{
	int i = 0;
	struct session *ses;
	struct f_data *ce;
	switch (type) {
		case CI_FILES:
			foreach(ses, sessions)
				foreach(ce, ses->format_cache) i++;
			/* fall through */
		case CI_LOCKED:
			foreach(ses, sessions)
				count_frames(ses->screen, &i);
			return i;
		default:
			internal((unsigned char *)"formatted_info: bad request");
	}
	return 0;
}

void f_data_attach(struct f_data_c *fd, struct f_data *f)
{
	struct additional_file *af;
	f->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
	f->rq->data = fd;
	if (f->af) foreach(af, f->af->af) {
		af->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
		af->rq->data = fd;
	}
}

void detach_f_data(struct f_data **ff)
{
	struct f_data *f = *ff;
	if (!f) return;
	*ff = NULL;
	if (f->frame_desc_link) {
		destroy_formatted(f);
		return;
	}
	f->fd = NULL;
#ifdef G
	f->locked_on = NULL;
	free_list(f->image_refresh);
#endif
	if (f->uncacheable || !f_is_finished(f)) destroy_formatted(f);
	else add_to_list(f->ses->format_cache, f);
}

static inline int is_format_cache_entry_uptodate(struct f_data *f)
{
	struct cache_entry *ce = f->rq->ce;
	struct additional_file *af;
	if (!ce || ce->count != f->use_tag) return 0;
	if (f->af) foreach(af, f->af->af) {
		struct cache_entry *ce = af->rq->ce;
		if (af->need_reparse) if (!ce || ce->count != af->use_tag) return 0;
	}
	return 1;
}

int shrink_format_cache(int u)
{
	static int sc = 0;
	int scc;
	int r = 0;
	struct f_data *f;
	int c = 0;
	struct session *ses;
	foreach(ses, sessions) foreach(f, ses->format_cache) {
		if (u == SH_FREE_ALL || !is_format_cache_entry_uptodate(f)) {
			struct f_data *ff = f;
			f = f->prev;
			del_from_list(ff);
			destroy_formatted(ff);
			r |= ST_SOMETHING_FREED;
		} else c++;
	}
	if (u == SH_FREE_SOMETHING) c = max_format_cache_entries + 1;
	if (c > max_format_cache_entries) {
		a:
		scc = sc++;
		foreach (ses, sessions) if (!scc--) {
			foreachback(f, ses->format_cache) {
				struct f_data *ff = f;
				f = f->next;
				del_from_list(ff);
				destroy_formatted(ff);
				r |= ST_SOMETHING_FREED;
				if (--c <= max_format_cache_entries) goto ret;
			}
			goto q;
		}
		sc = 0;
		goto a;
		q:;
	}
	ret:
	return r | (!c) * ST_CACHE_EMPTY;
}

void init_fcache(void)
{
	register_cache_upcall(shrink_format_cache, (unsigned char *)"format");
}

struct f_data *cached_format_html(struct f_data_c *fd, struct object_request *rq, unsigned char *url, struct document_options *opt, int *cch)
{
	struct f_data *f;
	/*if (F) opt->tables = 0;*/
	if (fd->marginwidth != -1) {
		int marg = (fd->marginwidth + G_HTML_MARGIN - 1) / G_HTML_MARGIN;
		if (marg >= 0 && marg < 9) opt->margin = marg;
	}
	if (opt->plain == 2) opt->margin = 0, opt->display_images = 1;
	pr(
	if (!jsint_get_source(fd, NULL, NULL)) 
		foreach(f, fd->ses->format_cache) 
			if (!strcmp((char *)f->rq->url, (char *)url) && !compare_opt(&f->opt, opt)) {
				if (!is_format_cache_entry_uptodate(f)) { 
					struct f_data *ff = f;
					f = f->prev;
					del_from_list(ff);
					destroy_formatted(ff);
					continue;
				}
				del_from_list(f);
				f->fd = fd;
				if (cch) *cch = 1;
				f_data_attach(fd, f);
				xpr();
				return f;
			}
	);
	f = format_html(fd, rq, url, opt, cch);
	if (f) f->fd = fd;
	shrink_memory(SH_CHECK_QUOTA);
	return f;
}

struct f_data_c *create_f_data_c(struct session *, struct f_data_c *);
void reinit_f_data_c(struct f_data_c *);
struct location *new_location(void);

void create_new_frames(struct f_data_c *fd, struct frameset_desc *fs, struct document_options *o)
{
	struct location *loc;
	struct frame_desc *frm;
	int c_loc;
	int i;
	int x, y;
	int xp, yp;

	i = 0;
#ifdef JS
	if (fd->onload_frameset_code)mem_free(fd->onload_frameset_code);
	fd->onload_frameset_code=stracpy(fs->onload_code);
#endif
	foreach(loc, fd->loc->subframes) i++;
	if (i != fs->n) {
		while (!list_empty(fd->loc->subframes)) destroy_location(fd->loc->subframes.next);
		c_loc = 1;
	} else {
		c_loc = 0;
		loc = fd->loc->subframes.next;
	}

	yp = fd->yp;
	frm = &fs->f[0];
	for (y = 0; y < fs->y; y++) {
		xp = fd->xp;
		for (x = 0; x < fs->x; x++) {
			struct f_data_c *nfdc;
			if (!(nfdc = create_f_data_c(fd->ses, fd))) return;
			if (c_loc) {
				struct list_head *l = (struct list_head *)fd->loc->subframes.prev;
				loc = new_location();
				add_to_list(*l, loc);
				loc->parent = fd->loc;
				loc->name = stracpy(frm->name);
				if ((loc->url = stracpy(frm->url)))
					nfdc->goto_position = extract_position(loc->url);
			}
			nfdc->xp = xp; nfdc->yp = yp;
			nfdc->xw = frm->xw;
			nfdc->yw = frm->yw;
			nfdc->loc = loc;
			nfdc->vs = loc->vs;
			if (frm->marginwidth != -1) nfdc->marginwidth = frm->marginwidth;
			else nfdc->marginwidth = fd->marginwidth;
			if (frm->marginheight != -1) nfdc->marginheight = frm->marginheight;
			else nfdc->marginheight = fd->marginheight;
			/*debug("frame: %d %d, %d %d\n", nfdc->xp, nfdc->yp, nfdc->xw, nfdc->yw);*/
			{
				struct list_head *l = (struct list_head *)fd->subframes.prev;
				add_to_list(*l, nfdc);
			}
			if (frm->subframe) {
				create_new_frames(nfdc, frm->subframe, o);
				/*nfdc->f_data = init_formatted(&fd->f_data->opt);*/
				nfdc->f_data = init_formatted(o);
				nfdc->f_data->frame_desc = copy_frameset_desc(frm->subframe);
				nfdc->f_data->frame_desc_link = 1;
			} else {
				if (fd->depth < HTML_MAX_FRAME_DEPTH && loc->url && *loc->url)
					request_object(fd->ses->term, loc->url, fd->loc->url, PRI_FRAME, NC_CACHE, (void (*)(struct object_request *, void *))fd_loaded, nfdc, &nfdc->rq);
			}
			xp += frm->xw + gf_val(1, 0);
			frm++;
			if (!c_loc) loc = loc->next;
		}
		yp += (frm - 1)->yw + gf_val(1, 0);
	}
}

void html_interpret(struct f_data_c *fd)
{
	int i;
	int oxw; int oyw; int oxp; int oyp;
	struct f_data_c *sf;
	int cch;
	/*int first = !fd->f_data;*/
	struct document_options o;
	if (!fd->loc) goto d;
	if (fd->f_data) {
		oxw = fd->f_data->opt.xw;
		oyw = fd->f_data->opt.yw;
		oxp = fd->f_data->opt.xp;
		oyp = fd->f_data->opt.yp;
	} else {
		oxw = oyw = oxp = oyp = -1;
	}
	memset(&o, 0, sizeof(struct document_options));
	ds2do(&fd->ses->ds, &o);
#ifdef JS
	o.js_enable = js_enable;
#else
	o.js_enable=0;
#endif
#ifdef G
	o.aspect_on=aspect_on;
	o.bfu_aspect=bfu_aspect;
#else
	o.aspect_on=0;
	o.bfu_aspect=0;
#endif
	o.plain = fd->vs->plain;
	o.xp = fd->xp;
	o.yp = fd->yp;
	o.xw = fd->xw;
	o.yw = fd->yw;
	if (fd->ses->term->spec) {
		o.col = fd->ses->term->spec->col;
		o.cp = fd->ses->term->spec->charset;
	} else {
		o.col = 3;
		o.cp = 0;
	}
	if (!F) {
		memcpy(&o.default_fg, &default_fg, sizeof(struct rgb));
		memcpy(&o.default_bg, &default_bg, sizeof(struct rgb));
		memcpy(&o.default_link, &default_link, sizeof(struct rgb));
		memcpy(&o.default_vlink, &default_vlink, sizeof(struct rgb));
#ifdef G
	} else {
		memcpy(&o.default_fg, &default_fg_g, sizeof(struct rgb));
		memcpy(&o.default_bg, &default_bg_g, sizeof(struct rgb));
		memcpy(&o.default_link, &default_link_g, sizeof(struct rgb));
		memcpy(&o.default_vlink, &default_vlink_g, sizeof(struct rgb));
#endif
	}
	if (!(o.framename = fd->loc->name)) o.framename = NULL;
	detach_f_data(&fd->f_data);
	if (!(fd->f_data = cached_format_html(fd, fd->rq, fd->rq->url, &o, &cch))) goto d;

	/* erase frames if changed */
	i = 0;
	foreach(sf, fd->subframes) i++;
	if (i != (fd->f_data->frame_desc ? fd->f_data->frame_desc->n : 0) && (f_is_finished(fd->f_data) || !f_need_reparse(fd->f_data))) {
		rd:
		foreach(sf, fd->subframes) reinit_f_data_c(sf);
		free_list(fd->subframes);

		/* create new frames */
		if (fd->f_data->frame_desc) create_new_frames(fd, fd->f_data->frame_desc, &fd->f_data->opt);
	} else {
		if (fd->f_data->frame_desc && f_is_finished(fd->f_data)) {
			if (fd->f_data->opt.xw != oxw ||
			    fd->f_data->opt.yw != oyw ||
			    fd->f_data->opt.xp != oxp ||
			    fd->f_data->opt.yp != oyp) goto rd;
		}
	}

	d:;
	/*draw_fd(fd);*/
}

void html_interpret_recursive(struct f_data_c *f)
{
	struct f_data_c *fd;
	if (f->rq) html_interpret(f);
	foreach(fd, f->subframes) html_interpret_recursive(fd);
}

/* You get a struct_additionl_file. never mem_free it. When you stop
 * using it, just forget the pointer.
 */
struct additional_file *request_additional_file(struct f_data *f, unsigned char *url_)
{
	struct additional_file *af;
	unsigned char *u, *url;
	url = stracpy(url_);
	if ((u = extract_position(url))) mem_free(u);
	if (!f->af) {
		if (!(f->af = f->fd->af)) {
			if (!(f->af = f->fd->af = mem_calloc(sizeof(struct additional_files)))) {
				mem_free(url);
				return NULL;
			}
			f->af->refcount = 1;
			init_list(f->af->af);
		}
		f->af->refcount++;
	}
	foreach (af, f->af->af) if (!strcmp((char *)af->url, (char *)url)) {
		mem_free(url);
		return af;
	}
	if (!(af = mem_alloc(sizeof(struct additional_file) + strlen(( char *)url) + 1))) return NULL;
	af->use_tag = 0;
	strcpy((char *)af->url, (char *)url);
	request_object(f->ses->term, url, f->rq->url, PRI_IMG, NC_CACHE, f->rq->upcall, f->rq->data, &af->rq);
	af->need_reparse = 0;
	add_to_list(f->af->af, af);
	mem_free(url);
	return af;
}

#ifdef G

void image_timer(struct f_data_c *fd)
{
	struct image_refresh *ir;
	struct list_head new;
	init_list(new);
	fd->image_timer = -1;
	if (!fd->f_data) return;
	foreach (ir, fd->f_data->image_refresh) {
		if (ir->t > G_IMG_REFRESH) ir->t -= G_IMG_REFRESH;
		else {
			struct image_refresh *irr = ir->prev;
			del_from_list(ir);
			add_to_list(new, ir);
			ir = irr;
		}
	}
	foreach (ir, new) {
		/*fprintf(stderr, "DRAW: %p\n", ir->img);*/
		draw_one_object(fd, ir->img);
	}
	free_list(new);
	if (fd->image_timer == -1 && !list_empty(fd->f_data->image_refresh)) fd->image_timer = install_timer(G_IMG_REFRESH, (void (*)(void *))image_timer, fd);
}

void refresh_image(struct f_data_c *fd, struct g_object *img, ttime tm)
{
	struct image_refresh *ir;
	if (!fd->f_data) return;
	foreach (ir, fd->f_data->image_refresh) if (ir->img == img) {
		if (ir->t > tm) ir->t = tm;
		return;
	}
	if (!(ir = mem_alloc(sizeof(struct image_refresh)))) return;
	ir->img = img;
	ir->t = tm;
	add_to_list(fd->f_data->image_refresh, ir);
	if (fd->image_timer == -1) fd->image_timer = install_timer(G_IMG_REFRESH, (void (*)(void *))image_timer, fd);
}

#endif

void reinit_f_data_c(struct f_data_c *fd)
{
	struct additional_file *af;
	struct f_data_c *fd1;
	jsint_destroy(fd);
	foreach(fd1, fd->subframes) {
		if (fd->ses->wtd_target_base == fd1) fd->ses->wtd_target_base = NULL;
		reinit_f_data_c(fd1);
		if (fd->ses->wtd_target_base == fd1) fd->ses->wtd_target_base = fd;
		if (fd->ses->defered_target_base == fd1) fd->ses->defered_target_base = fd;
	}
	free_list(fd->subframes);
#ifdef JS
	if (fd->onload_frameset_code)mem_free(fd->onload_frameset_code),fd->onload_frameset_code=NULL;
#endif
	fd->loc = NULL;
	if (fd->f_data && fd->f_data->rq) fd->f_data->rq->upcall = NULL;
	if (fd->f_data && fd->f_data->af) foreach(af, fd->f_data->af->af) af->rq->upcall = NULL;
	if (fd->af) foreach(af, fd->af->af) af->rq->upcall = NULL;
	detach_f_data(&fd->f_data);
	if (fd->link_bg) mem_free(fd->link_bg), fd->link_bg = NULL;
	fd->link_bg_n = 0;
	if (fd->goto_position) mem_free(fd->goto_position), fd->goto_position = NULL;
	release_object(&fd->rq);
	free_additional_files(&fd->af);
	fd->next_update = get_time();
	fd->done = 0;
	fd->parsed_done = 0;
	if (fd->image_timer != -1) kill_timer(fd->image_timer), fd->image_timer = -1;
	if (fd->refresh_timer != -1) kill_timer(fd->refresh_timer), fd->refresh_timer = -1;
}

struct f_data_c *create_f_data_c(struct session *ses, struct f_data_c *parent)
{
	static long id = 1;
	struct f_data_c *fd;
	if (!(fd = mem_calloc(sizeof(struct f_data_c)))) return NULL;
	fd->parent = parent;
	fd->ses = ses;
	fd->depth = parent ? parent->depth + 1 : 1;
	init_list(fd->subframes);
	fd->next_update = get_time();
	fd->done = 0;
	fd->parsed_done = 0;
	fd->script_t = 0;
	fd->id = id++;
	fd->marginwidth = fd->marginheight = -1;
	fd->image_timer = -1;
	fd->refresh_timer = -1;
	return fd;
}

int plain_type(struct session *ses, struct object_request *rq, unsigned char **p)
{
	struct cache_entry *ce;
	unsigned char *ct;
	int r = 0;
	if (p) *p = NULL;
	if (!rq || !(ce = rq->ce)) {
		r = 1;
		goto f;
	}
	if (!(ct = get_content_type(ce->head, ce->url))) goto f;
	if (!strcasecmp((char *)ct, "text/html")) goto ff;
	r = 1;
	if (!strcasecmp((char *)ct, "text/plain")) goto ff;
	r = 2;
	/* !!! FIXME: tady by se mel dat test, zda to ten obrazek umi */
	if (F && !casecmp(ct, (unsigned char *)"image/", 6)) goto ff;
	r = -1;

	ff:
	if (!p) mem_free(ct);
	else *p = ct;
	f:
	return r;
}

int get_file(struct object_request *o, unsigned char **start, unsigned char **end)
{
	struct cache_entry *ce;
	struct fragment *fr;
	*start = *end = NULL;
	if (!o || !o->ce) return 1;
	ce = o->ce;
	defrag_entry(ce);
	fr = ce->frag.next;
	if ((void *)fr == &ce->frag || fr->offset || !fr->length) return 1;
	else *start = fr->data, *end = fr->data + fr->length;
	return 0;
}

void refresh_timer(struct f_data_c *fd)
{
	if (fd->ses->rq) {
		fd->refresh_timer = install_timer(500, (void (*)(void *))refresh_timer, fd);
		return;
	}
	fd->refresh_timer = -1;
	if (fd->f_data && fd->f_data->refresh) {
		fd->refresh_timer = install_timer(fd->f_data->refresh_seconds * 1000, (void (*)(void *))refresh_timer, fd);
		goto_url_f(fd->ses, NULL, fd->f_data->refresh, (unsigned char *)"_self", fd, -1, 0, 0, 1);
	}
}

static int __frame_and_all_subframes_loaded(struct f_data_c *fd)
{
	struct f_data_c *f;
	int loaded=fd->done||fd->rq==NULL;

	if (loaded)		/* this frame is loaded */
		foreach(f,fd->subframes)
		{
			loaded=__frame_and_all_subframes_loaded(f);
			if (!loaded)break;
		}
	return loaded;
}

void fd_loaded(struct object_request *rq, struct f_data_c *fd)
{
	if (fd->done) {
		if (f_is_finished(fd->f_data)) goto priint;
		else fd->done = 0, fd->parsed_done = 1;
	}
	if (fd->vs->plain == -1 && rq->state != O_WAITING) {
		fd->vs->plain = plain_type(fd->ses, fd->rq, NULL);
	}
	if (fd->rq->state < 0 && (f_is_finished(fd->f_data) || !fd->f_data)) {
		if (!fd->parsed_done) html_interpret(fd);
		draw_fd(fd);
		/* it may happen that html_interpret requests load of additional file */
		if (!f_is_finished(fd->f_data)) goto more_data;
		fn:
		if (fd->f_data->are_there_scripts) {
			jsint_scan_script_tags(fd);
			if (!f_is_finished(fd->f_data)) goto more_data;
		}
		fd->done = 1;
		fd->parsed_done = 0;
		if (fd->f_data->refresh) {
			if (fd->refresh_timer != -1) kill_timer(fd->refresh_timer);
			fd->refresh_timer = install_timer(fd->f_data->refresh_seconds * 1000, (void (*)(void *))refresh_timer, fd);
		}
#ifdef JS
		jsint_run_queue(fd);
#endif
	} else more_data: if (get_time() >= fd->next_update) {
		ttime t;
		int first = !fd->f_data;
		if (!fd->parsed_done) {
			html_interpret(fd);
			if (fd->rq->state < 0 && !f_need_reparse(fd->f_data)) fd->parsed_done = 1;
		}
		draw_fd(fd);
		if (fd->rq->state < 0 && f_is_finished(fd->f_data)) goto fn;
		t = fd->f_data ? ((fd->parsed_done ? 0 : fd->f_data->time_to_get * DISPLAY_TIME) + fd->f_data->time_to_draw * IMG_DISPLAY_TIME) : 0;
		if (t < DISPLAY_TIME_MIN) t = DISPLAY_TIME_MIN;
		if (first && t > DISPLAY_TIME_MAX_FIRST) t = DISPLAY_TIME_MAX_FIRST;
		fd->next_update = get_time() + t;
	} else {
		change_screen_status(fd->ses);
		print_screen_status(fd->ses);
	}
	priint:
	/* process onload handler of a frameset */
#ifdef JS
	{
		int all_loaded;

		/* go to parent and test if all subframes are loaded, if yes, call onload handler */

		if (!fd->parent) goto hell;	/* this frame has no parent, skip */
		if (!fd->parent->onload_frameset_code)goto hell;	/* no onload handler, skip all this */
		all_loaded=__frame_and_all_subframes_loaded(fd->parent);
		if (!all_loaded) goto hell;
		/* parent has all subframes loaded */
		jsint_execute_code(fd->parent,fd->parent->onload_frameset_code,strlen(( char *)fd->parent->onload_frameset_code),-1,-1,-1);
		mem_free(fd->parent->onload_frameset_code), fd->parent->onload_frameset_code=NULL;
	hell:;
	}
#endif
	if (rq && (rq->state == O_FAILED || rq->state == O_INCOMPLETE) && (fd->rq == rq || fd->ses->rq == rq)) print_error_dialog(fd->ses, &rq->stat, rq->url);
}

struct location *new_location(void)
{
	struct location *loc;
	if (!(loc = mem_calloc(sizeof(struct location)))) return NULL;
	init_list(loc->subframes);
	loc->vs = create_vs();
	return loc;
}

static inline struct location *alloc_ses_location(struct session *ses)
{
	struct location *loc;
	if (!(loc = new_location())) return NULL;
	add_to_list(ses->history, loc);
	return loc;
}

void subst_location(struct f_data_c *fd, struct location *old, struct location *new)
{
	struct f_data_c *f;
	foreach(f, fd->subframes) subst_location(f, old, new);
	if (fd->loc == old) fd->loc = new;
}

struct location *copy_sublocations(struct session *ses, struct location *d, struct location *s, struct location *x)
{
	struct location *dl, *sl, *y, *z;
	d->name = stracpy(s->name);
	if (s == x) return d;
	d->url = stracpy(s->url);
	d->prev_url = stracpy(s->prev_url);
	destroy_vs(d->vs);
	d->vs = s->vs; s->vs->refcount++;
	subst_location(ses->screen, s, d);
	y = NULL;
	foreach(sl, s->subframes) {
		if ((dl = new_location())) {
			struct list_head *l = (struct list_head *)d->subframes.prev;
			add_to_list(*l, dl);
			dl->parent = d;
			z = copy_sublocations(ses, dl, sl, x);
			if (z && y) internal((unsigned char *)"copy_sublocations: crossed references");
			if (z) y = z;
		}
	}
	return y;
}

struct location *copy_location(struct session *ses, struct location *loc)
{
	struct location *l2, *l1, *nl;
	l1 = cur_loc(ses);
	if (!(l2 = alloc_ses_location(ses))) return NULL;
	if (!(nl = copy_sublocations(ses, l2, l1, loc))) internal((unsigned char *)"copy_location: sublocation not found");
	return nl;
}

struct f_data_c *new_main_location(struct session *ses)
{
	struct location *loc;
	if (!(loc = alloc_ses_location(ses))) return NULL;
	reinit_f_data_c(ses->screen);
	ses->screen->loc = loc;
	ses->screen->vs = loc->vs;
	if (ses->wanted_framename) loc->name=ses->wanted_framename, ses->wanted_framename=NULL;
	return ses->screen;
}

struct f_data_c *copy_location_and_replace_frame(struct session *ses, struct f_data_c *fd)
{
	struct location *loc;
	if (!(loc = copy_location(ses, fd->loc))) return NULL;
	reinit_f_data_c(fd);
	fd->loc = loc;
	fd->vs = loc->vs;
	return fd;
}

/* vrati frame prislusici danemu targetu
   pokud takovy frame nenajde, vraci NULL
 */
struct f_data_c *find_frame(struct session *ses, unsigned char *target, struct f_data_c *base)
{
	struct f_data_c *f, *ff;
	if (!base) base = ses->screen;
	if (!target || !*target) return base;
	if (!strcasecmp((char *)target, "_blank"))
		return NULL;  /* open in new window */
	if (!strcasecmp((char *)target, "_top"))
		return ses->screen;
	if (!strcasecmp((char *)target, "_self")) return base;
	if (!strcasecmp((char *)target, "_parent")) {
		for (ff = base->parent; ff && !ff->rq; ff = ff->parent) ;
		return ff ? ff : ses->screen;
	}
	f = ses->screen;
	if (f->loc && f->loc->name && !strcasecmp((char *)f->loc->name, (char *)target)) return f;
	d:
	foreach(ff, f->subframes) if (ff->loc && ff->loc->name && !strcasecmp((char *)ff->loc->name, (char *)target)) return ff;
	if (!list_empty(f->subframes)) {
		f = f->subframes.next;
		goto d;
	}
	u:
	if (!f->parent) return NULL;
	if (f->next == (void *)&f->parent->subframes) {
		f = f->parent;
		goto u;
	}
	f = f->next;
	goto d;
}

void destroy_location(struct location *loc)
{
	while (loc->subframes.next != &loc->subframes) destroy_location(loc->subframes.next);
	del_from_list(loc);
	if (loc->name) mem_free(loc->name);
	if (loc->url) mem_free(loc->url);
	if (loc->prev_url) mem_free(loc->prev_url);
	destroy_vs(loc->vs);
	mem_free(loc);
}

void ses_go_forward(struct session *ses, int plain, int refresh)
{
	struct location *cl;
	struct f_data_c *fd;
	if (ses->search_word) mem_free(ses->search_word), ses->search_word = NULL;
	if ((fd = find_frame(ses, ses->wtd_target, ses->wtd_target_base))&&fd!=ses->screen) {
		cl = NULL;
		if (refresh && fd->loc && !strcmp((char *)fd->loc->url, (char *)ses->rq->url)) cl = cur_loc(ses);
		fd = copy_location_and_replace_frame(ses, fd);
		if (cl) destroy_location(cl);
	} else fd = new_main_location(ses);
	if (!fd) return;
	fd->vs->plain = plain;
	ses->wtd = NULL;
	fd->rq = ses->rq; ses->rq = NULL;
	fd->goto_position = ses->goto_position; ses->goto_position = NULL;
	fd->loc->url = stracpy(fd->rq->url);
	fd->loc->prev_url = stracpy(fd->rq->prev_url);
	fd->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
	fd->rq->data = fd;
	/*ses->locked_link = 0;*/
	fd->rq->upcall(fd->rq, fd);
}

void ses_go_backward(struct session *ses)
{
	if (ses->search_word) mem_free(ses->search_word), ses->search_word = NULL;
	reinit_f_data_c(ses->screen);
	destroy_location(cur_loc(ses));
	ses->locked_link = 0;
	ses->screen->loc = cur_loc(ses);
	ses->screen->vs = ses->screen->loc->vs;
	ses->wtd = NULL;
	ses->screen->rq = ses->rq; ses->rq = NULL;
	ses->screen->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
	ses->screen->rq->data = ses->screen;
	ses->screen->rq->upcall(ses->screen->rq, ses->screen);

}

void tp_cancel(struct session *ses)
{
	release_object(&ses->tq);
}

void continue_download(struct session *ses, unsigned char *file)
{
	struct download *down;
	int h;
	unsigned char *url = ses->tq->url;
	if (!url) return;
	if (ses->tq_prog) {
		if (!(file = get_temp_name(url))) {
			tp_cancel(ses);
			return;
		}
	}
	kill_downloads_to_file(file);
	if ((h = create_download_file(ses->term, file, !!ses->tq_prog)) == -1) {
		tp_cancel(ses);
		if (ses->tq_prog) mem_free(file);
		return;
	}
	if (!(down = mem_alloc(sizeof(struct download)))) {
		tp_cancel(ses);
		if (ses->tq_prog) mem_free(file);
		return;
	}
	memset(down, 0, sizeof(struct download));
	down->url = stracpy(url);
	down->stat.end = (void (*)(struct status *, void *))download_data;
	down->stat.data = down;
	down->last_pos = 0;
	down->file = stracpy(file);
	down->handle = h;
	down->ses = ses;
	if (ses->tq_prog) {
		down->prog = subst_file(ses->tq_prog, file);
		mem_free(file);
		mem_free(ses->tq_prog);
		ses->tq_prog = NULL;
	}
	down->prog_flags = ses->tq_prog_flags;
	add_to_list(downloads, down);
	release_object_get_stat(&ses->tq, &down->stat, PRI_DOWNLOAD);
	display_download(ses->term, down, ses);
	return;

}


void tp_save(struct session *ses)
{
	if (ses->tq_prog) mem_free(ses->tq_prog), ses->tq_prog = NULL;
	query_file(ses, ses->tq->url, continue_download, tp_cancel);
}

void tp_open(struct session *ses)
{
	continue_download(ses, (unsigned char *)"");
}

int ses_abort_1st_state_loading(struct session *ses)
{
	int r = !!ses->rq;
	release_object(&ses->rq);
	ses->wtd = NULL;
	if (ses->wtd_target) mem_free(ses->wtd_target), ses->wtd_target = NULL;
	ses->wtd_target_base = NULL;
	if (ses->goto_position) mem_free(ses->goto_position), ses->goto_position = NULL;
	change_screen_status(ses);
	print_screen_status(ses);
	return r;
}

void tp_display(struct session *ses)
{
	ses_abort_1st_state_loading(ses);
	ses->rq = ses->tq;
	ses->tq = NULL;
	ses_go_forward(ses, 1, 0);
}


int prog_sel_save(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	if (ses->tq_prog) mem_free(ses->tq_prog), ses->tq_prog = NULL;
	query_file(ses, ses->tq->url, continue_download, tp_cancel);

	cancel_dialog(dlg,idata);
	return 0;
}

int prog_sel_display(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	ses_abort_1st_state_loading(ses);

	ses->rq = ses->tq;
	ses->tq = NULL;
	ses_go_forward(ses, 1, 0);

	cancel_dialog(dlg,idata);
	return 0;
}


int prog_sel_cancel(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	release_object(&ses->tq);
	cancel_dialog(dlg,idata);
	return 0;
}


int prog_sel_open(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct assoc *a=(struct assoc*)idata->item->udata;
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	if (!a) internal((unsigned char *)"This should not happen.\n");
	ses->tq_prog = stracpy(a->prog), ses->tq_prog_flags = a->block;
	tp_open(ses);
	cancel_dialog(dlg,idata);
	return 0;
}


void vysad_dvere(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	/*struct session *ses=(struct session *)(dlg->dlg->udata2);*/
	unsigned char *ct=(unsigned char *)(dlg->dlg->udata);
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	unsigned char *txt;
	int l=0;

	txt=init_str();
	
	/* brainovi to tady pada !!!
	add_to_str(&txt,&l,ses->tq->url);
	add_to_str(&txt,&l,(unsigned char *)" ");
	add_to_str(&txt,&l,_(TEXT(T_HAS_TYPE),term));
	*/
	add_to_str(&txt,&l,_(TEXT(T_CONTEN_TYPE_IS),term));
	add_to_str(&txt,&l,(unsigned char *)" ");
	add_to_str(&txt,&l,ct);
	add_to_str(&txt,&l,(unsigned char *)".\n");
	if (!anonymous)
		add_to_str(&txt,&l,_(TEXT(T_DO_YOU_WANT_TO_OPEN_SAVE_OR_DISPLAY_THIS_FILE),term));
	else
		add_to_str(&txt,&l,_(TEXT(T_DO_YOU_WANT_TO_OPEN_OR_DISPLAY_THIS_FILE),term));

	max_text_width(term, txt, &max, AL_CENTER);
	min_text_width(term, txt, &min, AL_CENTER);
	max_buttons_width(term, dlg->items , dlg->n, &max);
	min_buttons_width(term, dlg->items , dlg->n, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_text(dlg, NULL, txt, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_CENTER);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items , dlg->n, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, txt, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_CENTER);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items , dlg->n, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
	mem_free(txt);
}


void vysad_okno(struct session *ses, unsigned char *ct, struct assoc *a, int n)
{
	int i;
	struct dialog *d;
	struct memory_list *ml;

	if (!(d = mem_alloc(sizeof(struct dialog) + (n+3+(!anonymous)) * sizeof(struct dialog_item)))) {
		mem_free(a);
		mem_free(ct);
		return;
	}
	memset(d, 0, sizeof(struct dialog) + (n+3+(!anonymous)) * sizeof(struct dialog_item));
	d->title = TEXT(T_WHAT_TO_DO);
	d->fn = vysad_dvere;
	d->udata = ct;
	d->udata2=ses;
	ml=getml(d,a,ct,NULL);

	for (i=0;i<n;i++)
	{
		unsigned char *bla=stracpy(_(TEXT(T_OPEN_WITH),ses->term));
		add_to_strn(&bla,(unsigned char *)" ");
		add_to_strn(&bla,_(a[i].label,ses->term));
		
		d->items[i].type = D_BUTTON;
		d->items[i].fn = prog_sel_open;
		d->items[i].udata = a+i;
		d->items[i].text = bla;
		add_to_ml(&ml,bla,NULL);
	}
	if (!anonymous)
	{
		d->items[i].type = D_BUTTON;
		d->items[i].fn = prog_sel_save;
		d->items[i].text = TEXT(T_SAVE);
		i++;
	}
	d->items[i].type = D_BUTTON;
	d->items[i].fn = prog_sel_display;
	d->items[i].text = TEXT(T_DISPLAY);
	i++;
	d->items[i].type = D_BUTTON;
	d->items[i].fn = prog_sel_cancel;
	d->items[i].gid = B_ESC;
	d->items[i].text = TEXT(T_CANCEL);
	d->items[i+1].type = D_END;
 	do_dialog(ses->term, d, ml);
}



/* deallocates a */
void type_query(struct session *ses, unsigned char *ct, struct assoc *a, int n)
{
	unsigned char *m1;
	unsigned char *m2;
	int plumbres=0;
	if (ses->tq_prog) mem_free(ses->tq_prog), ses->tq_prog = NULL;

	if (n>1){vysad_okno(ses,ct,a,n);return;}
	
	if (a) ses->tq_prog = stracpy(a[0].prog), ses->tq_prog_flags = a[0].block;
	if (a && !a[0].ask) {
		tp_open(ses);
		if (n)mem_free(a);
		if (ct)mem_free(ct);
		return;
	}
	m1 = stracpy(ct);


	if (!a) {
		if (!anonymous){ /*try plumbing first, then menu*/
			plumbres=plumb((char *)ses->tq->orig_url);
		}
		if(plumbres!=-1)
			goto done;
		if (!anonymous) msg_box(ses->term, getml(m1, NULL), TEXT(T_UNKNOWN_TYPE), AL_CENTER | AL_EXTD_TEXT, TEXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TEXT(T_DO_YOU_WANT_TO_SAVE_OR_DISLPAY_THIS_FILE), NULL, ses, 3, TEXT(T_SAVE), tp_save, B_ENTER, TEXT(T_DISPLAY), tp_display, 0, TEXT(T_CANCEL), tp_cancel, B_ESC);
		else msg_box(ses->term, getml(m1, NULL), TEXT(T_UNKNOWN_TYPE), AL_CENTER | AL_EXTD_TEXT, TEXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TEXT(T_DO_YOU_WANT_TO_SAVE_OR_DISLPAY_THIS_FILE), NULL, ses, 2, TEXT(T_DISPLAY), tp_display, B_ENTER, TEXT(T_CANCEL), tp_cancel, B_ESC);
	} else {
		m2 = stracpy(a[0].label ? a[0].label : (unsigned char *)"");
		if (!anonymous) msg_box(ses->term, getml(m1, m2, NULL), TEXT(T_WHAT_TO_DO), AL_CENTER | AL_EXTD_TEXT, TEXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TEXT(T_DO_YOU_WANT_TO_OPEN_FILE_WITH), " ", m2, ", ", TEXT(T_SAVE_IT_OR_DISPLAY_IT), NULL, ses, 4, TEXT(T_OPEN), tp_open, B_ENTER, TEXT(T_SAVE), tp_save, 0, TEXT(T_DISPLAY), tp_display, 0, TEXT(T_CANCEL), tp_cancel, B_ESC);
		else msg_box(ses->term, getml(m1, m2, NULL), TEXT(T_WHAT_TO_DO), AL_CENTER | AL_EXTD_TEXT, TEXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TEXT(T_DO_YOU_WANT_TO_OPEN_FILE_WITH), " ", m2, ", ", TEXT(T_SAVE_IT_OR_DISPLAY_IT), NULL, ses, 3, TEXT(T_OPEN), tp_open, B_ENTER, TEXT(T_DISPLAY), tp_display, 0, TEXT(T_CANCEL), tp_cancel, B_ESC);
	}
	done: 
		if (n)mem_free(a);
		if (ct)mem_free(ct);
}

void ses_go_to_2nd_state(struct session *ses)
{
	struct assoc *a;
	int n;
	unsigned char *ct = NULL;
	int r = plain_type(ses, ses->rq, &ct);
	if (r == 0 || r == 1 || r == 2 || r == 3) goto go;
	if (!(a = get_type_assoc(ses->term, ct, &n)) && strlen(( char *)ct) >= 4 && !casecmp(ct, (unsigned char *)"text/", 4)) {
		r = 1;
		goto go;
	}
	if (ses->tq) {
		ses_abort_1st_state_loading(ses);
		if (n) mem_free(a);
		if (ct) mem_free(ct);
		return;
	}
	(ses->tq = ses->rq)->upcall = NULL;
	ses->rq = NULL;
	ses_abort_1st_state_loading(ses);
	type_query(ses, ct, a, n);
	return;
	go:
	ses_go_forward(ses, r, ses->wtd_refresh);
	if (ct) mem_free(ct);
}

void ses_go_back_to_2nd_state(struct session *ses)
{
	ses_go_backward(ses);
}

void ses_finished_1st_state(struct object_request *rq, struct session *ses)
{
	if (rq->state != O_WAITING) {
		if (ses->wtd_refresh && ses->wtd_target_base && ses->wtd_target_base->refresh_timer != -1) {
			kill_timer(ses->wtd_target_base->refresh_timer);
			ses->wtd_target_base->refresh_timer = -1;
		}
	}
	switch (rq->state) {
		case O_WAITING:
			change_screen_status(ses);
			print_screen_status(ses);
			break;
		case O_FAILED:
			print_error_dialog(ses, &rq->stat, rq->url);
			ses_abort_1st_state_loading(ses);
			break;
		case O_LOADING:
		case O_INCOMPLETE:
		case O_OK:
			ses->wtd(ses);
			break;
	}
}

void ses_destroy_defered_jump(struct session *ses)
{
	if (ses->defered_url) mem_free(ses->defered_url), ses->defered_url = NULL;
	if (ses->defered_target) mem_free(ses->defered_target), ses->defered_target = NULL;
	ses->defered_target_base = NULL;
}

#ifdef JS
/* test if there're any running scripts */
static inline int __any_running_scripts(struct f_data_c *fd)
{
	if (!fd->js)return 0;
	return (fd->js->active)||(!list_empty(fd->js->queue));
}
#endif

/* if from_goto_dialog is 1, set prev_url to NULL */
void goto_url_f(struct session *ses, void (*state2)(struct session *), unsigned char *url, unsigned char *target, struct f_data_c *df, int data, int defer, int from_goto_dialog, int refresh)
{
	unsigned char *u, *pos;
	unsigned char *prev_url;
	void (*fn)(struct session *, unsigned char *);
	if (!state2) state2 = ses_go_to_2nd_state;
	ses_destroy_defered_jump(ses);
	if ((fn = get_external_protocol_function(url))) {
		fn(ses, url);
		return;
	}
	ses->reloadlevel = NC_CACHE;
	if (!(u = translate_url(url, ses->term->cwd))) {
		struct status stat = { NULL, NULL, NULL, NULL, S_BAD_URL, PRI_CANCEL, 0, NULL, NULL };
		print_error_dialog(ses, &stat, url);
		return;
	}
#ifdef JS
	if (defer && __any_running_scripts(ses->screen) ) {
		ses->defered_url = u;
		ses->defered_target = stracpy(target);
		ses->defered_target_base = df;
		ses->defered_data=data;
		return;
	}
#endif
	pos = extract_position(u);
	if (ses->wtd == state2 && !strcmp((char *)ses->rq->orig_url,(char *) u) && !xstrcmp(ses->wtd_target, target) && ses->wtd_target_base == df) {
		mem_free(u);
		if (ses->goto_position) mem_free(ses->goto_position);
		ses->goto_position = pos;
		return;
	}
	ses_abort_1st_state_loading(ses);
	ses->wtd = state2;
	ses->wtd_target = stracpy(target);
	ses->wtd_target_base = df;
	ses->wtd_refresh = refresh;
	if (ses->goto_position) mem_free(ses->goto_position);
	ses->goto_position = pos;
	if (ses->default_status){mem_free(ses->default_status);ses->default_status=NULL;}	/* smazeme default status, aby neopruzoval na jinych strankach */
	if (!from_goto_dialog&&df&&(df->rq))prev_url=df->rq->url;
	else prev_url=NULL;   /* previous page is empty - this probably never happens, but for sure */
	request_object(ses->term, u, prev_url, PRI_MAIN, NC_CACHE, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);
	mem_free(u);
}

/* this doesn't send referer */
void goto_url(struct session *ses, unsigned char *url)
{
	goto_url_f(ses, NULL, url, NULL, NULL, -1, 0, 1, 0);
}

/* this one sends referer */
void goto_url_not_from_dialog(struct session *ses, unsigned char *url)
{
	goto_url_f(ses, NULL, url, NULL, NULL, -1, 0, 0, 0);
}

void ses_imgmap(struct session *ses)
{
	unsigned char *start, *end;
	struct memory_list *ml;
	struct menu_item *menu;
	if (ses->rq->state != O_OK && ses->rq->state != O_INCOMPLETE) return;
	get_file(ses->rq, &start, &end);
	if (get_image_map(ses->rq->ce && ses->rq->ce->head ? ses->rq->ce->head : (unsigned char *)"", start, end, ses->goto_position, &menu, &ml, ses->imgmap_href_base, ses->imgmap_target_base, ses->term->spec->charset, ses->ds.assume_cp, ses->ds.hard_assume, 0)) {
		ses_abort_1st_state_loading(ses);
		return;
	}
	add_empty_window(ses->term, (void (*)(void *))freeml, ml);
	do_menu(ses->term, menu, ses);
	ses_abort_1st_state_loading(ses);
}

void goto_imgmap(struct session *ses, unsigned char *url, unsigned char *href, unsigned char *target)
{
	if (ses->imgmap_href_base) mem_free(ses->imgmap_href_base);
	ses->imgmap_href_base = href;
	if (ses->imgmap_target_base) mem_free(ses->imgmap_target_base);
	ses->imgmap_target_base = target;
	goto_url_f(ses, ses_imgmap, url, NULL, NULL, -1, 0, 0, 0);
}

void map_selected(struct terminal *term, struct link_def *ld, struct session *ses)
{
	int x = 0;
	if (ld->onclick) {
		struct f_data_c *fd = current_frame(ses);
		jsint_execute_code(fd, ld->onclick, strlen(( char *)ld->onclick), -1, -1, -1);
		x = 1;
	}
	goto_url_f(ses, NULL, ld->link, ld->target, current_frame(ses), -1, x, 0, 0);
}

void go_back(struct session *ses)
{
	struct location *loc;
	ses->reloadlevel = NC_CACHE;
	ses_destroy_defered_jump(ses);
	if (ses_abort_1st_state_loading(ses)) {
		change_screen_status(ses);
		print_screen_status(ses);
		return;
	}
	if (ses->history.next == &ses->history || ses->history.next == ses->history.prev)
		return;
	loc = ((struct location *)ses->history.next)->next;
	ses->wtd = ses_go_back_to_2nd_state;
	if (ses->default_status){mem_free(ses->default_status);ses->default_status=NULL;}	/* smazeme default status, aby neopruzoval na jinych strankach */
	request_object(ses->term, loc->url, loc->prev_url, PRI_MAIN, NC_ALWAYS_CACHE, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);
}

void reload_frame(struct f_data_c *fd, int no_cache)
{
	unsigned char *u;
	if (!list_empty(fd->subframes)) {
		struct f_data_c *fdd;
		foreach(fdd, fd->subframes) {
			reload_frame(fdd, no_cache);
		}
		return;
	}
	if (!fd->f_data || !f_is_finished(fd->f_data)) return;
	u = stracpy(fd->rq->url);
	release_object(&fd->rq);
	release_object(&fd->f_data->rq);
	request_object(fd->ses->term, u, NULL, PRI_MAIN, no_cache, (void (*)(struct object_request *, void *))fd_loaded, fd, &fd->rq);
	clone_object(fd->rq, &fd->f_data->rq);
	fd->done = 0;
	fd->parsed_done = 0;
	mem_free(u);
	jsint_destroy(fd);
}

void reload(struct session *ses, int no_cache)
{
	ses_destroy_defered_jump(ses);
	if (no_cache == -1) no_cache = ++ses->reloadlevel;
	else ses->reloadlevel = no_cache;
	reload_frame(ses->screen, no_cache);
	/*request_object(ses->term, cur_loc(ses)->url, cur_loc(ses)->prev_url, PRI_MAIN, no_cache, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);*/
}

void set_doc_view(struct session *ses)
{
	ses->screen->xp = 0;
	ses->screen->yp = gf_val(1, G_BFU_FONT_SIZE);
	ses->screen->xw = ses->term->x;
	ses->screen->yw = ses->term->y - gf_val(2, 2 * G_BFU_FONT_SIZE);
}

struct session *create_session(struct window *win)
{
	static int session_id = 1;
	struct terminal *term = win->term;
	struct session *ses;
	if ((ses = mem_calloc(sizeof(struct session)))) {
		init_list(ses->history);
		ses->term = term;
		ses->win = win;
		ses->id = session_id++;
		ses->screen = create_f_data_c(ses, NULL);
		ses->screen->xp = 0; ses->screen->xw = term->x;
		ses->screen->yp = gf_val(1, G_BFU_FONT_SIZE);
		ses->screen->yw = term->y - gf_val(2, 2 * G_BFU_FONT_SIZE);
		memcpy(&ses->ds, &dds, sizeof(struct document_setup));
		init_list(ses->format_cache);
		add_to_list(sessions, ses);
	}
	if (first_use) {
		first_use = 0;
		msg_box(term, NULL, TEXT(T_WELCOME), AL_CENTER | AL_EXTD_TEXT, TEXT(T_WELCOME_TO_LINKS), "\n\n", TEXT(T_BASIC_HELP), NULL, NULL, 1, TEXT(T_OK), NULL, B_ENTER | B_ESC);
	}
	return ses;
}

/* vyrobi retezec znaku, ktery se posilaj skrz unix domain socket hlavni instanci 
   prohlizece
   cp=cislo session odkud se ma kopirovat (kdyz se klikne na "open new window")
   url=url kam se ma jit (v pripade kliknuti na "open link in new window" nebo pusteni linksu z prikazove radky s url)
   framename=jmeno ramu, ktery se vytvori

   vraci sekvenci bytu a delku
 */
void *create_session_info(int cp, unsigned char *url, unsigned char *framename, int *ll)
{
	int l = strlen(( char *)url);
	int l1 = framename?strlen(( char *)framename):0;
	int *i;
	if (framename&&!strcmp((char *)framename,"_blank")) l1=0;
	*ll = 3 * sizeof(int) + l + l1;
	if (!(i = mem_alloc(3 * sizeof(int) + l + l1))) return NULL;
	i[0] = cp;
	i[1] = l;
	i[2] = l1;
	memcpy(i + 3, url, l);
	if (l1) memcpy((unsigned char*)(i + 3) + l, framename, l1);
	return i;
}

/* dostane data z create_session_info a nainicializuje podle nich session
   vraci -1 pokud jsou data vadna
 */
int read_session_info(struct session *ses, void *data, int len)
{
	unsigned char *h;
	int cpfrom, sz, sz1;
	struct session *s;
	if (len < 3 * sizeof(int)) return -1;
	cpfrom = *(int *)data;
	sz = *((int *)data + 1);
	sz1= *((int *)data + 2);
	foreach(s, sessions) if (s->id == cpfrom) {
		if (!list_empty(s->history)) {
			struct location *loc = s->history.next;
			if (loc->url) goto_url(ses, loc->url);
		}
		return 0;
	}
	if (sz1) {
		unsigned char *tgt;
		if (len<3*sizeof(int)+sz+sz1) goto bla;
		if ((tgt=mem_alloc(sz1+1))) {
			memcpy(tgt, (unsigned char*)((int*)data+3)+sz,sz1);
			tgt[sz1]=0;
			if (ses->wanted_framename) mem_free(ses->wanted_framename), ses->wanted_framename=NULL;
			ses->wanted_framename=tgt;
		}
	}
	bla:
	if (sz) {
		char *u, *uu;
		if (len < 3 * sizeof(int) + sz) return 0;
		if ((u = mem_alloc(sz + 1))) {
			memcpy(u, (int *)data + 3, sz);
			u[sz] = 0;
			uu = (char *)decode_url((unsigned char *)u);
			goto_url(ses,(unsigned char *) uu);
			mem_free(u);
			mem_free(uu);
		}
	} else if ((h = (unsigned char *)getenv("WWW_HOME")) && *h) goto_url(ses, h);
	return 0;
}

void destroy_session(struct session *ses)
{
	struct download *d;
	foreach(d, downloads) if (d->ses == ses && d->prog) {
		d = d->prev;
		abort_download(d->next);
	}
	ses_abort_1st_state_loading(ses);
	reinit_f_data_c(ses->screen);
	mem_free(ses->screen);
	while (!list_empty(ses->format_cache)) {
		struct f_data *f = ses->format_cache.next;
		del_from_list(f);
		destroy_formatted(f);
	}
	while (!list_empty(ses->history)) destroy_location(ses->history.next);
	if (ses->st) mem_free(ses->st);
	if (ses->default_status)mem_free(ses->default_status);
	if (ses->dn_url) mem_free(ses->dn_url);
	if (ses->search_word) mem_free(ses->search_word);
	if (ses->last_search_word) mem_free(ses->last_search_word);
	if (ses->imgmap_href_base) mem_free(ses->imgmap_href_base);
	if (ses->imgmap_target_base) mem_free(ses->imgmap_target_base);
	if (ses->wanted_framename) mem_free(ses->wanted_framename);

	release_object(&ses->tq);
	if (ses->tq_prog) mem_free(ses->tq_prog);

	ses_destroy_defered_jump(ses);

	del_from_list(ses);
}

void win_func(struct window *win, struct event *ev, int fw)
{
	struct session *ses = win->data;
	switch (ev->ev) {
		case EV_ABORT:
			destroy_session(ses);
			break;
		case EV_INIT:
			if (!(ses = win->data = create_session(win)) || read_session_info(ses, (char *)ev->b + sizeof(int), *(int *)ev->b)) {
				register_bottom_half((void (*)(void *))destroy_terminal, win->term);
				return;
			}
		case EV_RESIZE:
			GF(set_window_pos(win, 0, 0, ev->x, ev->y));
			set_doc_view(ses);
			html_interpret_recursive(ses->screen);
			draw_fd(ses->screen);
			break;
		case EV_REDRAW:
			draw_formatted(ses);
			break;
		case EV_MOUSE:
#ifdef G
			if (F) set_window_ptr(win, ev->x, ev->y);
#endif
			/* fall through ... */
		case EV_KBD:
			send_event(ses, ev);
			break;
		default:
			error((unsigned char *)"ERROR: unknown event");
	}
}

/* 
  Gets the url being viewed by this session. Writes it into str.
  A maximum of str_size bytes (including null) will be written.
*/  
unsigned char *get_current_url(struct session *ses, unsigned char *str, size_t str_size) {
	unsigned char *here, *end_of_url;
	size_t url_len = 0;

	/* Not looking at anything */
	if (list_empty(ses->history))
		return NULL;

	here = cur_loc(ses)->url;

	/* Find the length of the url */
    if ((end_of_url = (unsigned char *)strchr((char *)here, POST_CHAR))) {
		url_len = (size_t)(end_of_url - (unsigned char *)here);
	} else {
		url_len = strlen(( char *)here);
	}

	/* Ensure that the url size is not greater than str_size */ 
	if (url_len >= str_size)
			url_len = str_size - 1;

	strncpy((char *)str, (char *)here, url_len);

	/* Ensure null termination */
	str[url_len] = '\0';
	
	return str;
}


/* 
  Gets the title of the page being viewed by this session. Writes it into str.
  A maximum of str_size bytes (including null) will be written.
*/  
unsigned char *get_current_title(struct session *ses, unsigned char *str, size_t str_size) {
	struct f_data_c *fd;
	fd = (struct f_data_c *)current_frame(ses);

	/* Ensure that the title is defined */
	if (!fd || !fd->f_data)
		return NULL;

	return safe_strncpy(str, fd->f_data->title, str_size);
}

/* 
  Gets the url of the link currently selected. Writes it into str.
  A maximum of str_size bytes (including null) will be written.
*/  
unsigned char *get_current_link_url(struct session *ses, unsigned char *str, size_t str_size) {
	struct f_data_c *fd;
    struct link *l;
	
	fd = (struct f_data_c *)current_frame(ses);
	/* What the hell is an 'fd'? */
	if (!fd)
		return NULL;
	
	/* Nothing selected? */
    if (fd->vs->current_link == -1 || fd->vs->current_link >= fd->f_data->nlinks) 
		return NULL;

    l = &fd->f_data->links[fd->vs->current_link];
	/* Only write a link */
    if (l->type != L_LINK) 
		return NULL;
	
	return safe_strncpy(str, l->where, str_size);
}
