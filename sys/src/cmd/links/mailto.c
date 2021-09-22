/* mailto.c
 * mailto:// processing
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

void prog_func(struct terminal *term, struct list_head *list, unsigned char *param, unsigned char *name)
{
	unsigned char *prog, *cmd;
	if (!(prog = get_prog(list)) || !*prog) {
		msg_box(term, NULL, TEXT(T_NO_PROGRAM), AL_CENTER | AL_EXTD_TEXT, TEXT(T_NO_PROGRAM_SPECIFIED_FOR), " ", name, ".", NULL, NULL, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return;
	}
	if ((cmd = subst_file(prog, param))) {
		exec_on_terminal(term, cmd, (unsigned  char *)"", 1);
		mem_free(cmd);
	}
}

void mailto_func(struct session *ses, unsigned char *url)
{
	unsigned char *user, *host, *m;
	int f = 1;
	if (!(user = get_user_name(url))) goto fail;
	if (!(host = get_host_name(url))) goto fail1;
	if (!(m = mem_alloc(strlen(( char *)user) + strlen(( char *)host) + 2))) goto fail2;
	f = 0;
	strcpy(( char *)m, ( char *)user);
	strcat(( char *)m, "@");
	strcat(( char *)m, ( char *)host);
	check_shell_security(&m);
	prog_func(ses->term, &mailto_prog, m, TEXT(T_MAIL));
	mem_free(m);
	fail2:
	mem_free(host);
	fail1:
	mem_free(user);
	fail:
	if (f) msg_box(ses->term, NULL, TEXT(T_BAD_URL_SYNTAX), AL_CENTER, TEXT(T_BAD_MAILTO_URL), NULL, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC);
}

void tn_func(struct session *ses, unsigned char *url, struct list_head *prog, unsigned char *t1, unsigned char *t2)
{
	unsigned char *m;
	unsigned char *h, *p;
	int hl, pl;
	unsigned char *hh, *pp;
	int f = 1;
	if (parse_url(url, NULL, NULL, NULL, NULL, NULL, &h, &hl, &p, &pl, NULL, NULL, NULL) || !hl) goto fail;
	if (!(hh = memacpy(h, hl))) goto fail;
	if (pl && !(pp = memacpy(p, pl))) goto fail1;
	check_shell_security(&hh);
	if (pl) check_shell_security(&pp);
	if (!(m = mem_alloc(strlen(( char *)hh) + (pl ? strlen(( char *)pp) : 0) + 2))) goto fail2;
	f = 0;
	strcpy(( char *)m, ( char *)hh);
	if (pl) {
		strcat((  char *)m, " ");
		strcat((  char *)m, (  char *)pp);
		m[hl + 1 + pl] = 0;
	}
	prog_func(ses->term, prog, m, t1);
	mem_free(m);
	fail2:
	if (pl) mem_free(pp);
	fail1:
	mem_free(hh);
	fail:
	if (f) msg_box(ses->term, NULL, TEXT(T_BAD_URL_SYNTAX), AL_CENTER, t2, NULL, 1, TEXT(T_CANCEL), NULL, B_ENTER | B_ESC);
}


void telnet_func(struct session *ses, unsigned char *url)
{
	tn_func(ses, url, &telnet_prog, TEXT(T_TELNET), TEXT(T_BAD_TELNET_URL));
}

void tn3270_func(struct session *ses, unsigned char *url)
{
	tn_func(ses, url, &tn3270_prog, TEXT(T_TN3270), TEXT(T_BAD_TN3270_URL));
}
