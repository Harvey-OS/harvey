/* objreq.c
 * Object Requester
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

void objreq_end(struct status *, struct object_request *);
void object_timer(struct object_request *);

struct list_head requests = {&requests, &requests};
tcount obj_req_count = 1;

#define LL gf_val(1, G_BFU_FONT_SIZE)

#define MAX_UID_LEN 256

struct auth_dialog {
	unsigned char uid[MAX_UID_LEN];
	unsigned char passwd[MAX_UID_LEN];
	unsigned char *realm;
	int proxy;
	unsigned char msg[1];
};

static inline struct object_request *find_rq(tcount c)
{
	struct object_request *rq;
	foreach(rq, requests) if (rq->count == c) return rq;
	return NULL;
}

void auth_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	struct auth_dialog *a = dlg->dlg->udata;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	max_text_width(term, a->msg, &max, AL_LEFT);
	min_text_width(term, a->msg, &min, AL_LEFT);
	max_text_width(term, TEXT(T_USERID), &max, AL_LEFT);
	min_text_width(term, TEXT(T_USERID), &min, AL_LEFT);
	max_text_width(term, TEXT(T_PASSWORD), &max, AL_LEFT);
	min_text_width(term, TEXT(T_PASSWORD), &min, AL_LEFT);
	max_buttons_width(term, dlg->items + 2, 2, &max);
	min_buttons_width(term, dlg->items + 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	rw = w;
	dlg_format_text(dlg, NULL, a->msg, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += LL;
	dlg_format_text(dlg, NULL, TEXT(T_USERID), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, NULL, dlg->items, 0, &y, w, &rw, AL_LEFT);
	y += LL;
	dlg_format_text(dlg, NULL, TEXT(T_PASSWORD), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, NULL, dlg->items + 1, 0, &y, w, &rw, AL_LEFT);
	y += LL;
	dlg_format_buttons(dlg, NULL, dlg->items + 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	y += LL;
	dlg_format_text(dlg, term, a->msg, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += LL;
	dlg_format_text(dlg, term, TEXT(T_USERID), dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += LL;
	dlg_format_text(dlg, term, TEXT(T_PASSWORD), dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items + 1, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += LL;
	dlg_format_buttons(dlg, term, dlg->items + 2, 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

int auth_cancel(struct dialog_data *dlg, struct dialog_item_data *item)
{
	struct object_request *rq = find_rq((int)dlg->dlg->udata2);
	if (rq) {
		rq->state = O_OK;
		if (rq->timer != -1) kill_timer(rq->timer);
		rq->timer = install_timer(0, (void (*)(void *))object_timer, rq);
		(rq->ce = rq->stat.ce)->refcount++;
	}
	cancel_dialog(dlg, item);
	return 0;
}

int auth_ok(struct dialog_data *dlg, struct dialog_item_data *item)
{
	struct object_request *rq = find_rq((int)dlg->dlg->udata2);
	if (rq) {
		struct auth_dialog *a = dlg->dlg->udata;
		struct session *ses;
		struct conv_table *ct;
		int net_cp;
		unsigned char *uid, *passwd;
		get_dialog_data(dlg);
		ses = ((struct window *)dlg->win->term->windows.prev)->data;
		ct = get_convert_table(rq->stat.ce->head, dlg->win->term->spec->charset, ses->ds.assume_cp, &net_cp, NULL, ses->ds.hard_assume);
		ct = get_translation_table(dlg->win->term->spec->charset, net_cp);
		uid = convert_string(ct, a->uid, strlen((char *)a->uid), NULL);
		passwd = convert_string(ct, a->passwd, strlen((char *)a->passwd), NULL);
		add_auth(rq->url, a->realm, uid, passwd, a->proxy);
		mem_free(uid);
		mem_free(passwd);
		change_connection(&rq->stat, NULL, PRI_CANCEL);
		load_url(rq->url, rq->prev_url, &rq->stat, rq->pri, NC_RELOAD);
	}
	cancel_dialog(dlg, item);
	return 0;
}

int auth_window(struct object_request *rq, unsigned char *realm)
{
	unsigned char *host, *port;
	struct dialog *d;
	struct auth_dialog *a;
	struct terminal *term;
	struct conv_table *ct;
	unsigned char *urealm;
	struct session *ses;
	foreach(term, terminals) if (rq->term == term->count) goto ok;
	return -1;
	ok:
	ses = ((struct window *)term->windows.prev)->data;
	ct = get_convert_table(rq->stat.ce->head, term->spec->charset, ses->ds.assume_cp, NULL, NULL, ses->ds.hard_assume);
	if (rq->stat.ce->http_code == 407) host = stracpy(http_proxy);
	else {
		host = get_host_name(rq->url);
		if (!host) return -1;
		if ((port = get_port_str(rq->url))) {
			add_to_strn(&host, (unsigned char *)":");
			add_to_strn(&host, port);
			mem_free(port);
		}
	}
	urealm = convert_string(ct, realm, strlen((char *)realm), NULL);
	if (!(d = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item) + sizeof(struct auth_dialog) + strlen((char *)_(TEXT(T_ENTER_USERNAME), term)) + strlen((char *)urealm) + 1 + strlen((char *)_(TEXT(T_AT), term)) + strlen((char *)host) + + 1))) {
		mem_free(host);
		mem_free(urealm);
		return -1;
	}
	memset(d, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item) + sizeof(struct auth_dialog));
	a = (struct auth_dialog *)((unsigned char *)d + sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
	strcpy((char *)a->msg, (char *)_(TEXT(T_ENTER_USERNAME), term));
	strcat((char *)a->msg, (char *)urealm);
	strcat((char *)a->msg, "\n");
	strcat((char *)a->msg, (char *)_(TEXT(T_AT), term));
	strcat((char *)a->msg, (char *)host);
	mem_free(host);
	mem_free(urealm);
	a->proxy = rq->stat.ce->http_code == 407;
	a->realm = stracpy(realm);
	d->udata = a;
	d->udata2 = (void *)rq->count;
	if (rq->stat.ce->http_code == 401) d->title = TEXT(T_AUTHORIZATION_REQUIRED);
	else d->title = TEXT(T_PROXY_AUTHORIZATION_REQUIRED);
	d->fn = auth_fn;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_UID_LEN;
	d->items[0].data = a->uid;

	d->items[1].type = D_FIELD_PASS;
	d->items[1].dlen = MAX_UID_LEN;
	d->items[1].data = a->passwd;

	d->items[2].type = D_BUTTON;
	d->items[2].gid = B_ENTER;
	d->items[2].fn = auth_ok;
	d->items[2].text = TEXT(T_OK);

	d->items[3].type = D_BUTTON;
	d->items[3].gid = B_ESC;
	d->items[3].fn = auth_cancel;
	d->items[3].text = TEXT(T_CANCEL);

	do_dialog(term, d, getml(d, a->realm, NULL));
	return 0;
}

/* prev_url is a pointer to previous url or NULL */
/* prev_url will NOT be deallocated */
void request_object(struct terminal *term, unsigned char *url, unsigned char *prev_url, int pri, int cache, void (*upcall)(struct object_request *, void *), void *data, struct object_request **rqp)
{
	struct object_request *rq;
	if (!(rq = mem_calloc(sizeof(struct object_request)))) return;
	rq->state = O_WAITING;
	rq->refcount = 1;
	rq->term = term ? term->count : 0;
	rq->stat.end = (void (*)(struct status *, void *))objreq_end;
	rq->stat.data = rq;
	if (!(rq->orig_url = stracpy(url))) {
		mem_free(rq);
		return;
	}
	if (!(rq->url = stracpy(url))) {
		mem_free(rq->orig_url);
		mem_free(rq);
		return;
	}
	rq->pri = pri;
	rq->cache = cache;
	rq->upcall = upcall;
	rq->data = data;
	rq->timer = -1;
	rq->z = get_time() - STAT_UPDATE_MAX;
	rq->last_update = rq->z;
	rq->last_bytes = 0;
	if (rq->prev_url)mem_free(rq->prev_url);
	rq->prev_url=stracpy(prev_url);
	if (rqp) *rqp = rq;
	rq->count = obj_req_count++;
	add_to_list(requests, rq);
	load_url(url, prev_url, &rq->stat, pri, cache);
}

void objreq_end(struct status *stat, struct object_request *rq)
{
	if (stat->state < 0) {
		if (stat->ce && rq->state == O_WAITING && stat->ce->redirect) {
			if (rq->redirect_cnt++ < MAX_REDIRECTS) {
				unsigned char *u, *p;
				change_connection(stat, NULL, PRI_CANCEL);
				u = join_urls(rq->url, stat->ce->redirect);
				if (!http_bugs.bug_302_redirect && !stat->ce->redirect_get && (p = (unsigned char *)strchr((char *)u, POST_CHAR))) add_to_strn(&u, p);
				mem_free(rq->url);
				rq->url = u;
				load_url(u, rq->prev_url, &rq->stat, rq->pri, rq->cache);
				return;
			} else {
				rq->stat.state = S_CYCLIC_REDIRECT;
			}
		}
		if (stat->ce && rq->state == O_WAITING && (stat->ce->http_code == 401 || stat->ce->http_code == 407)) {
			unsigned char *realm = get_auth_realm(rq->url, stat->ce->head, stat->ce->http_code == 407);
			if (stat->ce->http_code == 401 && !find_auth(rq->url, realm)) {
				mem_free(realm);
				change_connection(stat, NULL, PRI_CANCEL);
				load_url(rq->url, rq->prev_url, &rq->stat, rq->pri, NC_RELOAD);
				return;
			}
			if (!auth_window(rq, realm)) {
				mem_free(realm);
				goto tm;
			}
			mem_free(realm);
			goto xx;
		}
	}
	if ((stat->state < 0 || stat->state == S_TRANS) && stat->ce && !stat->ce->redirect && stat->ce->http_code != 401 && stat->ce->http_code != 407) {
		xx:
		rq->state = O_LOADING;
		if (!rq->ce) (rq->ce = stat->ce)->refcount++;
	}
	tm:
	if (rq->timer != -1) kill_timer(rq->timer);
	rq->timer = install_timer(0, (void (*)(void *))object_timer, rq);
}

void object_timer(struct object_request *rq)
{
	int last = rq->last_bytes;
	if (rq->ce) rq->last_bytes = rq->ce->length;
	rq->timer = -1;
	if (rq->stat.state < 0 && (!rq->stat.ce || (!rq->stat.ce->redirect && rq->stat.ce->http_code != 401 && rq->stat.ce->http_code != 407) || rq->stat.state == S_CYCLIC_REDIRECT)) {
		if (rq->stat.ce && rq->stat.state != S_CYCLIC_REDIRECT) {
			rq->state = rq->stat.state != S_OK ? O_INCOMPLETE : O_OK;
			/*(rq->ce = rq->stat.ce)->refcount++;*/
		} else rq->state = O_FAILED;
	}
	if (rq->stat.state != S_TRANS) {
		rq->last_update = rq->z;
		if (rq->upcall) rq->upcall(rq, rq->data);
	} else {
		ttime ct = get_time();
		ttime t = ct - rq->last_update;
		rq->timer = install_timer(STAT_UPDATE_MIN, (void (*)(void *))object_timer, rq);
		if (t >= STAT_UPDATE_MAX || (t >= STAT_UPDATE_MIN && rq->ce && rq->last_bytes > last)) {
			rq->last_update = ct;
			if (rq->upcall) rq->upcall(rq, rq->data);
		}
	}
}

void release_object_get_stat(struct object_request **rqq, struct status *news, int pri)
{
	struct object_request *rq = *rqq;
	if (!rq) return;
	*rqq = NULL;
	if (--rq->refcount) return;
	change_connection(&rq->stat, news, pri);
	if (rq->timer != -1) kill_timer(rq->timer);
	if (rq->ce) rq->ce->refcount--;
	mem_free(rq->orig_url);
	mem_free(rq->url);
	if (rq->prev_url)mem_free(rq->prev_url);
	del_from_list(rq);
	mem_free(rq);
}

void release_object(struct object_request **rqq)
{
	release_object_get_stat(rqq, NULL, PRI_CANCEL);
}

void detach_object_connection(struct object_request *rq, int pos)
{
	if (rq->state == O_WAITING || rq->state == O_FAILED) {
		internal((unsigned char *)"detach_object_connection: no data received");
		return;
	}
	if (rq->refcount == 1) detach_connection(&rq->stat, pos);
}

void clone_object(struct object_request *rq, struct object_request **rqq)
{
	(*rqq = rq)->refcount++;
}
