/* view_gr.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef G

#include "links.h"

int *highlight_positions = NULL;
int n_highlight_positions = 0;
int highlight_position_size = 0;

static int root_x = 0;
static int root_y = 0;

void g_get_search_data(struct f_data *f);

static int previous_link=-1;	/* for mouse event handlers */

void g_draw_background(struct graphics_device *dev, struct background *bg, int x, int y, int xw, int yw)
{
	if (xw > 4096) {
		int halfx = x + xw / 2;
		if (dev->clip.x1 < halfx) g_draw_background(dev, bg, x, y, halfx - x, yw);
		if (dev->clip.x2 > halfx) g_draw_background(dev, bg, halfx, y, x + xw - halfx, yw);
		return;
	}
	if (yw > 4096) {
		int halfy = y + yw / 2;
		if (dev->clip.y1 < halfy) g_draw_background(dev, bg, x, y, xw, halfy - y);
		if (dev->clip.y2 > halfy) g_draw_background(dev, bg, x, halfy, xw, y + yw - halfy);
		return;
	}
	if (bg->img) {
		img_draw_decoded_image(dev, bg->u.img, x, y, xw, yw, x - root_x, y - root_y);
	} else {
		drv->fill_area(dev, x, y, x + xw, y + yw
			       , dip_get_color_sRGB(bg->u.sRGB));
	}
}

void g_release_background(struct background *bg)
{
	if (bg->img) img_release_decoded_image(bg->u.img);
	mem_free(bg);
}

void g_dummy_draw(struct f_data_c *fd, struct g_object *t, int x, int y)
{
}

void g_tag_destruct(struct g_object *t)
{
	mem_free(t);
}

void g_dummy_mouse(struct f_data_c *fd, struct g_object *a, int x, int y, int b)
{
}

int print_all_textarea = 0;

void g_text_draw(struct f_data_c *fd, struct g_object_text *t, int x, int y)
{
	struct form_control *form;
	struct form_state *fs;
	struct link *link = t->link_num >= 0 ? fd->f_data->links + t->link_num : NULL;
	int l;
	int ll;
	int i, j;
	int yy;
	int cur;
	struct line_info *ln, *lnx;
	if (link && ((form = link->form)) && ((fs = find_form_state(fd, form)))) {
		switch (form->type) {
			struct style *inv;
			int in;
			case FC_RADIO:
				if (link && fd->active && fd->vs->g_display_link && fd->vs->current_link == link - fd->f_data->links) inv = g_invert_style(t->style), in = 1;
				else inv = t->style, in = 0;
				g_print_text(drv, fd->ses->term->dev, x, y, inv, fs->state ? (unsigned char *)"[X]" : (unsigned char *)"[ ]", NULL);
				if (in) g_free_style(inv);
				return;
			case FC_CHECKBOX:
				if (link && fd->active && fd->vs->g_display_link && fd->vs->current_link == link - fd->f_data->links) inv = g_invert_style(t->style), in = 1;
				else inv = t->style, in = 0;
				g_print_text(drv, fd->ses->term->dev, x, y, inv, fs->state ? (unsigned char *)"[X]" : (unsigned char *)"[ ]", NULL);
				if (in) g_free_style(inv);
				return;
			case FC_SELECT:
				if (link && fd->active && fd->vs->g_display_link && fd->vs->current_link == link - fd->f_data->links) inv = g_invert_style(t->style), in = 1;
				else inv = t->style, in = 0;
				fixup_select_state(form, fs);
				l = 0;
				if (fs->state < form->nvalues) g_print_text(drv, fd->ses->term->dev, x, y, inv, form->labels[fs->state], &l);
				while (l < t->xw) g_print_text(drv, fd->ses->term->dev, x + l, y, inv, (unsigned char *)"_", &l);
				if (in) g_free_style(inv);
				return;
			case FC_TEXT:
			case FC_PASSWORD:
			case FC_FILE:
				/*
				if (fs->state >= fs->vpos + form->size) fs->vpos = fs->state - form->size + 1;
				if (fs->state < fs->vpos) fs->vpos = fs->state;
				*/
				while (fs->vpos < strlen((char *)fs->value) && utf8_diff(fs->value + fs->state, fs->value + fs->vpos) >= form->size) {
					unsigned char *p = fs->value + fs->vpos;
					FWD_UTF_8(p);
					fs->vpos = p - fs->value;
				}
				while (fs->vpos > fs->state) {
					unsigned char *p = fs->value + fs->vpos;
					BACK_UTF_8(p, fs->value);
					fs->vpos = p - fs->value;
				}
				l = 0;
				i = 0;
				ll = strlen((char *)fs->value);
				while (l < t->xw) {
					struct style *st = t->style;
					int sm = 0;
					unsigned char tx[11];
					if (fs->state == fs->vpos + i && t->link_num == fd->vs->current_link && fd->ses->locked_link) {
						st = g_invert_style(t->style);
						sm = 1;
					}
					tx[1] = 0;
					if (fs->vpos + i >= ll) tx[0] = '_', tx[1] = 0, i++;
					else {
						unsigned char *p = fs->value + fs->vpos + i;
						unsigned char *pp = p;
						FWD_UTF_8(p);
						if (p - pp > 10) {
							i++;
							goto xy;
						}
						memcpy(tx, pp, p - pp);
						tx[p - pp] = 0;
						i += strlen((char *)tx);
						if (form->type == FC_PASSWORD) xy:tx[0] = '*';
					}
					g_print_text(drv, fd->ses->term->dev, x + l, y, st, tx, &l);
					if (sm) g_free_style(st);
				}
				return;
			case FC_TEXTAREA:
				cur = _area_cursor(form, fs);
				if (!(lnx = format_text(fs->value, form->cols, form->wrap))) break;
				ln = lnx;
				yy = y - t->link_order * t->style->height;
				for (j = 0; j < fs->vypos; j++) if (ln->st) ln++;
				for (j = 0; j < form->rows; j++) {
					unsigned char *pp = ln->st;
					int xx = fs->vpos;
					while (pp < ln->en && xx > 0) {
						FWD_UTF_8(pp);
						xx--;
					}
					if (cur >= 0 && cur < form->cols && t->link_num == fd->vs->current_link && fd->ses->locked_link && fd->active) {
						unsigned char tx[11];
						int xx = x;

						if (print_all_textarea || j == t->link_order) while (xx < x + t->xw) {
							struct style *st = t->style;
							unsigned char *ppp = pp;
							if (ln->st && pp < ln->en) {
								FWD_UTF_8(pp);
								memcpy(tx, ppp, pp - ppp);
								tx[pp - ppp] = 0;
							} else {
								tx[0] = '_';
								tx[1] = 0;
							}
							if (!cur) {
								st = g_invert_style(t->style);
							}
							g_print_text(drv, fd->ses->term->dev, xx, yy + j * t->style->height, st, tx, &xx);
							if (!cur) {
								g_free_style(st);
							}
							cur--;
						} else cur -= form->cols;
					} else {
						if (print_all_textarea || j == t->link_order) {
							int aa;
							unsigned char *a;
							struct rect old;
							if (ln->st && pp < ln->en) a = memacpy(pp, ln->en - pp);
							else a = stracpy((unsigned char *)"");
							for (aa = 0; aa < form->cols; aa += 4) add_to_strn(&a, (unsigned char *)"____");
							restrict_clip_area(fd->ses->term->dev, &old, x, 0, x + t->xw, fd->ses->term->dev->size.y2);
							g_print_text(drv, fd->ses->term->dev, x, yy + j * t->style->height, t->style, a, NULL);
							drv->set_clip_area(fd->ses->term->dev, &old);
							mem_free(a);
						}
						cur -= form->cols;
					}
					if (ln->st) ln++;
				}
				mem_free(lnx);
				return;
		}
	}
	if (link && fd->active && fd->vs->g_display_link && fd->vs->current_link == link - fd->f_data->links) {
		struct style *inv;
		inv = g_invert_style(t->style);
		g_print_text(drv, fd->ses->term->dev, x, y, inv, t->text, NULL);
		g_free_style(inv);
	} else if (!highlight_positions || !n_highlight_positions) {
		prn:
		g_print_text(drv, fd->ses->term->dev, x, y, t->style, t->text, NULL);
	} else {
		int tlen = strlen((char *)t->text);
		int found;
		int start = t->srch_pos;
		int end = t->srch_pos + tlen;
		unsigned char *mask;
		unsigned char *tx;
		int txl;
		int pmask;
		int ii;
		struct style *inv;
#define B_EQUAL(t, m) ((t) + highlight_position_size > start && (t) < end)
#define B_ABOVE(t, m) ((t) >= end)
		BIN_SEARCH(highlight_positions, n_highlight_positions, B_EQUAL, B_ABOVE, *, found);
		if (found == -1) goto prn;
		while (found > 0 && B_EQUAL(highlight_positions[found - 1], *)) found--;
		if (!(mask = mem_calloc(tlen))) goto prn;
		while (found < n_highlight_positions && !B_ABOVE(highlight_positions[found], *)) {
			int pos = highlight_positions[found] - t->srch_pos;
			int ii = 0;
			for (ii = 0; ii < highlight_position_size; ii++) {
				if (pos >= 0 && pos < tlen) mask[pos] = 1;
				pos++;
			}
			found++;
		}
		inv = g_invert_style(t->style);
		tx = init_str();;
		txl = 0;
		pmask = -1;
		for (ii = 0; ii < tlen; ii++) {
			if (mask[ii] != pmask) {
				g_print_text(drv, fd->ses->term->dev, x, y, pmask ? inv : t->style, tx, &x);
				mem_free(tx);
				tx = init_str();
				txl = 0;
			}
			add_chr_to_str(&tx, &txl, t->text[ii]);
			pmask = mask[ii];
		}
		g_print_text(drv, fd->ses->term->dev, x, y, pmask ? inv : t->style, tx, &x);
		mem_free(tx);
		g_free_style(inv);
		mem_free(mask);
	}
}

void g_text_destruct(struct g_object_text *t)
{
	release_image_map(t->map);
	g_free_style(t->style);
	mem_free(t);
}

void g_line_draw(struct f_data_c *fd, struct g_object_line *l, int xx, int yy)
{
	struct graphics_device *dev = fd->ses->term->dev;
	int i;
	int x = 0;
	for (i = 0; i < l->n_entries; i++) {
		struct g_object *o = (struct g_object *)l->entries[i];
		if (o->x > x) g_draw_background(dev, l->bg, xx + x, yy, o->x - x, l->yw);
		if (o->y > 0) g_draw_background(dev, l->bg, xx + o->x, yy, o->xw, o->y);
		if (o->y + o->yw < l->yw) g_draw_background(dev, l->bg, xx + o->x, yy + o->y + o->yw, o->xw, l->yw - o->y - o->yw);
		o->draw(fd, o, xx + o->x, yy + o->y);
		x = o->x + o->xw;
	}
	if (x < l->xw) g_draw_background(dev, l->bg, xx + x, yy, l->xw - x, l->yw);
}

void g_line_destruct(struct g_object_line *l)
{
	int i;
	for (i = 0; i < l->n_entries; i++) l->entries[i]->destruct(l->entries[i]);
	mem_free(l);
}

void g_line_get_list(struct g_object_line *l, void (*f)(struct g_object *parent, struct g_object *child))
{
	int i;
	for (i = 0; i < l->n_entries; i++) f((struct g_object *)l, l->entries[i]);
}

#define OBJ_EQ(a, b)	(*(a)).y <= (b) && (*(a)).y + (*(a)).yw > (b)
#define OBJ_ABOVE(a, b)	(*(a)).y > (b)

static inline struct g_object **g_find_line(struct g_object **a, int n, int p)
{
	int res = -1;
	BIN_SEARCH(a, n, OBJ_EQ, OBJ_ABOVE, p, res);
	if (res == -1) return NULL;
	return &a[res];
}

void g_area_draw(struct f_data_c *fd, struct g_object_area *a, int xx, int yy)
{
	struct g_object **i;
	int rx = root_x, ry = root_y;
	int y1 = fd->ses->term->dev->clip.y1 - yy;
	int y2 = fd->ses->term->dev->clip.y2 - yy - 1;
	struct g_object **l1;
	struct g_object **l2;
	if (fd->ses->term->dev->clip.y1 == fd->ses->term->dev->clip.y2 || fd->ses->term->dev->clip.x1 == fd->ses->term->dev->clip.x2) return;
	l1 = g_find_line((struct g_object **)&a->lines, a->n_lines, y1);
	l2 = g_find_line((struct g_object **)&a->lines, a->n_lines, y2);
	root_x = xx, root_y = yy;
	if (!l1) {
		if (y1 > a->yw) return;
		else l1 = (struct g_object **)&a->lines[0];
	}
	if (!l2) {
		if (y2 < 0) return;
		else l2 = (struct g_object **)&a->lines[a->n_lines - 1];
	}
	for (i = l1; i <= l2; i++) {
		struct g_object *o = *i;
		o->draw(fd, o, xx + o->x, yy + o->y);
	}
	/* !!! FIXME: floating objects */
	root_x = rx, root_y = ry;
}

void g_area_destruct(struct g_object_area *a)
{
	int i;
	g_release_background(a->bg);
	for (i = 0; i < a->n_lfo; i++) a->lfo[i]->destruct(a->lfo[i]);
	mem_free(a->lfo);
	for (i = 0; i < a->n_rfo; i++) a->rfo[i]->destruct(a->rfo[i]);
	mem_free(a->rfo);
	for (i = 0; i < a->n_lines; i++) a->lines[i]->destruct(a->lines[i]);
	mem_free(a);
}

void g_area_get_list(struct g_object_area *a, void (*f)(struct g_object *parent, struct g_object *child))
{
	int i;
	for (i = 0; i < a->n_lfo; i++) f((struct g_object *)a, a->lfo[i]);
	for (i = 0; i < a->n_rfo; i++) f((struct g_object *)a, a->rfo[i]);
	for (i = 0; i < a->n_lines; i++) f((struct g_object *)a, (struct g_object *)a->lines[i]);
}

/*
 * dsize - size of scrollbar
 * total - total data
 * vsize - visible data
 * vpos - position of visible data
 */

void get_scrollbar_pos(int dsize, int total, int vsize, int vpos, int *start, int *end)
{
	int ssize;
	if (!total) {
		*start = *end = 0;
		return;
	}
	ssize = dsize * vsize / total;
	if (ssize < G_SCROLL_BAR_MIN_SIZE) ssize = G_SCROLL_BAR_MIN_SIZE;
	if (total == vsize) {
		*start = 0; *end = dsize;
		return;
	}
	*start = (dsize - ssize) * vpos / (total - vsize);
	*end = *start + ssize;
	if (*start > dsize) *start = dsize;
	if (*start < 0) *start = 0;
	if (*end > dsize) *end = dsize;
	if (*end < 0) *end = 0;
	/*
	else {
		*start = vpos * dsize / total;
		*end = (vpos + vsize) * dsize / total;
	}
	if (*end > dsize) *end = dsize;
	*/
}

long scroll_bar_frame_color;
long scroll_bar_area_color;
long scroll_bar_bar_color;

void draw_vscroll_bar(struct graphics_device *dev, int x, int y, int yw, int total, int view, int pos)
{
	int spos, epos;
	drv->draw_hline(dev, x, y, x + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_vline(dev, x, y, y + yw, scroll_bar_frame_color);
	drv->draw_vline(dev, x + G_SCROLL_BAR_WIDTH - 1, y, y + yw, scroll_bar_frame_color);
	drv->draw_hline(dev, x, y + yw - 1, x + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_vline(dev, x + 1, y + 1, y + yw - 1, scroll_bar_area_color);
	drv->draw_vline(dev, x + G_SCROLL_BAR_WIDTH - 2, y + 1, y + yw - 1, scroll_bar_area_color);
	get_scrollbar_pos(yw - 4, total, view, pos, &spos, &epos);
	drv->fill_area(dev, x + 2, y + 1, x + G_SCROLL_BAR_WIDTH - 2, y + 2 + spos, scroll_bar_area_color);
	drv->fill_area(dev, x + 2, y + 2 + spos, x + G_SCROLL_BAR_WIDTH - 2, y + 2 + epos, scroll_bar_bar_color);
	drv->fill_area(dev, x + 2, y + 2 + epos, x + G_SCROLL_BAR_WIDTH - 2, y + yw - 1, scroll_bar_area_color);
}

void draw_hscroll_bar(struct graphics_device *dev, int x, int y, int xw, int total, int view, int pos)
{
	int spos, epos;
	drv->draw_vline(dev, x, y, y + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_hline(dev, x, y, x + xw, scroll_bar_frame_color);
	drv->draw_hline(dev, x, y + G_SCROLL_BAR_WIDTH - 1, x + xw, scroll_bar_frame_color);
	drv->draw_vline(dev, x + xw - 1, y, y + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_hline(dev, x + 1, y + 1, x + xw - 1, scroll_bar_area_color);
	drv->draw_hline(dev, x + 1, y + G_SCROLL_BAR_WIDTH - 2, x + xw - 1, scroll_bar_area_color);
	get_scrollbar_pos(xw - 4, total, view, pos, &spos, &epos);
	drv->fill_area(dev, x + 1, y + 2, x + 2 + spos, y + G_SCROLL_BAR_WIDTH - 2, scroll_bar_area_color);
	drv->fill_area(dev, x + 2 + spos, y + 2, x + 2 + epos, y + G_SCROLL_BAR_WIDTH - 2, scroll_bar_bar_color);
	drv->fill_area(dev, x + 2 + epos, y + 2, x + xw - 1, y + G_SCROLL_BAR_WIDTH - 2, scroll_bar_area_color);
}

void g_get_search(struct f_data *f, unsigned char *s)
{
	int i;
	if (!s || !*s) return;
	if (f->last_search && !strcmp((char *)f->last_search, (char *)s)) return;
	if (f->search_positions) mem_free(f->search_positions), f->search_positions = DUMMY, f->n_search_positions = 0;
	if (f->last_search) mem_free(f->last_search);
	if (!(f->last_search = stracpy(s))) return;
	f->last_search_len = strlen((char *)s);
	for (i = 0; i <= f->srch_string_size - f->last_search_len; i++) {
		int ii;
		if (srch_cmp(f->srch_string[i], s[0])) aiee__killing_interrupt_handler: continue;
		for (ii = 1; ii < f->last_search_len; ii++)
			if (srch_cmp(f->srch_string[i + ii], s[ii])) goto aiee__killing_interrupt_handler;
		if (!(f->n_search_positions & (ALLOC_GR - 1))) {
			if (!(f->search_positions = mem_realloc(f->search_positions, (f->n_search_positions + ALLOC_GR) * sizeof(int)))) return;
		}
		f->search_positions[f->n_search_positions++] = i;
	}
}

void draw_graphical_doc(struct terminal *t, struct f_data_c *scr, int active)
{
	int i;
	int r = 0;
	struct rect old;
	struct view_state *vs = scr->vs;
	struct rect_set *rs1, *rs2;
	int xw = scr->xw;
	int yw = scr->yw;
	int vx, vy;

	if (active) {
		if (scr->ses->search_word && scr->ses->search_word[0]) {
			g_get_search_data(scr->f_data);
			g_get_search(scr->f_data, scr->ses->search_word);
			highlight_positions = scr->f_data->search_positions;
			n_highlight_positions = scr->f_data->n_search_positions;
			highlight_position_size = scr->f_data->last_search_len;
		}
	}

	if (vs->view_pos > scr->f_data->y - scr->yw + scr->f_data->hsb * G_SCROLL_BAR_WIDTH) vs->view_pos = scr->f_data->y - scr->yw + scr->f_data->hsb * G_SCROLL_BAR_WIDTH;
	if (vs->view_pos < 0) vs->view_pos = 0;
	if (vs->view_posx > scr->f_data->x - scr->xw + scr->f_data->vsb * G_SCROLL_BAR_WIDTH) vs->view_posx = scr->f_data->x - scr->xw + scr->f_data->vsb * G_SCROLL_BAR_WIDTH;
	if (vs->view_posx < 0) vs->view_posx = 0;
	vx = vs->view_posx;
	vy = vs->view_pos;
	restrict_clip_area(t->dev, &old, scr->xp, scr->yp, scr->xp + xw, scr->yp + yw);
	if (scr->f_data->vsb) draw_vscroll_bar(t->dev, scr->xp + xw - G_SCROLL_BAR_WIDTH, scr->yp, yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH, scr->f_data->y, yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH, vs->view_pos);
	if (scr->f_data->hsb) draw_hscroll_bar(t->dev, scr->xp, scr->yp + yw - G_SCROLL_BAR_WIDTH, xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->f_data->x, xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, vs->view_posx);
	if (scr->f_data->vsb && scr->f_data->hsb) drv->fill_area(t->dev, scr->xp + xw - G_SCROLL_BAR_WIDTH, scr->yp + yw - G_SCROLL_BAR_WIDTH, scr->xp + xw, scr->yp + yw, scroll_bar_frame_color);
	restrict_clip_area(t->dev, NULL, scr->xp, scr->yp, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
	/*debug("buu: %d %d %d, %d %d %d", scr->xl, vx, xw, scr->yl, vy, yw);*/
	if (drv->flags & GD_DONT_USE_SCROLL) goto rrr;
	if (scr->xl == -1 || scr->yl == -1) goto rrr;
	if (is_rect_valid(&scr->ses->win->redr)) goto rrr;
	if (scr->xl - vx > xw || vx - scr->xl > xw ||
	    scr->yl - vy > yw || vy - scr->yl > yw) {
		goto rrr;
	}
	rs1 = rs2 = NULL;
	if (scr->xl != vx)
		r |= drv->hscroll(t->dev, &rs1, scr->xl - vx);
	
	if (scr->yl != vy)
		r |= drv->vscroll(t->dev, &rs2, scr->yl - vy);
	
	if (rs1 || rs2) for (i = 0; i < 2; i++) {
		struct rect_set *rs = !i ? rs1 : rs2;
		if (rs) {
			int j;
			for (j = 0; j < rs->m; j++) {
				struct rect *r = &rs->r[j];
				struct rect clip1;
				/*fprintf(stderr, "scroll: %d,%d %d,%d\n", r->x1, r->y1, r->x2, r->y2);*/
				restrict_clip_area(t->dev, &clip1, r->x1, r->y1, r->x2, r->y2);
				scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
				drv->set_clip_area(t->dev, &clip1);
			}
			mem_free(rs);
		}
	}

	
	if (r) {
		struct rect clip1;
		if (scr->xl < vx)  {
			if (scr->yl < vy) {
				restrict_clip_area(t->dev, &clip1, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH - (vx - scr->xl), scr->yp, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH - (vy - scr->yl));
			} else {
				restrict_clip_area(t->dev, &clip1, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH - (vx - scr->xl), scr->yp + (scr->yl - vy), scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
			}
		} else {
			if (scr->yl < vy) {
				restrict_clip_area(t->dev, &clip1, scr->xp, scr->yp, scr->xp + (scr->xl - vx), scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH - (vy - scr->yl));
			} else {
				restrict_clip_area(t->dev, &clip1, scr->xp, scr->yp + (scr->yl - vy), scr->xp + (scr->xl - vx), scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
			}
		}
		scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
		drv->set_clip_area(t->dev, &clip1);
		if (scr->yl < vy) {
			restrict_clip_area(t->dev, NULL, scr->xp, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH - (vy - scr->yl), scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
		} else {
			restrict_clip_area(t->dev, NULL, scr->xp, scr->yp, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + (scr->yl - vy));
		}
		scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
	}

	goto eee;
	rrr:
	scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
	eee:
	scr->xl = vx;
	scr->yl = vy;
	drv->set_clip_area(t->dev, &old);

	highlight_positions = NULL;
	n_highlight_positions = 0;
	highlight_position_size = 0;
}

int g_forward_mouse(struct f_data_c *fd, struct g_object *a, int x, int y, int b)
{
	if (x >= a->x && x < a->x + a->xw && y >= a->y && y < a->y + a->yw) {
		a->mouse_event(fd, a, x - a->x, y - a->y, b);
		return 1;
	}
	return 0;
}

struct draw_data {
	struct f_data_c *fd;
	struct g_object *o;
};

void draw_one_object_fn(struct terminal *t, struct draw_data *d)
{
	struct rect clip;
	struct f_data_c *scr = d->fd;
	struct g_object *o = d->o;
	int x, y;
	restrict_clip_area(t->dev, &clip, scr->xp, scr->yp, scr->xp + scr->xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + scr->yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
	get_object_pos(o, &x, &y);
	o->draw(scr, o, scr->xp - scr->vs->view_posx + x, scr->yp - scr->vs->view_pos + y);
	drv->set_clip_area(t->dev, &clip);
}

void draw_one_object(struct f_data_c *fd, struct g_object *o)
{
	struct draw_data d;
	d.fd = fd;
	d.o = o;
	draw_to_window(fd->ses->win, (void (*)(struct terminal *, void *))draw_one_object_fn, &d);
}

void g_area_mouse(struct f_data_c *fd, struct g_object_area *a, int x, int y, int b)
{
	int i;
	for (i = 0; i < a->n_lines; i++) if (g_forward_mouse(fd, (struct g_object *)a->lines[i], x, y, b)) return;
}

void g_line_mouse(struct f_data_c *fd, struct g_object_line *a, int x, int y, int b)
{
	int i;
	for (i = 0; i < a->n_entries; i++) if (g_forward_mouse(fd, (struct g_object *)a->entries[i], x, y, b)) return;
}

struct f_data *ffff;

void get_parents_sub(struct g_object *p, struct g_object *c)
{
	c->parent = p;
	if (c->get_list) c->get_list(c, get_parents_sub);
	if (c->destruct == g_tag_destruct) {
		int x = 0, y = 0;
		struct g_object *o;
		c->y -= c->parent->yw;
		for (o = c; o; o = o->parent) x += o->x, y += o->y;
		html_tag(ffff, ((struct g_object_tag *)c)->name, x, y);
	}
	if (c->mouse_event == (void (*)(struct f_data_c *, struct g_object *, int, int, int))g_text_mouse) {
		int l = ((struct g_object_text *)c)->link_num;
		if (l >= 0) {
			struct link *link = &ffff->links[l];
			int x = 0, y = 0;
			struct g_object *o;
			for (o = c; o; o = o->parent) x += o->x, y += o->y;
			if (x < link->r.x1) link->r.x1 = x;
			if (y < link->r.y1) link->r.y1 = y;
			if (x + c->xw > link->r.x2) link->r.x2 = x + c->xw;
			if (y + c->yw > link->r.y2) link->r.y2 = y + c->yw;
			link->obj = c;
		}
	}
}

void get_parents(struct f_data *f, struct g_object *a)
{
	ffff = f;
	a->parent = NULL;
	if (a->get_list) a->get_list(a, get_parents_sub);
}

void get_object_pos(struct g_object *o, int *x, int *y)
{
	*x = *y = 0;
	while (o) {
		*x += o->x;
		*y += o->y;
		o = o->parent;
	}
}

void redraw_link(struct f_data_c *fd, int nl);

/* if set_position is 1 sets cursor position in FIELD/AREA elements */
void g_set_current_link(struct f_data_c *fd, struct g_object_text *a, int x, int y, int set_position)
{
	if (a->map) {
		int i;
		for (i = 0; i < a->map->n_areas; i++) {
			if (is_in_area(&a->map->area[i], x, y) && a->map->area[i].link_num >= 0) {
				fd->vs->current_link = a->map->area[i].link_num;
				return;
			}
		}
	}
	fd->vs->current_link = -1;
	if (a->link_num >= 0) {
		fd->vs->current_link = a->link_num;
		/* if links is a field, set cursor position */
		if (set_position&&a->link_num>=0&&a->link_num<fd->f_data->nlinks) /* valid link */
		{
			struct link *l=&fd->f_data->links[a->link_num];
			struct form_state *fs;
			int xx,yy;
			
			if (!l->form)return;
			if (l->type==L_AREA)
			{
				struct line_info *ln;
				if (!(fs=find_form_state(fd,l->form)))return;

				if (g_char_width(a->style,' ')) {
					xx=x/g_char_width(a->style,' ');
				} else xx=x;
				xx+=fs->vpos;
				xx=xx<0?0:xx;
				yy=a->link_order;
				yy+=fs->vypos;
				if ((ln = format_text(fs->value, l->form->cols, l->form->wrap))) {
					int a;
					for (a = 0; ln[a].st; a++) if (a==yy){
						int bla=utf8_diff(ln[a].en,ln[a].st);
						
						fs->state=ln[a].st-fs->value;
						fs->state = utf8_add(fs->value + fs->state, xx<bla?xx:bla) - fs->value;
						break;
					}
					mem_free(ln);
				}
				return;
			}
			if (l->type!=L_FIELD||!(fs=find_form_state(fd,l->form)))return;
			if (g_char_width(a->style,' ')) {
				xx=x/g_char_width(a->style,' ');
			} else xx=x;
			fs->state=utf8_add(fs->value + (fs->vpos > strlen((char *)fs->value) ? strlen((char *)fs->value) : fs->vpos), (xx<0?0:xx)) - fs->value;
		}
	}
}

void g_text_mouse(struct f_data_c *fd, struct g_object_text *a, int x, int y, int b)
{
	int e;
	g_set_current_link(fd, a, x, y, (b == (B_UP | B_LEFT)));

#ifdef JS
	if (fd->vs&&fd->f_data&&fd->vs->current_link>=0&&fd->vs->current_link<fd->f_data->nlinks)
	{
		/* fd->vs->current links is a valid link */

		struct link *l=&(fd->f_data->links[fd->vs->current_link]);

		if (l->js_event&&l->js_event->up_code&&(b&BM_ACT)==B_UP)
			jsint_execute_code(fd,l->js_event->up_code,strlen((char *)l->js_event->up_code),-1,-1,-1);

		if (l->js_event&&l->js_event->down_code&&(b&BM_ACT)==B_DOWN)
			jsint_execute_code(fd,l->js_event->down_code,strlen((char *)l->js_event->down_code),-1,-1,-1);
		
	}
#endif

	if (b == (B_UP | B_LEFT)) {
		ismap_x = x;
		ismap_y = y;
		ismap_link = a->ismap;
		e = enter(fd->ses, fd, 1);
		ismap_x = 0;
		ismap_y = 0;
		ismap_link = 0;
		if (e) {
			print_all_textarea = 1;
			draw_one_object(fd, (struct g_object *)a);
			print_all_textarea = 0;
		}
		if (e == 2) fd->f_data->locked_on = (struct g_object *)a;
		return;
	}
	if (b == (B_UP | B_RIGHT) || b == (B_UP | B_MIDDLE)) {
		if (fd->vs->current_link != -1) link_menu(fd->ses->term, NULL, fd->ses);
	}
}

void process_sb_event(struct f_data_c *fd, int off, int h)
{
	int spos, epos;
	int w = h ? fd->f_data->hsbsize : fd->f_data->vsbsize;
	get_scrollbar_pos(w - 4, h ? fd->f_data->x : fd->f_data->y, w, h ? fd->vs->view_posx : fd->vs->view_pos, &spos, &epos);
	spos += 2;
	epos += 2;
	/*debug("%d %d %d", spos, epos, off);*/
	if (off >= spos && off < epos) {
		fd->ses->scrolling = 1;
		fd->ses->scrolltype = h;
		fd->ses->scrolloff = off - spos - 1;
		return;
	}
	if (off < spos) if (h) fd->vs->view_posx -= fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH;
			else fd->vs->view_pos -= fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
	else if (h) fd->vs->view_posx += fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH;
		else fd->vs->view_pos += fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
	draw_graphical_doc(fd->ses->term, fd, 1);
}

void process_sb_move(struct f_data_c *fd, int off)
{
	int h = fd->ses->scrolltype;
	int w = h ? fd->f_data->hsbsize : fd->f_data->vsbsize;
	int rpos = off - 2 - fd->ses->scrolloff;
	int st, en;
	get_scrollbar_pos(w - 4, h ? fd->f_data->x : fd->f_data->y, w, h ? fd->vs->view_posx : fd->vs->view_pos, &st, &en);
	if (en - st >= w - 4) return;
	/*
	*(h ? &fd->vs->view_posx : &fd->vs->view_pos) = rpos * (h ? fd->f_data->x : fd->f_data->y) / (w - 4);
	*/
	if (!(w - 4 - (en - st))) return;
	*(h ? &fd->vs->view_posx : &fd->vs->view_pos) = rpos * (h ? fd->f_data->x - w : fd->f_data->y - w) / (w - 4 - (en - st));
	draw_graphical_doc(fd->ses->term, fd, 1);
}

inline int ev_in_rect(struct event *ev, int x1, int y1, int x2, int y2)
{
	return ev->x >= x1 && ev->y >= y1 && ev->x < x2 && ev->y < y2;
}

int is_link_in_view(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	return fd->vs->view_pos < l->r.y2 && fd->vs->view_pos + fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH > l->r.y1;
}

int skip_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	return !l->where && !l->form;
}

void redraw_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	struct rect r;
	memcpy(&r, &l->r, sizeof(struct rect));
	r.x1 += fd->f_data->opt.xp - fd->vs->view_posx;
	r.x2 += fd->f_data->opt.xp - fd->vs->view_posx;
	r.y1 += fd->f_data->opt.yp - fd->vs->view_pos;
	r.y2 += fd->f_data->opt.yp - fd->vs->view_pos;
	t_redraw(fd->ses->term->dev, &r);
}

int lr_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	int xx = fd->vs->view_posx;
	if (l->r.x2 > fd->vs->view_posx + fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH) fd->vs->view_posx = l->r.x2 - (fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH);
	if (l->r.x1 < fd->vs->view_posx) fd->vs->view_posx = l->r.x1;
	return xx != fd->vs->view_posx;
}

int g_next_link(struct f_data_c *fd, int dir)
{
	int r = 2;
	int n, pn;
	if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
		n = (pn = fd->vs->current_link) + dir;
	} else retry: n = dir > 0 ? 0 : fd->f_data->nlinks - 1, pn = -1;
	again:
	if (n < 0 || n >= fd->f_data->nlinks) {
		if (r == 1) {
			fd->vs->current_link = -1;
			fd->ses->locked_link = 0;
			return 1;
		}
		if (dir < 0) {
			if (!fd->vs->view_pos) return 0;
			fd->vs->view_pos -= fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
			/*if (fd->vs->view_pos < 0) fd->vs->view_pos = 0;*/
		} else {
			if (fd->vs->view_pos >= fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
			fd->vs->view_pos += fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
			/*if (fd->vs->view_pos > fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) fd->vs->view_pos = fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
			if (fd->vs->view_pos < 0) fd->vs->view_pos = 0;*/
		}
		r = 1;
		goto retry;
	}
	if (!is_link_in_view(fd, n) || skip_link(fd, n)) {
		n += dir;
		goto again;
	}
	if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
		redraw_link(fd, fd->vs->current_link);
	}
	fd->vs->current_link = n;
	fd->vs->g_display_link = 1;
	redraw_link(fd, n);
	fd->ses->locked_link = 0;
	if (fd->f_data->links[fd->vs->current_link].type == L_FIELD || fd->f_data->links[fd->vs->current_link].type == L_AREA) {
		if ((fd->f_data->locked_on = fd->f_data->links[fd->vs->current_link].obj)) fd->ses->locked_link = 1;
	}
	set_textarea(fd->ses, fd, dir < 0 ? KBD_DOWN : KBD_UP);
	change_screen_status(fd->ses);
	print_screen_status(fd->ses);
	if (lr_link(fd, fd->vs->current_link)) r = 1;
	return r;
}

void unset_link(struct f_data_c *fd)
{
	int n = fd->vs->current_link;
	fd->vs->current_link = -1;
	fd->vs->g_display_link = 0;
	fd->ses->locked_link = 0;
	if (n >= 0 && n < fd->f_data->nlinks) {
		redraw_link(fd, n);
	}
}

int g_frame_ev(struct session *ses, struct f_data_c *fd, struct event *ev)
{
	int x, y;
	if (!fd->f_data) return 0;
	switch (ev->ev) {
		case EV_MOUSE:
			if ((ev->b & BM_BUTT) == B_WHEELUP) goto up;
			if ((ev->b & BM_BUTT) == B_WHEELDOWN) goto down;
			if ((ev->b & BM_BUTT) == B_WHEELUP1) goto up1;
			if ((ev->b & BM_BUTT) == B_WHEELDOWN1) goto down1;
			if ((ev->b & BM_BUTT) == B_WHEELLEFT) goto left;
			if ((ev->b & BM_BUTT) == B_WHEELRIGHT) goto right;
			if ((ev->b & BM_BUTT) == B_WHEELLEFT1) goto left1;
			if ((ev->b & BM_BUTT) == B_WHEELRIGHT1) goto right1;
			x = ev->x;
			y = ev->y;
			if ((ev->b & BM_ACT) == B_MOVE) ses->scrolling = 0;
			if (ses->scrolling == 1) process_sb_move(fd, ses->scrolltype ? ev->x : ev->y);
			if (ses->scrolling == 2) {
				fd->vs->view_posx = -ev->x + ses->scrolltype;
				fd->vs->view_pos = -ev->y + ses->scrolloff;
				draw_graphical_doc(fd->ses->term, fd, 1);
				break;
			}
			if (ses->scrolling) {
				if ((ev->b & BM_ACT) == B_UP) {
					ses->scrolling = 0;
				}
				break;
			}
			if ((ev->b & BM_ACT) == B_DOWN && (ev->b & BM_BUTT) == B_MIDDLE) {
				scrll:
				ses->scrolltype = ev->x + fd->vs->view_posx;
				ses->scrolloff = ev->y + fd->vs->view_pos;
				ses->scrolling = 2;
				break;
			}
			if ((ev->b & BM_ACT) == B_DOWN && fd->f_data->vsb && ev_in_rect(ev, fd->xw - G_SCROLL_BAR_WIDTH, 0, fd->xw, fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH)) {
				process_sb_event(fd, ev->y, 0);
				break;
			}
			if ((ev->b & BM_ACT) == B_DOWN && fd->f_data->hsb && ev_in_rect(ev, 0, fd->yw - G_SCROLL_BAR_WIDTH, fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH, fd->yw)) {
				process_sb_event(fd, ev->x, 1);
				break;
			}
			if (fd->f_data->vsb && ev_in_rect(ev, fd->xw - G_SCROLL_BAR_WIDTH, 0, fd->xw, fd->yw)) return 0;
			if (fd->f_data->hsb && ev_in_rect(ev, 0, fd->yw - G_SCROLL_BAR_WIDTH, fd->xw, fd->yw)) return 0;
			previous_link=fd->vs->current_link;
			if (fd->vs->g_display_link) {
				fd->vs->g_display_link = 0;
				if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) redraw_link(fd, fd->vs->current_link);
			}
			fd->vs->current_link = -1;
			fd->f_data->root->mouse_event(fd, fd->f_data->root, ev->x + fd->vs->view_posx, ev->y + fd->vs->view_pos, ev->b);
			if (previous_link!=fd->vs->current_link)
				change_screen_status(ses);
			print_screen_status(ses);
#ifdef JS
			/* process onmouseover/onmouseout handlers */

			if (previous_link!=fd->vs->current_link)
			{
				struct link* lnk=NULL;

			if (previous_link>=0&&previous_link<fd->f_data->nlinks)lnk=&(fd->f_data->links[previous_link]);
				if (lnk&&lnk->js_event&&lnk->js_event->out_code)
					jsint_execute_code(fd,lnk->js_event->out_code,strlen((char *)lnk->js_event->out_code),-1,-1,-1);
				lnk=NULL;
				if (fd->vs->current_link>=0&&fd->vs->current_link<fd->f_data->nlinks)lnk=&(fd->f_data->links[fd->vs->current_link]);
				if (lnk&&lnk->js_event&&lnk->js_event->over_code)
					jsint_execute_code(fd,lnk->js_event->over_code,strlen((char *)lnk->js_event->over_code),-1,-1,-1);
			}

			if ((ev->b & BM_ACT) == B_DOWN && (ev->b & BM_BUTT) == B_RIGHT && fd->vs->current_link == -1) goto scrll;
#endif
			break;
		case EV_KBD:
			if (ses->locked_link && fd->vs->current_link >= 0 && (fd->f_data->links[fd->vs->current_link].type == L_FIELD || fd->f_data->links[fd->vs->current_link].type == L_AREA)) {
				if (field_op(ses, fd, &fd->f_data->links[fd->vs->current_link], ev, 0)) {
					if (fd->f_data->locked_on) {
						print_all_textarea = 1;
						draw_one_object(fd, fd->f_data->locked_on);
						print_all_textarea = 0;
						return 2;
					}
					return 1;
				}
				if (ev->x == KBD_ENTER) {
					return enter(ses, fd, 0);
				}
			}
			if (ev->x == KBD_RIGHT || ev->x == KBD_ENTER) {
				struct link *l;
				if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
					l = &fd->f_data->links[fd->vs->current_link];
					set_window_ptr(ses->win, fd->f_data->opt.xp + l->r.x1 - fd->vs->view_posx, fd->f_data->opt.yp + l->r.y1 - fd->vs->view_pos);
				} else {
					set_window_ptr(ses->win, fd->f_data->opt.xp, fd->f_data->opt.yp);
				}
				return enter(ses, fd, 0);
			}
			if (ev->x == KBD_PAGE_DOWN || (ev->x == ' ' && (!ev->y || ev->y== KBD_CTRL)) || (upcase(ev->x) == 'F' && ev->y & KBD_CTRL)) {
				unset_link(fd);
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
				return 3;
			}
			if (ev->x == KBD_PAGE_UP || (upcase(ev->x) == 'B' && (!ev->y || ev->y == KBD_CTRL)) || (upcase(ev->x) == 'B' && ev->y & KBD_CTRL)) {
				unset_link(fd);
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
				return 3;
			}
			if (0) {
				down:
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += 64;
				return 3;
			}
			if (0) {
				up:
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= 64;
				return 3;
			}
			if (ev->x == KBD_DEL || (upcase(ev->x) == 'N' && ev->y == KBD_CTRL)) {
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += 32;
				return 3;
			}
			if (ev->x == KBD_INS || (upcase(ev->x) == 'P' && ev->y == KBD_CTRL)) {
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= 32;
				return 3;
			}
			if (/*ev->x == KBD_DOWN*/ 0) {
				down1:
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += 16;
				return 3;
			}
			if (/*ev->x == KBD_UP*/ 0) {
				up1:
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= 16;
				return 3;
			}
			if (ev->x == KBD_DOWN) {
				return g_next_link(fd, 1);
			}
			if (ev->x == KBD_UP) {
				return g_next_link(fd, -1);
			}
			if (ev->x == '[') {
				left:
				if (!fd->vs->view_posx) return 0;
				fd->vs->view_posx -= 64;
				return 3;
			}
			if (ev->x == ']') {
				right:
				if (fd->vs->view_posx == fd->f_data->x - fd->xw + fd->f_data->vsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_posx += 64;
				return 3;
			}
			if (/*ev->x == KBD_LEFT*/ 0) {
				left1:
				if (!fd->vs->view_posx) return 0;
				fd->vs->view_posx -= 16;
				return 3;
			}
			if (/*ev->x == KBD_RIGHT*/ 0) {
				right1:
				if (fd->vs->view_posx == fd->f_data->x - fd->xw + fd->f_data->vsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_posx += 16;
				return 3;
			}
			if (ev->x == KBD_HOME || (upcase(ev->x == 'A') && ev->y & KBD_CTRL)) {
				fd->vs->view_pos = 0;
				unset_link(fd);
				return 3;
			}
			if (ev->x == KBD_END || (upcase(ev->x == 'E') && ev->y & KBD_CTRL)) {
				fd->vs->view_pos = fd->f_data->y;
				unset_link(fd);
				return 3;
			}
			if (upcase(ev->x) == 'F' && !(ev->y & KBD_ALT)) {
				set_frame(ses, fd, 0);
				return 2;
			}
			if (ev->x == '#') {
				ses->ds.images ^= 1;
				html_interpret_recursive(fd);
				ses->ds.images ^= 1;
				return 1;
			}
			if (ev->x == 'i' && !(ev->y & KBD_ALT)) {
				if (!F || fd->f_data->opt.plain != 2) frm_view_image(ses, fd);
				return 2;
			}
			if (ev->x == 'I' && !(ev->y & KBD_ALT)) {
				if (!anonymous) frm_download_image(ses, fd);
				return 2;
			}
			if (upcase(ev->x) == 'D' && !(ev->y & KBD_ALT)) {
				if (!anonymous) frm_download(ses, fd);
				return 2;
			}
			if (ev->x == '/') {
				search_dlg(ses, fd, 0);
				return 2;
			}
			if (ev->x == '?') {
				search_back_dlg(ses, fd, 0);
				return 2;
			}
			if (ev->x == 'n' && !(ev->y & KBD_ALT)) {
				find_next(ses, fd, 0);
				return 2;
			}
			if (ev->x == 'N' && !(ev->y & KBD_ALT)) {
				find_next_back(ses, fd, 0);
				return 2;
			}
			break;
	}
	return 0;
}

void draw_title(struct f_data_c *f)
{
	unsigned char *title = stracpy(!drv->set_title ? f->f_data && f->f_data->title ? f->f_data->title : (unsigned char *)"" : f->rq && f->rq->url ? f->rq->url : (unsigned char *)"");
	int b, z, w;
	struct graphics_device *dev = f->ses->term->dev;
	if (drv->set_title && strchr((char *)title, POST_CHAR)) *strchr((char *)title, POST_CHAR) = 0;
	w = g_text_width(bfu_style_bw, title);
	z = 0;
	g_print_text(drv, dev, 0, 0, bfu_style_bw, (unsigned char *)" <- ", &z);
	f->ses->back_size = z;
	b = (dev->size.x2 - w) - 16;
	if (b < z) b = z;
	drv->fill_area(dev, z, 0, b, G_BFU_FONT_SIZE, bfu_bg_color);
	g_print_text(drv, dev, b, 0, bfu_style_bw, title, &b);
	drv->fill_area(dev, b, 0, dev->size.x2, G_BFU_FONT_SIZE, bfu_bg_color);
	mem_free(title);
}

struct f_data *srch_f_data;

void get_searched_sub(struct g_object *p, struct g_object *c)
{
	if (c->draw == (void (*)(struct f_data_c *, struct g_object *, int, int))g_text_draw) {
		struct g_object_text *t = (struct g_object_text *)c;
		int pos = srch_f_data->srch_string_size;
		t->srch_pos = pos;
		add_to_str(&srch_f_data->srch_string, &srch_f_data->srch_string_size, t->text);
	}
	if (c->get_list) c->get_list(c, get_searched_sub);
	if (c->draw == (void (*)(struct f_data_c *, struct g_object *, int, int))g_line_draw) {
		if (srch_f_data->srch_string_size && srch_f_data->srch_string[srch_f_data->srch_string_size - 1] != ' ')
			add_to_str(&srch_f_data->srch_string, &srch_f_data->srch_string_size, (unsigned char *)" ");
	}
}

void g_get_search_data(struct f_data *f)
{
	srch_f_data = f;
	if (f->srch_string) return;
	f->srch_string = init_str();
	f->srch_string_size = 0;
	if (f->root && f->root->get_list) f->root->get_list(f->root, get_searched_sub);
	while (f->srch_string_size && f->srch_string[f->srch_string_size - 1] == ' ') {
		f->srch_string[--f->srch_string_size] = 0;
	}
}

unsigned char *search_word;

int find_refline;
int find_direction;

int find_opt_yy;
int find_opt_y;
int find_opt_yw;
int find_opt_x;
int find_opt_xw;

void find_next_sub(struct g_object *p, struct g_object *c)
{
	if (c->draw == (void (*)(struct f_data_c *, struct g_object *, int, int))g_text_draw) {
		struct g_object_text *t = (struct g_object_text *)c;
		int start = t->srch_pos;
		int end = t->srch_pos + strlen((char *)t->text);
		int found;
		BIN_SEARCH(highlight_positions, n_highlight_positions, B_EQUAL, B_ABOVE, *, found);
		if (found != -1) {
			int x, y, yy;
			get_object_pos(c, &x, &y);
			y += t->yw / 2;
			yy = y;
			if (yy < find_refline) yy += MAXINT / 2;
			if (find_direction < 0) yy = MAXINT - yy;
			if (find_opt_yy == -1 || yy > find_opt_yy) {
				int i, l, ll;
				find_opt_yy = yy;
				find_opt_y = y;
				find_opt_yw = t->style->height;
				find_opt_x = x;
				find_opt_xw = t->xw;
				l = strlen((char *)t->text);
				ll = strlen((char *)search_word);
				for (i = 0; i <= l - ll; i++) {
					int j;
					unsigned char *tt;
					for (j = 0; j < ll; j++) if (srch_cmp(t->text[i + j], search_word[j])) goto no_ch;
					tt = memacpy(t->text, i);
					find_opt_x += g_text_width(t->style, tt);
					find_opt_xw = g_text_width(t->style, search_word);
					mem_free(tt);
					goto fnd;
					no_ch:;
				}
				fnd:;
				/*debug("-%s-%s-: %d %d", t->text, search_word, find_opt_x, find_opt_xw);*/
			}
		}
	}
	if (c->get_list) c->get_list(c, find_next_sub);
}

void g_find_next_str(struct f_data *f)
{
	find_opt_yy = -1;
	if (f->root && f->root->get_list) f->root->get_list(f->root, find_next_sub);
}

void g_find_next(struct f_data_c *f, int a)
{
	g_get_search_data(f->f_data);
	g_get_search(f->f_data, f->ses->search_word);
	search_word = f->ses->search_word;
	if (!f->f_data->n_search_positions) msg_box(f->ses->term, NULL, TEXT(T_SEARCH), AL_CENTER, TEXT(T_SEARCH_STRING_NOT_FOUND), NULL, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC);

	highlight_positions = f->f_data->search_positions;
	n_highlight_positions = f->f_data->n_search_positions;
	highlight_position_size = f->f_data->last_search_len;

	if ((!a && f->ses->search_direction == -1) ||
	     (a && f->ses->search_direction == 1)) find_refline = f->vs->view_pos;
	else find_refline = f->vs->view_pos + f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH;
	find_direction = -f->ses->search_direction;

	g_find_next_str(f->f_data);

	highlight_positions = NULL;
	n_highlight_positions = highlight_position_size = 0;

	if (find_opt_yy == -1) goto d;
	if (!a || find_opt_y < f->vs->view_pos || find_opt_y + find_opt_yw >= f->vs->view_pos + f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH) {
		f->vs->view_pos = find_opt_y - (f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH) / 2;
	}
	if (find_opt_x < f->vs->view_posx || find_opt_x + find_opt_xw >= f->vs->view_posx + f->xw - f->f_data->vsb * G_SCROLL_BAR_WIDTH) {
		f->vs->view_posx = find_opt_x + find_opt_xw / 2 - (f->xw - f->f_data->vsb * G_SCROLL_BAR_WIDTH) / 2;
	}

	d:draw_fd(f);
}

void init_grview(void)
{
	int i, w = g_text_width(bfu_style_wb_mono, (unsigned char *)" ");
	for (i = 32; i < 128; i++) {
		unsigned char a[2];
		a[0] = i, a[1] = 0;
		if (g_text_width(bfu_style_wb_mono, a) != w) internal((unsigned char *)"Monospaced font is not monospaced (error at char %d, width %d, wanted width %d)", i, (int)g_text_width(bfu_style_wb_mono, a), w);
	}
	scroll_bar_frame_color = dip_get_color_sRGB(G_SCROLL_BAR_FRAME_COLOR);
	scroll_bar_area_color = dip_get_color_sRGB(G_SCROLL_BAR_AREA_COLOR);
	scroll_bar_bar_color = dip_get_color_sRGB(G_SCROLL_BAR_BAR_COLOR);
}

#endif
