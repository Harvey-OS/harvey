/* html.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#define format format_

struct list_head html_stack = {&html_stack, &html_stack};

int html_format_changed = 0;

static inline int atchr(unsigned char c)
{
	return isA(c) || (c > ' ' && c != '=' && c != '<' && c != '>');
}

/* accepts one html element */
/* e is pointer to the begining of the element (*e must be '<') */
/* eof is pointer to the end of scanned area */
/* parsed element name is stored in name, it's length is namelen */
/* first attribute is stored in attr */
/* end points to first character behind the html element */
/* returns: -1 fail (returned values in pointers are invalid) */
/*	    0 success */
int parse_element(unsigned char *e, unsigned char *eof, unsigned char **name, int *namelen, unsigned char **attr, unsigned char **end)
{
	if (eof - e < 3 || *(e++) != '<') return -1;
	if (name) *name = e;
	if (*e == '/') e++;
	if (!isA(*e)) return -1;
	goto x1;
	while (isA(*e) || *e == '=') {
		x1:
		e++;
		if (e >= eof) return -1;
	}
	if ((!WHITECHAR(*e) && *e != '>' && *e != '<' && *e != '/' && *e != ':')) return -1;
	if (name && namelen) *namelen = e - *name;
	while ((WHITECHAR(*e) || *e == '/' || *e == ':')) {
		e++;
		if (e >= eof) return -1;
	}
	if ((!atchr(*e) && *e != '>' && *e != '<')) return -1;
	if (attr) *attr = e;
	nextattr:
	while (WHITECHAR(*e)) {
		e++;
		if (e >= eof) return -1;
	}
	if ((!atchr(*e) && *e != '>' && *e != '<')) return -1;
	if (*e == '>' || *e == '<') goto en;
	while (atchr(*e)) {
		e++;
		if (e >= eof) return -1;
	}
	while (WHITECHAR(*e)) {
		e++;
		if (e >= eof) return -1;
	}
	if (*e != '=') goto endattr;
	goto x2;
	while (WHITECHAR(*e)) {
		x2:
		e++;
		if (e >= eof) return -1;
	}
	if (U(*e)) {
		unsigned char uu = *e;
		u:
		goto x3;
		while (e < eof && *e != uu && *e /*(WHITECHAR(*e) || *e > ' ')*/) {
			x3:
			e++;
			if (e >= eof) return -1;
		}
		if (*e < ' ') return -1;
		e++;
		if (e >= eof /*|| (!WHITECHAR(*e) && *e != uu && *e != '>' && *e != '<')*/) return -1;
		if (*e == uu) goto u;
	} else {
		while (!WHITECHAR(*e) && *e != '>' && *e != '<') {
			e++;
			if (e >= eof) return -1;
		}
	}
	while (WHITECHAR(*e)) {
		e++;
		if (e >= eof) return -1;
	}
	endattr:
	if (*e != '>' && *e != '<') goto nextattr;
	en:
	if (end) *end = e + (*e == '>');
	return 0;
}

unsigned char *_xx_;

#define add_chr(s, l, c)	do {\
	if ((_xx_ = s, l % ALLOC_GR) || (_xx_ = mem_realloc(s, l + ALLOC_GR))) s = _xx_, s[l++] = c;	\
	} while (0)

int get_attr_val_nl = 0;

/* parses html element attributes */
/* e is attr pointer previously get from parse_element, DON'T PASS HERE ANY OTHER VALUE!!! */
/* name is searched attribute */
/* returns allocated string containing the attribute, or NULL on unsuccess */
unsigned char *get_attr_val(unsigned char *e, unsigned char *name)
{
	unsigned char *n;
	unsigned char *a = DUMMY;
	int l = 0;
	int f;
	aa:
	while (WHITECHAR(*e)) e++;
	if (*e == '>' || *e == '<') return NULL;
	n = name;
	while (*n && upcase(*e) == upcase(*n)) e++, n++;
	f = *n;
	while (atchr(*e)) f = 1, e++;
	while (WHITECHAR(*e)) e++;
	if (*e != '=') goto ea;
	e++;
	while (WHITECHAR(*e)) e++;
	if (!U(*e)) {
		while (!WHITECHAR(*e) && *e != '>' && *e != '<') {
			if (!f) add_chr(a, l, *e);
			e++;
		}
	} else {
		char uu = *e;
		a:
		e++;
		while (*e != uu) {
			if (!*e) {
				mem_free(a);
				return NULL;
			}
			if (!f && *e != 13) {
				if (*e != 9 && *e != 10) add_chr(a, l, *e);
				else if (!get_attr_val_nl) add_chr(a, l, ' ');
			}
			e++;
		}
		e++;
		if (*e == uu) {
			if (!f) add_chr(a, l, *e);
			goto a;
		}
	}
	ea:
	if (!f) {
		unsigned char *b;
		add_chr(a, l, 0);
		if (strchr(( char *)a, '&')) {
			unsigned char *aa = a;
			a = convert_string(NULL, aa, strlen(( char *)aa), d_opt);
			mem_free(aa);
		}
		for (b = a; *b == ' '; b++);
		if (b != a) memmove(a, b, strlen(( char *)b) + 1);
		for (b = a + strlen(( char *)a) - 1; b >= a && *b == ' '; b--) *b = 0;
		set_mem_comment(a, name, strlen(( char *)name));
		return a;
	}
	goto aa;
}

int has_attr(unsigned char *e, unsigned char *name)
{
	unsigned char *a;
	if (!(a = get_attr_val(e, name))) return 0;
	mem_free(a);
	return 1;
}

/*
unsigned char *get_url_val(unsigned char *e, unsigned char *name)
{
	int n = 0;
	unsigned char *p, *q, *pp;
	if (!(pp = get_attr_val(e, name))) return NULL;
	p = pp; q = pp;
	while (1) {
		if (*p == '#') n = 1;
		if ((*p = *q) != ' ' || n) p++;
		if (!*q) break;
		q++;
	}
	return pp;
}
*/

unsigned char *get_url_val(unsigned char *e, unsigned char *name)
{
	unsigned char *a;
	get_attr_val_nl = 1;
	a = get_attr_val(e, name);
	get_attr_val_nl = 0;
	return a;
}

struct {
	int n;
	char *s;
} roman_tbl[] = {
	{1000,	"m"},
	{999,	"im"},
/*	{995,	"vm"},*/
	{990,	"xm"},
/*	{950,	"lm"},*/
	{900,	"cm"},
	{500,	"d"},
	{499,	"id"},
/*	{495,	"vd"},*/
	{490,	"xd"},
/*	{450,	"ld"},*/
	{400,	"cd"},
	{100,	"c"},
	{99,	"ic"},
/*	{95,	"vc"},*/
	{90,	"xc"},
	{50,	"l"},
	{49,	"il"},
/*	{45,	"vl"},*/
	{40,	"xl"},
	{10,	"x"},
	{9,	"ix"},
	{5,	"v"},
	{4,	"iv"},
	{1,	"i"},
	{0,	NULL}
};

void roman(char *p, unsigned n)
{
	int i = 0;
	if (n >= 4000) {
		strcpy(p, "---");
		return;
	}
	if (!n) {
		strcpy(p, "o");
		return;
	}
	p[0] = 0;
	while (n) {
		while (roman_tbl[i].n <= n) {
			n -= roman_tbl[i].n;
			strcat(p, roman_tbl[i].s);
		}
		i++;
		if (n && !roman_tbl[i].n) {
			internal((unsigned char *)"BUG in roman number convertor");
			return;
		}
	}
}

struct color_spec {
	char *name;
	int rgb;
};

struct color_spec color_specs[] = {
	{"aliceblue",		0xF0F8FF},
	{"antiquewhite",	0xFAEBD7},
	{"aqua",		0x00FFFF},
	{"aquamarine",		0x7FFFD4},
	{"azure",		0xF0FFFF},
	{"beige",		0xF5F5DC},
	{"bisque",		0xFFE4C4},
	{"black",		0x000000},
	{"blanchedalmond",	0xFFEBCD},
	{"blue",		0x0000FF},
	{"blueviolet",		0x8A2BE2},
	{"brown",		0xA52A2A},
	{"burlywood",		0xDEB887},
	{"cadetblue",		0x5F9EA0},
	{"chartreuse",		0x7FFF00},
	{"chocolate",		0xD2691E},
	{"coral",		0xFF7F50},
	{"cornflowerblue",	0x6495ED},
	{"cornsilk",		0xFFF8DC},
	{"crimson",		0xDC143C},
	{"cyan",		0x00FFFF},
	{"darkblue",		0x00008B},
	{"darkcyan",		0x008B8B},
	{"darkgoldenrod",	0xB8860B},
	{"darkgray",		0xA9A9A9},
	{"darkgreen",		0x006400},
	{"darkkhaki",		0xBDB76B},
	{"darkmagenta",		0x8B008B},
	{"darkolivegreen",	0x556B2F},
	{"darkorange",		0xFF8C00},
	{"darkorchid",		0x9932CC},
	{"darkred",		0x8B0000},
	{"darksalmon",		0xE9967A},
	{"darkseagreen",	0x8FBC8F},
	{"darkslateblue",	0x483D8B},
	{"darkslategray",	0x2F4F4F},
	{"darkturquoise",	0x00CED1},
	{"darkviolet",		0x9400D3},
	{"deeppink",		0xFF1493},
	{"deepskyblue",		0x00BFFF},
	{"dimgray",		0x696969},
	{"dodgerblue",		0x1E90FF},
	{"firebrick",		0xB22222},
	{"floralwhite",		0xFFFAF0},
	{"forestgreen",		0x228B22},
	{"fuchsia",		0xFF00FF},
	{"gainsboro",		0xDCDCDC},
	{"ghostwhite",		0xF8F8FF},
	{"gold",		0xFFD700},
	{"goldenrod",		0xDAA520},
	{"gray",		0x808080},
	{"green",		0x008000},
	{"greenyellow",		0xADFF2F},
	{"honeydew",		0xF0FFF0},
	{"hotpink",		0xFF69B4},
	{"indianred",		0xCD5C5C},
	{"indigo",		0x4B0082},
	{"ivory",		0xFFFFF0},
	{"khaki",		0xF0E68C},
	{"lavender",		0xE6E6FA},
	{"lavenderblush",	0xFFF0F5},
	{"lawngreen",		0x7CFC00},
	{"lemonchiffon",	0xFFFACD},
	{"lightblue",		0xADD8E6},
	{"lightcoral",		0xF08080},
	{"lightcyan",		0xE0FFFF},
	{"lightgoldenrodyellow",	0xFAFAD2},
	{"lightgreen",		0x90EE90},
	{"lightgrey",		0xD3D3D3},
	{"lightpink",		0xFFB6C1},
	{"lightsalmon",		0xFFA07A},
	{"lightseagreen",	0x20B2AA},
	{"lightskyblue",	0x87CEFA},
	{"lightslategray",	0x778899},
	{"lightsteelblue",	0xB0C4DE},
	{"lightyellow",		0xFFFFE0},
	{"lime",		0x00FF00},
	{"limegreen",		0x32CD32},
	{"linen",		0xFAF0E6},
	{"magenta",		0xFF00FF},
	{"maroon",		0x800000},
	{"mediumaquamarine",	0x66CDAA},
	{"mediumblue",		0x0000CD},
	{"mediumorchid",	0xBA55D3},
	{"mediumpurple",	0x9370DB},
	{"mediumseagreen",	0x3CB371},
	{"mediumslateblue",	0x7B68EE},
	{"mediumspringgreen",	0x00FA9A},
	{"mediumturquoise",	0x48D1CC},
	{"mediumvioletred",	0xC71585},
	{"midnightblue",	0x191970},
	{"mintcream",		0xF5FFFA},
	{"mistyrose",		0xFFE4E1},
	{"moccasin",		0xFFE4B5},
	{"navajowhite",		0xFFDEAD},
	{"navy",		0x000080},
	{"oldlace",		0xFDF5E6},
	{"olive",		0x808000},
	{"olivedrab",		0x6B8E23},
	{"orange",		0xFFA500},
	{"orangered",		0xFF4500},
	{"orchid",		0xDA70D6},
	{"palegoldenrod",	0xEEE8AA},
	{"palegreen",		0x98FB98},
	{"paleturquoise",	0xAFEEEE},
	{"palevioletred",	0xDB7093},
	{"papayawhip",		0xFFEFD5},
	{"peachpuff",		0xFFDAB9},
	{"peru",		0xCD853F},
	{"pink",		0xFFC0CB},
	{"plum",		0xDDA0DD},
	{"powderblue",		0xB0E0E6},
	{"purple",		0x800080},
	{"red",			0xFF0000},
	{"rosybrown",		0xBC8F8F},
	{"royalblue",		0x4169E1},
	{"saddlebrown",		0x8B4513},
	{"salmon",		0xFA8072},
	{"sandybrown",		0xF4A460},
	{"seagreen",		0x2E8B57},
	{"seashell",		0xFFF5EE},
	{"sienna",		0xA0522D},
	{"silver",		0xC0C0C0},
	{"skyblue",		0x87CEEB},
	{"slateblue",		0x6A5ACD},
	{"slategray",		0x708090},
	{"snow",		0xFFFAFA},
	{"springgreen",		0x00FF7F},
	{"steelblue",		0x4682B4},
	{"tan",			0xD2B48C},
	{"teal",		0x008080},
	{"thistle",		0xD8BFD8},
	{"tomato",		0xFF6347},
	{"turquoise",		0x40E0D0},
	{"violet",		0xEE82EE},
	{"wheat",		0xF5DEB3},
	{"white",		0xFFFFFF},
	{"whitesmoke",		0xF5F5F5},
	{"yellow",		0xFFFF00},
	{"yellowgreen",		0x9ACD32},
};

#define endof(T) ((T)+sizeof(T)/sizeof(*(T)))

int decode_color(unsigned char *str, struct rgb *col)
{
	int ch;
	if (*str != '#') {
		struct color_spec *cs;
		for (cs = color_specs; cs < endof(color_specs); cs++)
			if (!strcasecmp(( char *)cs->name, ( char *)str)) {
				ch = cs->rgb;
				goto found;
			}
		str--;
	}
	str++;
	if (strlen(( char *)str) == 6) {
		char *end;
		ch = strtoul((char *)str, &end, 16);
		if (!*end) {
found:
			col->r = ch / 0x10000;
			col->g = ch / 0x100 % 0x100;
			col->b = ch % 0x100;
			return 0;
		}
	}
	return -1;
}

int get_color(unsigned char *a, unsigned char *c, struct rgb *rgb)
{
	unsigned char *at;
	int r = -1;
	if (d_opt->col >= 1 || F) if ((at = get_attr_val(a, c))) {
		r = decode_color(at, rgb);
		mem_free(at);
	}
	return r;
}

int get_bgcolor(unsigned char *a, struct rgb *rgb)
{
	if (d_opt->col < 2 && !F) return -1;
	return get_color(a, (unsigned char *)"bgcolor", rgb);
}

unsigned char *get_target(unsigned char *a)
{
	return get_attr_val(a, (unsigned char *)"target");
}

void kill_html_stack_item(struct html_element *e)
{
	html_format_changed = 1;
	if (e->dontkill == 2) {
		internal((unsigned char *)"trying to kill unkillable element");
		return;
	}
	if (!e || (void *)e == &html_stack) {
		internal((unsigned char *)"trying to free bad html element");
		return;
	}
	if (e->attr.fontface) mem_free(e->attr.fontface);
	if (e->attr.link) mem_free(e->attr.link);
	if (e->attr.target) mem_free(e->attr.target);
	if (e->attr.image) mem_free(e->attr.image);
	if (e->attr.href_base) mem_free(e->attr.href_base);
	if (e->attr.target_base) mem_free(e->attr.target_base);
	if (e->attr.select) mem_free(e->attr.select);
	free_js_event_spec(e->attr.js_event);
	del_from_list(e);
	mem_free(e);
	/*if ((void *)(html_stack.next) == &html_stack || !html_stack.next) {
		debug("killing last element");
	}*/
}

static inline void kill_elem(char *e)
{
	if (html_top.namelen == strlen(( char *)e) && !casecmp(html_top.name, (unsigned char *)e, html_top.namelen))
		kill_html_stack_item(&html_top);
}

#ifdef DEBUG
void debug_stack(void)
{
	struct html_element *e;
	printf("HTML stack debug: \n");
	foreachback(e, html_stack) {
		int i;
		printf("\"");
		for (i = 0; i < e->namelen; i++) printf("%c", e->name[i]);
		printf("\"\n");
	}
	printf("%c", 7);
	fflush(stdout);
	sleep(1);
}
#endif

void html_stack_dup(void)
{
	struct html_element *e;
	struct html_element *ep;
	html_format_changed = 1;
	if ((void *)(ep = html_stack.next) == &html_stack || !html_stack.next) {
	   	internal((unsigned char *)"html stack empty");
		return;
	}
	if (!(e = mem_alloc(sizeof(struct html_element)))) return;
	memcpy(e, ep, sizeof(struct html_element));
	copy_string(&e->attr.fontface, ep->attr.fontface);
	copy_string(&e->attr.link, ep->attr.link);
	copy_string(&e->attr.target, ep->attr.target);
	copy_string(&e->attr.image, ep->attr.image);
	copy_string(&e->attr.href_base, ep->attr.href_base);
	copy_string(&e->attr.target_base, ep->attr.target_base);
	copy_string(&e->attr.select, ep->attr.select);
	copy_js_event_spec(&e->attr.js_event, ep->attr.js_event);
	/*if (e->name) {
		if (e->attr.link) set_mem_comment(e->attr.link, e->name, e->namelen);
		if (e->attr.target) set_mem_comment(e->attr.target, e->name, e->namelen);
		if (e->attr.image) set_mem_comment(e->attr.image, e->name, e->namelen);
		if (e->attr.href_base) set_mem_comment(e->attr.href_base, e->name, e->namelen);
		if (e->attr.target_base) set_mem_comment(e->attr.target_base, e->name, e->namelen);
		if (e->attr.select) set_mem_comment(e->attr.select, e->name, e->namelen);
	}*/
	e->name = e->options = NULL;
	e->namelen = 0;
	e->dontkill = 0;
	add_to_list(html_stack, e);
}


#ifdef JS
void get_js_event(unsigned char *a, unsigned char *name, unsigned char **where)
{
	unsigned char *v;
	if ((v = get_attr_val(a, name))) {
		if (*where) mem_free(*where);
		*where = v;
	}
}

int get_js_events(unsigned char *a)
{
	if (!has_attr(a, (unsigned char *)"onkeyup") && !has_attr(a, (unsigned char *)"onkeydown") && !has_attr(a,(unsigned char *)"onkeypress") && !has_attr(a,(unsigned char *)"onchange") && !has_attr(a, (unsigned char *)"onfocus") && !has_attr(a,(unsigned char *)"onblur") && !has_attr(a, (unsigned char *)"onclick") && !has_attr(a, (unsigned char *)"ondblclick") && !has_attr(a, (unsigned char *)"onmousedown") && !has_attr(a, (unsigned char *)"onmousemove") && !has_attr(a, (unsigned char *)"onmouseout") && !has_attr(a, (unsigned char *)"onmouseover") && !has_attr(a, (unsigned char *)"onmouseup")) return 0;
	create_js_event_spec(&format.js_event);
	get_js_event(a, (unsigned char *)"onclick", &format.js_event->click_code);
	get_js_event(a, (unsigned char *)"ondblclick", &format.js_event->dbl_code);
	get_js_event(a, (unsigned char *)"onmousedown", &format.js_event->down_code);
	get_js_event(a, (unsigned char *)"onmouseup", &format.js_event->up_code);
	get_js_event(a, (unsigned char *)"onmouseover", &format.js_event->over_code);
	get_js_event(a, (unsigned char *)"onmouseout", &format.js_event->out_code);
	get_js_event(a, (unsigned char *)"onmousemove", &format.js_event->move_code);
	get_js_event(a, (unsigned char *)"onfocus", &format.js_event->focus_code);
	get_js_event(a, (unsigned char *)"onblur", &format.js_event->blur_code);
	get_js_event(a, (unsigned char *)"onchange", &format.js_event->change_code);
	get_js_event(a, (unsigned char *)"onkeypress", &format.js_event->keypress_code);
	get_js_event(a, (unsigned char *)"onkeyup", &format.js_event->keyup_code);
	get_js_event(a, (unsigned char *)"onkeydown", &format.js_event->keydown_code);
	return 1;
}
#else
int get_js_events(unsigned char *a)
{
	return 0;
}
#endif

void *ff;
void (*put_chars_f)(void *, unsigned char *, int);
void (*line_break_f)(void *);
void *(*special_f)(void *, int, ...);

unsigned char *eoff;
unsigned char *eofff;
unsigned char *startf;

int line_breax;
int pos;
int putsp;

int was_br;
int table_level;
int empty_format;

void ln_break(int n, void (*line_break)(void *), void *f)
{
	if (!n || html_top.invisible) return;
	while (n > line_breax) line_breax++, line_break(f);
	pos = 0;
	putsp = -1; /* ??? */
}

void put_chrs(unsigned char *start, int len, void (*put_chars)(void *, unsigned char *, int), void *f)
{
	if (par_format.align == AL_NO) putsp = 0;
	if (!len || html_top.invisible) return;
	if (putsp == 1) { put_chars(f, (unsigned char *)" ", 1); pos++;  putsp = -1; }
	if (putsp == -1) {
		if (start[0] == ' ') start++, len--;
		putsp = 0;
	}
	if (!len) {
		putsp = -1;
		if (par_format.align == AL_NO) putsp = 0;
		return;
	}
	if (start[len - 1] == ' ') putsp = -1;
	if (par_format.align == AL_NO) putsp = 0;
	was_br = 0;
	put_chars(f, start, len);
	pos += len;
	line_breax = 0;
}

void kill_until(int ls, ...)
{
	int l;
	struct html_element *e = &html_top;
	if (ls) e = e->next;
	while ((void *)e != &html_stack) {
		int sk = 0;
		va_list arg;
		va_start(arg, ls);
		while (1) {
			char *s = va_arg(arg, char *);
			if (!s) break;
			if (!*s) sk++;
			else if (e->namelen == strlen(( char *)s) && !casecmp((unsigned char *)e->name, (unsigned char *)s, strlen(( char *)s))) {
				if (!sk) {
					if (e->dontkill) break;
					va_end(arg);
					goto killll;
				}
				else if (sk == 1) {
					va_end(arg);
					goto killl;
				} else break;
			}
		}
		va_end(arg);
		if (e->dontkill || (e->namelen == 5 && !casecmp(e->name, (unsigned char *)"TABLE", 5))) break;
		if (e->namelen == 2 && upcase(e->name[0]) == 'T' && (upcase(e->name[1]) == 'D' || upcase(e->name[1]) == 'H' || upcase(e->name[1]) == 'R')) break;
		e = e->next;
	}
	return;
	killl:
	e = e->prev;
	killll:
	l = 0;
	while ((void *)e != &html_stack) {
		if (ls && e == html_stack.next) break;
		if (e->linebreak > l) l = e->linebreak;
		e = e->prev;
		kill_html_stack_item(e->next);
	}
	ln_break(l, line_break_f, ff);
}

int get_num(unsigned char *a, unsigned char *n)
{
	char *al;
	if ((al = ( char *)get_attr_val(a, n))) {
		char *end;
		int s = strtoul(al, &end, 10);
		if (!*al || *end || s < 0) s = -1;
		mem_free(al);
		return s;
	}
	return -1;
}

int parse_width(unsigned char *w, int trunc)
{
	unsigned char *end;
	int p = 0;
	int s;
	int l;
	int limit = par_format.width - (par_format.leftmargin + par_format.rightmargin) * gf_val(1, G_HTML_MARGIN);
	while (WHITECHAR(*w)) w++;
	for (l = 0; w[l] && w[l] != ','; l++) ;
	while (l && WHITECHAR(w[l - 1])) l--;
	if (!l) return -1;
	if (w[l - 1] == '%') l--, p = 1;
	while (l && WHITECHAR(w[l - 1])) l--;
	if (!l) return -1;
	s = strtoul((char *)w, (char **)(void *)&end, 10);
	if (end - w < l) return -1;
	if (p) {
		if (trunc) {
#ifdef G
			if (trunc == 3) {
				limit = d_opt->yw - G_SCROLL_BAR_WIDTH;
				if (limit < 0) limit = 0;
			}
#endif
			s = s * limit / 100;
		}
		else return -1;
	} else s = (s + (gf_val(HTML_CHAR_WIDTH, 1) - 1) / 2) / gf_val(HTML_CHAR_WIDTH, 1);
	if (trunc == 1 && s > limit) s = limit;
	if (s < 0) s = 0;
	return s;
}

int get_width(unsigned char *a, unsigned char *n, int trunc)
{
	int r;
	unsigned char *w;
	if (!(w = get_attr_val(a, n))) return -1;
	r = parse_width(w, trunc);
	mem_free(w);
	return r;
}

/*int form_num;
struct form form = { 0, NULL, 0 };
int g_ctrl_num;*/

struct form form = { NULL, NULL, 0, 0 };

unsigned char *last_form_tag;
unsigned char *last_form_attr;
unsigned char *last_input_tag;

void put_link_line(unsigned char *prefix, unsigned char *linkname, unsigned char *link, unsigned char *target)
{
	html_stack_dup();
	ln_break(1, line_break_f, ff);
	if (format.link) mem_free(format.link), format.link = NULL;
	if (format.target) mem_free(format.target), format.target = NULL;
	format.form = NULL;
	put_chrs(prefix, strlen(( char *)prefix), put_chars_f, ff);
	html_format_changed = 1;
	format.link = join_urls(format.href_base, link);
	format.target = stracpy(target);
	memcpy(&format.fg, &format.clink, sizeof(struct rgb));
	put_chrs(linkname, strlen(( char *)linkname), put_chars_f, ff);
	ln_break(1, line_break_f, ff);
	kill_html_stack_item(&html_top);
}

void html_span(unsigned char *a) { }

void html_bold(unsigned char *a)
{ 
	get_js_events(a); 
	format.attr |= AT_BOLD; 
}

void html_italic(unsigned char *a)
{
	get_js_events(a);
	format.attr |= AT_ITALIC;
}

void html_underline(unsigned char *a)
{
	get_js_events(a); 
	format.attr |= AT_UNDERLINE;
}

void html_fixed(unsigned char *a)
{
	get_js_events(a);
	format.attr |= AT_FIXED;
}

void html_a(unsigned char *a)
{
	char *al;

	int ev = get_js_events(a);

	if ((al = (char *)get_url_val(a, (unsigned char *)"href"))) {
		char *all = al;
		while (all[0] == ' ') all++;
		while (all[0] && all[strlen(( char *)all) - 1] == ' ') all[strlen(( char *)all) - 1] = 0;
		if (format.link) mem_free(format.link);
		format.link = join_urls((unsigned char *)format.href_base, (unsigned char *)all);
		mem_free(al);
		if ((al = (char *)get_target(a))) {
			if (format.target) mem_free(format.target);
			format.target = (unsigned char *)al;
		} else {
			if (format.target) mem_free(format.target);
			format.target = stracpy(format.target_base);
		}
		/*format.attr ^= AT_BOLD;*/
		memcpy(&format.fg, &format.clink, sizeof(struct rgb));
	} else if (!ev) kill_html_stack_item(&html_top);
	if ((al = (char *)get_attr_val(a, (unsigned char *)"name"))) {
		special_f(ff, SP_TAG, al);
		mem_free(al);
	}
}

void html_font(unsigned char *a)
{
	char *al;
	if ((al = (char *)get_attr_val(a, (unsigned char *)"size"))) {
		int p = 0;
		unsigned s;
		char *nn = al;
		char *end;
		if (*al == '+') p = 1, nn++;
		if (*al == '-') p = -1, nn++;
		s = strtoul(nn, &end, 10);
		if (*nn && !*end) {
			if (s > 7) s = 7;
			if (!p) format.fontsize = s;
			else format.fontsize += p * s;
			if (format.fontsize < 1) format.fontsize = 1;
			if (format.fontsize > 7) format.fontsize = 7;
		}
		mem_free(al);
	}
	get_color(a, (unsigned char *)"color", &format.fg);
}

void html_img(unsigned char *a)
{
	unsigned char *al;
	unsigned char *s;
	unsigned char *orig_link = NULL;
	int ismap, usemap = 0;
	/*put_chrs(" ", 1, put_chars_f, ff);*/
	get_js_events(a);
	if ((!F || !d_opt->display_images) && ((al = get_attr_val(a, (unsigned char *)"usemap")))) {
		unsigned char *u;
		usemap = 1;
		html_stack_dup();
		if (format.link) mem_free(format.link);
		if (format.form) format.form = NULL;
		u = join_urls((unsigned char *)format.href_base, (unsigned char *)al);
		if ((format.link = mem_alloc(strlen(( char *)u) + 5))) {
			strcpy((char *)format.link, "MAP@");
			strcat((char *)format.link, (char *)u);
		}
		format.attr |= AT_BOLD;
		mem_free(u);
		mem_free(al);
	}
	ismap = format.link && !has_attr(a, (unsigned char *)"usemap") && has_attr(a, (unsigned char *)"ismap");
	if (format.image) mem_free(format.image), format.image = NULL;
	if ((s = get_url_val(a, (unsigned char *)"src")) || (s = get_attr_val(a, (unsigned char *)"dynsrc")) || (s = get_attr_val(a, (unsigned char *)"data"))) {
		 format.image = join_urls(format.href_base, (unsigned char *)s);
		 orig_link = s;
	}
	if (!F || !d_opt->display_images) {
		if ((!(al = get_attr_val(a, (unsigned char *)"alt")) && !(al = get_attr_val(a, (unsigned char *)"title"))) || !*al) {
			if (al) mem_free(al);
			if (!d_opt->images && !format.link) goto ret;
			if (d_opt->image_names && s) {
				unsigned char *ss;
				al = stracpy((unsigned char *)"[");
				if (!(ss = (unsigned char *)strrchr((char *)s, '/'))) ss = s;
				else ss++;
				add_to_strn(&al, ss);
				if ((ss = (unsigned char *)strchr((char *)al, '?'))) *ss = 0;
				if ((ss = (unsigned char *)strchr((char *)al, '&'))) *ss = 0;
				add_to_strn(&al, (unsigned char *)"]");
			} else if (usemap) al = stracpy((unsigned char *)"[USEMAP]");
			else if (ismap) al = stracpy((unsigned char *)"[ISMAP]");
			else al = stracpy((unsigned char *)"[IMG]");
		}
		if (al) {
			if (ismap) {
				unsigned char *h;
				html_stack_dup();
				h = stracpy(format.link);
				add_to_strn(&h, (unsigned char *)"?0,0");
				mem_free(format.link);
				format.link = h;
			}
			html_format_changed = 1;
			put_chrs(al, strlen(( char *)al), put_chars_f, ff);
			if (ismap) kill_html_stack_item(&html_top);
		}
		mem_free(al);
#ifdef G
	} else {
		struct image_description i;
		unsigned char *al;
		unsigned char *u;
		int aa = -1;

		if ((al = get_attr_val(a,(unsigned char *) "align"))) {
			if (!strcasecmp((char *)al, (char *)"left")) aa = AL_LEFT;
			if (!strcasecmp((char *)al, (char *)"right")) aa = AL_RIGHT;
			if (!strcasecmp((char *)al, (char *)"center")) aa = AL_CENTER;
			mem_free(al);
		}

		if (aa != -1) {
			ln_break(1, line_break_f, ff);
			html_stack_dup();
			par_format.align = aa;
		}

		memset(&i,0,sizeof(i));
		if (ismap) {
			unsigned char *h;
			html_stack_dup();
			h = stracpy(format.link);
			add_to_strn(&h, (unsigned char *)"?0,0");
			mem_free(format.link);
			format.link = h;
		}
		i.url = stracpy(format.image);
		i.src = orig_link, orig_link = NULL;
		/*
		i.xsize = get_num(a, "width");
		i.ysize = get_num(a, "height");
		*/
		i.xsize = get_width(a, (unsigned char *)"width", 2);
		i.ysize = get_width(a, (unsigned char *)"height", 3);
		/*debug("%s, %s -> %d, %d", get_attr_val(a, "width"), get_attr_val(a, "height"), i.xsize, i.ysize);*/
		i.hspace = get_num(a, (unsigned char *)"hspace");
		i.vspace = get_num(a, (unsigned char *)"vspace");
		i.border = get_num(a, (unsigned char *)"border");
		i.name = get_attr_val(a, (unsigned char *)"id");
		if (!i.name) i.name = get_attr_val(a, (unsigned char *)"name");
		i.alt = get_attr_val(a, (unsigned char *)"alt");
		i.insert_flag= !(format.form);
		i.ismap = ismap;
		if ((u = get_attr_val(a, (unsigned char *)"usemap"))) {
			i.usemap = join_urls((unsigned char *)format.href_base, (unsigned char *)u);
			mem_free(u);
		}
		if (i.url) special_f(ff, SP_IMAGE, &i), mem_free(i.url);
		if (i.name) mem_free(i.name);
		if (i.alt) mem_free(i.alt);
		if (i.src) mem_free(i.src);
		line_breax = 0;
		if (ismap) kill_html_stack_item(&html_top);
		if (aa != -1) {
			ln_break(1, line_break_f, ff);
			kill_html_stack_item(&html_top);
		}
		line_breax = 0;
		was_br = 0;
#endif
	}
	ret:
	if (format.image) mem_free(format.image), format.image = NULL;
	html_format_changed = 1;
	if (usemap) kill_html_stack_item(&html_top);
	/*put_chrs(" ", 1, put_chars_f, ff);*/
	if (orig_link) mem_free(orig_link);
}

void html_obj(unsigned char *a, int obj)
{
	unsigned char *old_base = format.href_base;
	unsigned char *url;
	unsigned char *type = get_attr_val(a,(unsigned char *) "type");
	unsigned char *base;
	if ((base = get_attr_val(a, (unsigned char *)"codebase"))) format.href_base = base;
	if (!type) {
		url = get_attr_val(a, (unsigned char *)"src");
		if (!url) url = get_attr_val(a, (unsigned char *)"data");
		if (url) type = get_content_type(NULL, url), mem_free(url);
	}
	if (type && !casecmp(type, (unsigned char *)"image/", 6)) {
		html_img(a);
		if (obj == 1) html_top.invisible = 1;
		goto ret;
	}
	url = get_attr_val(a, (unsigned char *)"src");
	if (!url) url = get_attr_val(a, (unsigned char *)"data");
	if (url) put_link_line((unsigned char *)"", !obj ? (unsigned char *)"[EMBED]" : (unsigned char *)"[OBJ]", url, (unsigned char *)""), mem_free(url);
	ret:
	if (base) format.href_base = old_base, mem_free(base);
	if (type) mem_free(type);
}

void html_embed(unsigned char *a)
{
	html_obj(a, 0);
}

void html_object(unsigned char *a)
{
	html_obj(a, 1);
}

void html_body(unsigned char *a)
{
	get_color(a, (unsigned char *)"text", &format.fg);
	get_color(a, (unsigned char *)"link", &format.clink);
	get_color(a, (unsigned char *)"vlink", &format.vlink);
	/*
	get_bgcolor(a, &format.bg);
	get_bgcolor(a, &par_format.bgcolor);
	*/
}

void html_skip(unsigned char *a) { html_top.invisible = html_top.dontkill = 1; }

void html_title(unsigned char *a) { html_top.invisible = html_top.dontkill = 1; }

void html_script(unsigned char *a)
{
	unsigned char *s;
	s = get_attr_val(a, (unsigned char *)"src");
	special_f(ff, SP_SCRIPT, s);
	if (s) mem_free(s);
	html_skip(a);
}

void html_noscript(unsigned char *a)
{
	if (d_opt->js_enable) html_skip(a);
}

void html_center(unsigned char *a)
{
	par_format.align = AL_CENTER;
	if (!table_level && !F) par_format.leftmargin = par_format.rightmargin = 0;
}

void html_linebrk(unsigned char *a)
{
	char *al;
	if ((al = (char *)get_attr_val(a, (unsigned char *)"align"))) {
		if (!strcasecmp(al, ( char *)"left")) par_format.align = AL_LEFT;
		if (!strcasecmp(al, ( char *)"right")) par_format.align = AL_RIGHT;
		if (!strcasecmp(al, ( char *)"center")) {
			par_format.align = AL_CENTER;
			if (!table_level && !F) par_format.leftmargin = par_format.rightmargin = 0;
		}
		if (!strcasecmp(al, "justify")) par_format.align = AL_BLOCK;
		mem_free(al);
	}
}

void html_br(unsigned char *a)
{
	html_linebrk(a);
	if (was_br) ln_break(2, line_break_f, ff);
	was_br = 1;
}

void html_form(unsigned char *a)
{
	was_br = 1;
}

void html_p(unsigned char *a)
{
	if (par_format.leftmargin < margin) par_format.leftmargin = margin;
	if (par_format.rightmargin < margin) par_format.rightmargin = margin;
	/*par_format.align = AL_LEFT;*/
	html_linebrk(a);
}

void html_address(unsigned char *a)
{
	par_format.leftmargin += 1;
	par_format.align = AL_LEFT;
}

void html_blockquote(unsigned char *a)
{
	par_format.leftmargin += 2;
	par_format.align = AL_LEFT;
}

void html_h(int h, char *a)
{
	par_format.align = AL_LEFT;
#ifdef G
	if (F) {
		html_linebrk((unsigned char *)a);
		format.fontsize = 8 - h;
		format.attr |= AT_BOLD;
		return;
	}
#endif
	if (h == 1) {
		html_center((unsigned char *)a);
		return;
	}
	html_linebrk((unsigned char *)a);
	switch (par_format.align) {
		case AL_LEFT:
			par_format.leftmargin = (h - 2) * 2;
			par_format.rightmargin = 0;
			break;
		case AL_RIGHT:
			par_format.leftmargin = 0;
			par_format.rightmargin = (h - 2) * 2;
			break;
		case AL_CENTER:
			par_format.leftmargin = par_format.rightmargin = 0;
			break;
		case AL_BLOCK:
			par_format.leftmargin = par_format.rightmargin = (h - 2) * 2;
			break;
	}
}

void html_h1(unsigned char *a) { html_h(1, (char *)a); }
void html_h2(unsigned char *a) { html_h(2, (char *)a); }
void html_h3(unsigned char *a) { html_h(3, (char *)a); }
void html_h4(unsigned char *a) { html_h(4, (char *)a); }
void html_h5(unsigned char *a) { html_h(5, (char *)a); }
void html_h6(unsigned char *a) { html_h(6, (char *)a); }

void html_pre(unsigned char *a)
{
	format.attr |= AT_FIXED;
	par_format.align = AL_NO;
	par_format.leftmargin = par_format.leftmargin > 1;
	par_format.rightmargin = 0;
}

void html_hr(unsigned char *a)
{
	int i/* = par_format.width - 10*/;
	char r = 205;
	int q = get_num(a, (unsigned char *)"size");
	if (q >= 0 && q < 2) r = 196;
	html_stack_dup();
	par_format.align = AL_CENTER;
	if (format.link) mem_free(format.link), format.link = NULL;
	format.form = NULL;
	html_linebrk(a);
	if (par_format.align == AL_BLOCK) par_format.align = AL_CENTER;
	par_format.leftmargin = margin;
	par_format.rightmargin = margin;
	if ((i = get_width(a, (unsigned char *)"width", 1)) == -1) i = par_format.width - 2 * margin - 4;
	if (!F) format.attr = AT_GRAPHICS;
	else r = 0xa0, i = 1;
	special_f(ff, SP_NOWRAP, 1);
	while (i-- > 0) put_chrs((unsigned char *)&r, 1, put_chars_f, (unsigned char *)ff);
	special_f(ff, SP_NOWRAP, 0);
	ln_break(2, line_break_f, ff);
	kill_html_stack_item(&html_top);
}

void html_table(unsigned char *a)
{
	par_format.leftmargin = margin;
	par_format.rightmargin = margin;
	par_format.align = AL_LEFT;
	html_linebrk(a);
	format.attr = 0;
}

void html_tr(unsigned char *a)
{
	html_linebrk(a);
}

void html_th(unsigned char *a)
{
	/*html_linebrk(a);*/
	kill_until(1, "TD", "TH", "", "TR", "TABLE", NULL);
	format.attr |= AT_BOLD;
	put_chrs((unsigned char *)" ", 1, put_chars_f, ff);
}

void html_td(unsigned char *a)
{
	/*html_linebrk(a);*/
	kill_until(1, "TD", "TH", "", "TR", "TABLE", NULL);
	format.attr &= ~AT_BOLD;
	put_chrs((unsigned char *)" ", 1, put_chars_f, ff);
}

void html_base(unsigned char *a)
{
	char *al;
	if ((al = ( char *)get_url_val(a, (unsigned char *)"href"))) {
		if (format.href_base) mem_free(format.href_base);
		format.href_base = join_urls(((struct html_element *)html_stack.prev)->attr.href_base, (unsigned char *)al);
		mem_free(al);
	}
	if ((al = ( char *)get_target(a))) {
		if (format.target_base) mem_free(format.target_base);
		format.target_base = (unsigned char *)al;
	}
}

void html_ul(unsigned char *a)
{
	char *al;
	/*debug_stack();*/
	par_format.list_level++;
	par_format.list_number = 0;
	par_format.flags = P_STAR;
	if ((al = ( char *)get_attr_val(a, (unsigned char *)"type"))) {
		if (!strcasecmp(( char *)al, ( char *)"disc") || !strcasecmp(( char *)al, ( char *)"circle")) par_format.flags = P_O;
		if (!strcasecmp(( char *)al, ( char *)"square")) par_format.flags = P_PLUS;
		mem_free(al);
	}
	if ((par_format.leftmargin += 2 + (par_format.list_level > 1)) > par_format.width / 2 && !table_level)
		par_format.leftmargin = par_format.width / 2;
	par_format.align = AL_LEFT;
	html_top.dontkill = 1;
}

void html_ol(unsigned char *a)
{
	char *al;
	int st;
	par_format.list_level++;
	st = get_num(a, (unsigned char *)"start");
	if (st == -1) st = 1;
	par_format.list_number = st;
	par_format.flags = P_NUMBER;
	if ((al = ( char *)get_attr_val(a, (unsigned char *)"type"))) {
		if (!strcmp(al, "1")) par_format.flags = P_NUMBER;
		if (!strcmp(al, "a")) par_format.flags = P_alpha;
		if (!strcmp(al, "A")) par_format.flags = P_ALPHA;
		if (!strcmp(al, "r")) par_format.flags = P_roman;
		if (!strcmp(al, "R")) par_format.flags = P_ROMAN;
		if (!strcmp(al, "i")) par_format.flags = P_roman;
		if (!strcmp(al, "I")) par_format.flags = P_ROMAN;
		mem_free(al);
	}
	if (!F) if ((par_format.leftmargin += (par_format.list_level > 1)) > par_format.width / 2 && !table_level)
		par_format.leftmargin = par_format.width / 2;
	par_format.align = AL_LEFT;
	html_top.dontkill = 1;
}

void html_li(unsigned char *a)
{
	/*kill_until(0, "", "UL", "OL", NULL);*/
	if (!par_format.list_number) {
		char x[7] = "*&nbsp;";
		if ((par_format.flags & P_LISTMASK) == P_O) x[0] = 'o';
		if ((par_format.flags & P_LISTMASK) == P_PLUS) x[0] = '+';
		put_chrs((unsigned char *)x, 7, put_chars_f, ff);
		par_format.leftmargin += 2;
		par_format.align = AL_LEFT;
		putsp = -1;
	} else {
		char c = 0;
		char n[32];
		int t = par_format.flags & P_LISTMASK;
		int s = get_num(a, (unsigned char *)"value");
		if (s != -1) par_format.list_number = s;
		if ((t != P_roman && t != P_ROMAN && par_format.list_number < 10) || t == P_alpha || t == P_ALPHA) put_chrs((unsigned char *)"&nbsp;", 6, put_chars_f, ff), c = 1;
		if (t == P_ALPHA || t == P_alpha) {
			n[0] = par_format.list_number ? (par_format.list_number - 1) % 26 + (t == P_ALPHA ? 'A' : 'a') : 0;
			n[1] = 0;
		} else if (t == P_ROMAN || t == P_roman) {
			roman(n, par_format.list_number);
			if (t == P_ROMAN) {
				char *x;
				for (x = n; *x; x++) *x = upcase(*x);
			}
		} else sprintf(n, "%d", par_format.list_number);
		put_chrs((unsigned char *)n, strlen(( char *)n), put_chars_f, ff);
		put_chrs((unsigned char *)".&nbsp;", 7, put_chars_f, ff);
		par_format.leftmargin += strlen(( char *)n) + c + 2;
		par_format.align = AL_LEFT;
		par_format.list_number = 0;
		html_top.next->parattr.list_number++;
		putsp = -1;
	}
	line_breax = 2;
}

void html_dl(unsigned char *a)
{
	par_format.flags &= ~P_COMPACT;
	if (has_attr(a, (unsigned char *)"compact")) par_format.flags |= P_COMPACT;
	if (par_format.list_level) par_format.leftmargin += 5;
	par_format.list_level++;
	par_format.list_number = 0;
	par_format.align = AL_LEFT;
	par_format.dd_margin = par_format.leftmargin;
	html_top.dontkill = 1;
	if (!(par_format.flags & P_COMPACT)) {
		ln_break(2, line_break_f, ff);
		html_top.linebreak = 2;
	}
}

void html_dt(unsigned char *a)
{
	kill_until(0, "", "DL", NULL);
	par_format.align = AL_LEFT;
	par_format.leftmargin = par_format.dd_margin;
	if (!(par_format.flags & P_COMPACT) && !has_attr(a, (unsigned char *)"compact"))
		ln_break(2, line_break_f, ff);
}

void html_dd(unsigned char *a)
{
	kill_until(0, "", "DL", NULL);
	if ((par_format.leftmargin = par_format.dd_margin + (table_level ? 3 : 8)) > par_format.width / 2 && !table_level)
		par_format.leftmargin = par_format.width / 2;
	par_format.align = AL_LEFT;
}

void get_html_form(unsigned char *a, struct form *form)
{
	unsigned char *al;
	unsigned char *ch;
	form->method = FM_GET;
	if ((al = get_attr_val(a, (unsigned char *)"method"))) {
		if (!strcasecmp((char *)al, ( char *)"post")) {
			char *ax;
			form->method = FM_POST;
			if ((ax = ( char *)get_attr_val(a, (unsigned char *)"enctype"))) {
				if (!strcasecmp(ax, "multipart/form-data"))
					form->method = FM_POST_MP;
				mem_free(ax);
			}
		}
		mem_free(al);
	}
	if ((al = get_url_val(a, (unsigned char *)"action"))) {
		char *all = (char *)al;
		while (all[0] == ' ') all++;
		while (all[0] && all[strlen(( char *)all) - 1] == ' ') all[strlen(( char *)all) - 1] = 0;
		form->action = join_urls(format.href_base, (unsigned char *)all);
		mem_free(al);
	} else {
		if ((ch =(unsigned char *) strchr((char *)(form->action = stracpy(format.href_base)), POST_CHAR))) *ch = 0;
		if (form->method == FM_GET && (ch = (unsigned char *)strchr(( char *)form->action, '?'))) *ch = 0;
	}
	if ((al = get_target(a))) {
		form->target = al;
	} else {
		form->target = stracpy(format.target_base);
	}
	if ((al=get_attr_val(a,(unsigned char *)"name")))
	{
		form->form_name=al;
	}
	if ((al=get_attr_val(a,(unsigned char *)"onsubmit")))
	{
		form->onsubmit=al;
	}
	form->num = a - startf;
}

void find_form_for_input(unsigned char *i)
{
	unsigned char *s, *ss, *name, *attr, *lf, *la;
	int namelen;
	if (form.action) mem_free(form.action);
	if (form.target) mem_free(form.target);
	if (form.form_name) mem_free(form.form_name);
	if (form.onsubmit) mem_free(form.onsubmit);
	memset(&form, 0, sizeof(struct form));
	if (!special_f(ff, SP_USED, NULL)) return;
	if (last_form_tag && last_input_tag && i <= last_input_tag && i > last_form_tag) {
		get_html_form(last_form_attr, &form);
		return;
	}
	if (last_form_tag && last_input_tag && i > last_input_tag) s = last_form_tag;
	else s = startf;
	lf = NULL, la = NULL;
	se:
	while (s < i && *s != '<') sp:s++;
	if (s >= i) goto end_parse;
	if (s + 2 <= eofff && (s[1] == '!' || s[1] == '?')) {
		s = skip_comment(s, i);
		goto se;
	}
	ss = s;
	if (parse_element(s, i, &name, &namelen, &attr, &s)) goto sp;
	if (namelen != 4 || casecmp(name, (unsigned char *)"FORM", 4)) goto se;
	lf = ss;
	la = attr;
	goto se;

	end_parse:
	if (lf) {
		last_form_tag = lf;
		last_form_attr = la;
		last_input_tag = i;
		get_html_form(la, &form);
	} else last_form_tag = NULL;
}

void html_button(unsigned char *a)
{
	char *al;
	struct form_control *fc;
	find_form_for_input(a);
	if (!(fc = mem_alloc(sizeof(struct form_control)))) return;
	memset(fc, 0, sizeof(struct form_control));
	if (!(al = ( char *)get_attr_val(a, (unsigned char *)"type"))) {
		fc->type = FC_SUBMIT;
		goto xxx;
	}
	if (!strcasecmp(al, ( char *)"submit")) fc->type = FC_SUBMIT;
	else if (!strcasecmp(al, ( char *)"reset")) fc->type = FC_RESET;
	else if (!strcasecmp(al, ( char *)"button")) fc->type = FC_BUTTON;
	else {
		mem_free(al);
		mem_free(fc);
		return;
	}
	mem_free(al);
	xxx:
	get_js_events(a);
	fc->form_num = last_form_tag ? last_form_tag - startf : 0;
	fc->ctrl_num = last_form_tag ? a - last_form_tag : a - startf;
	fc->position = a - startf;
	fc->method = form.method;
	fc->action = stracpy(form.action);
	fc->form_name = stracpy(form.form_name);
	fc->onsubmit = stracpy(form.onsubmit);
	fc->name = get_attr_val(a, (unsigned char *)"name");
	fc->default_value = get_attr_val(a, (unsigned char *)"value");
	fc->ro = has_attr(a, (unsigned char *)"disabled") ? 2 : has_attr(a, (unsigned char *)"readonly") ? 1 : 0;
	if (fc->type == FC_IMAGE) fc->alt = get_attr_val(a, (unsigned char *)"alt");
	if (fc->type == FC_SUBMIT && !fc->default_value) fc->default_value = stracpy((unsigned char *)"Submit");
	if (fc->type == FC_RESET && !fc->default_value) fc->default_value = stracpy((unsigned char *)"Reset");
	if (fc->type == FC_BUTTON && !fc->default_value) fc->default_value = stracpy((unsigned char *)"BUTTON");
	if (!fc->default_value) fc->default_value = stracpy((unsigned char *)"");
	special_f(ff, SP_CONTROL, fc);
	format.form = fc;
	format.attr |= AT_BOLD | AT_FIXED;
	/*put_chrs("[&nbsp;", 7, put_chars_f, ff);
	if (fc->default_value) put_chrs(fc->default_value, strlen(( char *)fc->default_value), put_chars_f, ff);
	put_chrs("&nbsp;]", 7, put_chars_f, ff);
	put_chrs(" ", 1, put_chars_f, ff);*/
}

void html_input(unsigned char *a)
{
	int i;
	unsigned char *al;
	struct form_control *fc;
	find_form_for_input(a);
	if (!(fc = mem_alloc(sizeof(struct form_control)))) return;
	memset(fc, 0, sizeof(struct form_control));
	if (!(al = get_attr_val(a, (unsigned char *)"type"))) {
		fc->type = FC_TEXT;
		goto xxx;
	}
	if (!strcasecmp(( char *)al, "text")) fc->type = FC_TEXT;
	else if (!strcasecmp(( char *)al, "password")) fc->type = FC_PASSWORD;
	else if (!strcasecmp(( char *)al, "checkbox")) fc->type = FC_CHECKBOX;
	else if (!strcasecmp(( char *)al, "radio")) fc->type = FC_RADIO;
	else if (!strcasecmp(( char *)al, "submit")) fc->type = FC_SUBMIT;
	else if (!strcasecmp(( char *)al, "reset")) fc->type = FC_RESET;
	else if (!strcasecmp(( char *)al, "file")) fc->type = FC_FILE;
	else if (!strcasecmp(( char *)al, "hidden")) fc->type = FC_HIDDEN;
	else if (!strcasecmp(( char *)al, "image")) fc->type = FC_IMAGE;
	else if (!strcasecmp(( char *)al, "button")) fc->type = FC_BUTTON;
	else fc->type = FC_TEXT;
	mem_free(al);
	xxx:
	fc->form_num = last_form_tag ? last_form_tag - startf : 0;
	fc->ctrl_num = last_form_tag ? a - last_form_tag : a - startf;
	fc->position = a - startf;
	fc->method = form.method;
	fc->action = stracpy(form.action);
	fc->form_name = stracpy(form.form_name);
	fc->onsubmit = stracpy(form.onsubmit);
	fc->target = stracpy(form.target);
	fc->name = get_attr_val(a, (unsigned char *)"name");
	if (fc->type != FC_FILE) fc->default_value = get_attr_val(a, (unsigned char *)"value");
	if (fc->type == FC_CHECKBOX && !fc->default_value) fc->default_value = stracpy((unsigned char *)"on");
	if ((fc->size = get_num(a, (unsigned char *)"size")) == -1) fc->size = HTML_DEFAULT_INPUT_SIZE;
	fc->size++;
	if (fc->size > d_opt->xw) fc->size = d_opt->xw;
	if ((fc->maxlength = get_num(a, (unsigned char *)"maxlength")) == -1) fc->maxlength = MAXINT;
	if (fc->type == FC_CHECKBOX || fc->type == FC_RADIO) fc->default_state = has_attr(a, (unsigned char *)"checked");
	fc->ro = has_attr(a, (unsigned char *)"disabled") ? 2 : has_attr(a, (unsigned char *)"readonly") ? 1 : 0;
	if (fc->type == FC_IMAGE) fc->alt = get_attr_val(a, (unsigned char *)"alt");
	if (fc->type == FC_SUBMIT && !fc->default_value) fc->default_value = stracpy((unsigned char *)"Submit");
	if (fc->type == FC_RESET && !fc->default_value) fc->default_value = stracpy((unsigned char *)"Reset");
	if (!fc->default_value) fc->default_value = stracpy((unsigned char *)"");
	if (fc->type == FC_HIDDEN) goto hid;
	put_chrs((unsigned char *)" ", 1, put_chars_f, ff);
	html_stack_dup();
	format.form = fc;
	get_js_events(a);
	switch (fc->type) {
		case FC_TEXT:
		case FC_PASSWORD:
		case FC_FILE:
			skip_nonprintable(fc->default_value);
			format.attr |= AT_BOLD | AT_FIXED;
			format.fontsize = 3;
			for (i = 0; i < fc->size; i++) put_chrs((unsigned char *)"_", 1, put_chars_f, ff);
			break;
		case FC_CHECKBOX:
			format.attr |= AT_BOLD | AT_FIXED;
			format.fontsize = 3;
			put_chrs((unsigned char *)"[&nbsp;]", 8, put_chars_f, ff);
			break;
		case FC_RADIO:
			format.attr |= AT_BOLD | AT_FIXED;
			format.fontsize = 3;
			put_chrs((unsigned char *)"[&nbsp;]", 8, put_chars_f, ff);
			break;
		case FC_IMAGE:
			if (!F || !d_opt->display_images) {
				if (format.image) mem_free(format.image), format.image = NULL;
				if ((al = get_url_val(a, (unsigned char *)"src")) || (al = get_url_val(a,(unsigned char *) "dynsrc"))) {
					format.image = join_urls(format.href_base, al);
					mem_free(al);
				}
				format.attr |= AT_BOLD | AT_FIXED;
				put_chrs((unsigned char *)"[&nbsp;", 7, put_chars_f, ff);
				if (fc->alt) put_chrs(fc->alt, strlen(( char *)fc->alt), put_chars_f, ff);
				else put_chrs((unsigned char *)"Submit", 6, put_chars_f, ff);
				put_chrs((unsigned char *)"&nbsp;]", 7, put_chars_f, ff);
			} else html_img(a);
			break;
		case FC_SUBMIT:
		case FC_RESET:
			format.attr |= AT_BOLD | AT_FIXED;
			format.fontsize = 3;
			put_chrs((unsigned char *)"[&nbsp;", 7, put_chars_f, ff);
			if (fc->default_value) put_chrs(fc->default_value, strlen(( char *)fc->default_value), put_chars_f, ff);
			put_chrs((unsigned char *)"&nbsp;]", 7, put_chars_f, ff);
			break;
		case FC_BUTTON:
			format.attr |= AT_BOLD | AT_FIXED;
			format.fontsize = 3;
			put_chrs((unsigned char *)"[&nbsp;", 7, put_chars_f, ff);
			if (fc->default_value) put_chrs(fc->default_value, strlen(( char *)fc->default_value), put_chars_f, ff);
			else put_chrs((unsigned char *)"BUTTON", 6, put_chars_f, ff);
			put_chrs((unsigned char *)"&nbsp;]", 7, put_chars_f, ff);
			break;
		default:
			internal((unsigned char *)"bad control type");
	}
	kill_html_stack_item(&html_top);
	put_chrs((unsigned char *)" ", 1, put_chars_f, ff);
			
	hid:
	special_f(ff, SP_CONTROL, fc);
}

void html_select(unsigned char *a)
{
	char *al;
	if (!(al = ( char *)get_attr_val(a, (unsigned char *)"name"))) return;
	html_top.dontkill = 1;
	format.select = (unsigned char *)al;
	format.select_disabled = 2 * has_attr(a, (unsigned char *)"disabled");
}

void html_option(unsigned char *a)
{
	struct form_control *fc;
	unsigned char *val;
	find_form_for_input(a);
	if (!format.select) return;
	if (!(fc = mem_alloc(sizeof(struct form_control)))) return;
	memset(fc, 0, sizeof(struct form_control));
	if (!(val = get_attr_val(a, (unsigned char *)"value"))) {
		unsigned char *p, *r;
		unsigned char *name;
		int namelen;
		int l = 0;
		for (p = a - 1; *p != '<'; p--) ;
		if (!(val = init_str())) goto x;
		if (parse_element(p, eoff, NULL, NULL, NULL, &p)) {
			internal((unsigned char *)"parse element failed");
			goto x;
		}
		rrrr:
		while (p < eoff && WHITECHAR(*p)) p++;
		while (p < eoff && !WHITECHAR(*p) && *p != '<') {
			pppp:
			add_chr_to_str(&val, &l, *p), p++;
		}
		r = p;
		while (r < eoff && WHITECHAR(*r)) r++;
		if (r >= eoff) goto x;
		if (r - 2 <= eoff && (r[1] == '!' || r[1] == '?')) {
			p = skip_comment(r, eoff);
			goto rrrr;
		}
		if (parse_element(r, eoff, &name, &namelen, NULL, &p)) goto pppp;
		if (!((namelen == 6 && !casecmp(name, (unsigned char *)"OPTION", 6)) ||
		    (namelen == 7 && !casecmp(name, (unsigned char *)"/OPTION", 7)) ||
		    (namelen == 6 && !casecmp(name, (unsigned char *)"SELECT", 6)) ||
		    (namelen == 7 && !casecmp(name, (unsigned char *)"/SELECT", 7)) ||
		    (namelen == 8 && !casecmp(name, (unsigned char *)"OPTGROUP", 8)) ||
		    (namelen == 9 && !casecmp(name, (unsigned char *)"/OPTGROUP", 9)))) goto rrrr;
	}
	x:
	fc->form_num = last_form_tag ? last_form_tag - startf : 0;
	fc->ctrl_num = last_form_tag ? a - last_form_tag : a - startf;
	fc->position = a - startf;
	fc->method = form.method;
	fc->action = stracpy(form.action);
	fc->form_name = stracpy(form.form_name);
	fc->onsubmit = stracpy(form.onsubmit);
	fc->type = FC_CHECKBOX;
	fc->name = stracpy(format.select);
	fc->default_value = val;
	fc->default_state = has_attr(a, (unsigned char *)"selected");
	fc->ro = format.select_disabled;
	if (has_attr(a, (unsigned char *)"disabled")) fc->ro = 2;
	put_chrs((unsigned char *)" ", 1, put_chars_f, ff);
	html_stack_dup();
	format.form = fc;
	format.attr |= AT_BOLD | AT_FIXED;
	format.fontsize = 3;
	put_chrs((unsigned char *)"[ ]", 3, put_chars_f, ff);
	kill_html_stack_item(&html_top);
	put_chrs((unsigned char *)" ", 1, put_chars_f, ff);
	special_f(ff, SP_CONTROL, fc);
}

void clr_white(unsigned char *name)
{
	unsigned char *nm;
	for (nm = name; *nm; nm++)
		if (WHITECHAR(*nm) || *nm == 1) *nm = ' ';
}

void clr_spaces(unsigned char *name)
{
	unsigned char *nm;
	clr_white(name);
	for (nm = name; *nm; nm++)
		while (nm[0] == ' ' && (nm == name || nm[1] == ' ' || !nm[1]))
			memmove(nm, nm + 1, strlen(( char *)nm));
}

int menu_stack_size;
struct menu_item **menu_stack;

void new_menu_item(unsigned char *name, int data, int fullname)
	/* name == NULL - up;	data == -1 - down */
{
	struct menu_item *top, *item, *nmenu = NULL; /* no uninitialized warnings */
	if (name) {
		clr_spaces(name);
		if (!name[0]) mem_free(name), name = stracpy((unsigned char *)" ");
		if (name[0] == 1) name[0] = ' ';
	}
	if (name && data == -1) {
		if (!(nmenu = mem_alloc(sizeof(struct menu_item)))) {
			mem_free(name);
			return;
		}
		memset(nmenu, 0, sizeof(struct menu_item));
		/*nmenu->text = "";*/
	}
	if (menu_stack_size && name) {
		top = item = menu_stack[menu_stack_size - 1];
		while (item->text) item++;
		if (!(top = mem_realloc(top, (char *)(item + 2) - (char *)top))) {
			if (data == -1) mem_free(nmenu);
			mem_free(name);
			return;
		}
		item = item - menu_stack[menu_stack_size - 1] + top;
		menu_stack[menu_stack_size - 1] = top;
		if (menu_stack_size >= 2) {
			struct menu_item *below = menu_stack[menu_stack_size - 2];
			while (below->text) below++;
			below[-1].data = top;
		}
		item->text = name;
		item->rtext = (unsigned char *)(data == -1 ? ">" : "");
		item->hotkey = (unsigned char *)( fullname ? "\000\001" : "\000\000"); /* dirty */
		item->func = data == -1 ? MENU_FUNC do_select_submenu : MENU_FUNC selected_item;
		item->data = data == -1 ? nmenu : (void *)data;
		item->in_m = data == -1 ? 1 : 0;
		item->free_i = 0;
		item++;
		memset(item, 0, sizeof(struct menu_item));
		/*item->text = "";*/
	} else if (name) mem_free(name);
	if (name && data == -1) {
		struct menu_item **ms;
		if (!(ms = mem_realloc(menu_stack, (menu_stack_size + 1) * sizeof(struct menu_item *)))) return;
		menu_stack = ms;
		menu_stack[menu_stack_size++] = nmenu;
	}
	if (!name) menu_stack_size--;
}

void init_menu(void)
{
	menu_stack_size = 0;
	menu_stack = DUMMY;
	new_menu_item(stracpy((unsigned char *)""), -1, 0);
}

void free_menu(struct menu_item *m) /* Grrr. Recursion */
{
	struct menu_item *mm;
	for (mm = m; mm->text; mm++) {
		mem_free(mm->text);
		if (mm->func == MENU_FUNC do_select_submenu) free_menu(mm->data);
	}
	mem_free(m);
}

struct menu_item *detach_menu(void)
{
	struct menu_item *i = NULL;
	if (menu_stack_size) i = menu_stack[0];
	if (menu_stack) mem_free(menu_stack);
	return i;
}

void destroy_menu(void)
{
	if (menu_stack && menu_stack != DUMMY) free_menu(menu_stack[0]);
	detach_menu();
}

void menu_labels(struct menu_item *m, unsigned char *base, unsigned char **lbls)
{
	unsigned char *bs;
	for (; m->text; m++) {
		if (m->func == MENU_FUNC do_select_submenu) {
			if ((bs = stracpy(base))) {
				add_to_strn(&bs, m->text);
				add_to_strn(&bs, (unsigned char *)" ");
				menu_labels(m->data, bs, lbls);
				mem_free(bs);
			}
		} else {
			if ((bs = stracpy(m->hotkey[1] ? (unsigned char *)"" : base))) add_to_strn(&bs, m->text);
			lbls[(int)m->data] = bs;
		}
	}
}

int menu_contains(struct menu_item *m, int f)
{
	if (m->func != MENU_FUNC do_select_submenu) return (int)m->data == f;
	for (m = m->data; m->text; m++) if (menu_contains(m, f)) return 1;
	return 0;
}

void do_select_submenu(struct terminal *term, struct menu_item *menu, struct session *ses)
{
	struct menu_item *m;
	int def = get_current_state(ses);
	int sel = 0;
	if (def < 0) def = 0;
	for (m = menu; m->text; m++, sel++) if (menu_contains(m, def)) goto f;
	sel = 0;
	f:
	do_menu_selected(term, menu, ses, sel);
}

int do_html_select(unsigned char *attr, unsigned char *html, unsigned char *eof, unsigned char **end, void *f)
{
	struct form_control *fc;
	unsigned char *t_name, *t_attr, *en;
	int t_namelen;
	unsigned char *lbl;
	int lbl_l;
	int nnmi = 0;
	struct conv_table *ct = special_f(f, SP_TABLE, NULL);
	unsigned char **val, **lbls;
	int order, preselect, group;
	int i, mw;
	if (has_attr(attr, (unsigned char *)"multiple")) return 1;
	find_form_for_input(attr);
	lbl = NULL;
	val = DUMMY;
	order = 0, group = 0, preselect = -1;
	init_menu();
        se:
        en = html;
        see:
        html = en;
	while (html < eof && *html != '<') html++;
	if (html >= eof) {
		int i;
		abort:
		*end = html;
		if (lbl) mem_free(lbl);
		for (i = 0; i < order; i++) if (val[i]) mem_free(val[i]);
		mem_free(val);
		destroy_menu();
		*end = en;
		return 0;
	}
	if (lbl) {
		unsigned char *q, *s = en;
		int l = html - en;
		while (l && WHITECHAR(s[0])) s++, l--;
		while (l && WHITECHAR(s[l-1])) l--;
		q = convert_string(ct, s, l, d_opt);
		if (q) add_to_str(&lbl, &lbl_l, q), mem_free(q);
	}
	if (html + 2 <= eof && (html[1] == '!' || html[1] == '?')) {
		html = skip_comment(html, eof);
		goto se;
	}
	if (parse_element(html, eof, &t_name, &t_namelen, &t_attr, &en)) {
		html++;
		goto se;
	}
	if (t_namelen == 7 && !casecmp(t_name, (unsigned char *)"/SELECT", 7)) {
		if (lbl) {
			if (!val[order - 1]) val[order - 1] = stracpy(lbl);
			if (!nnmi) new_menu_item(lbl, order - 1, 1), lbl = NULL;
			else mem_free(lbl), lbl = NULL;
		}
		goto end_parse;
	}
	if (t_namelen == 7 && !casecmp(t_name, (unsigned char *)"/OPTION", 7)) {
		if (lbl) {
			if (!val[order - 1]) val[order - 1] = stracpy(lbl);
			if (!nnmi) new_menu_item(lbl, order - 1, 1), lbl = NULL;
			else mem_free(lbl), lbl = NULL;
		}
		goto see;
	}
	if (t_namelen == 6 && !casecmp(t_name, (unsigned char *)"OPTION", 6)) {
		unsigned char *v, *vx;
		if (lbl) {
			if (!val[order - 1]) val[order - 1] = stracpy(lbl);
			if (!nnmi) new_menu_item(lbl, order - 1, 1), lbl = NULL;
			else mem_free(lbl), lbl = NULL;
		}
		if (has_attr(t_attr, (unsigned char *)"disabled")) goto see;
		if (preselect == -1 && has_attr(t_attr, (unsigned char *)"selected")) preselect = order;
		v = get_attr_val(t_attr, (unsigned char *)"value");
		if (!(order & (ALLOC_GR - 1))) {
			unsigned char **vv;
			if (!(vv = mem_realloc(val, (order + ALLOC_GR) * sizeof(unsigned char *)))) goto abort;
			val = vv;
		}
		val[order++] = v;
		if ((vx = get_attr_val(t_attr,(unsigned char *) "label"))) new_menu_item(vx, order - 1, 0);
		if (!v || !vx) {
			lbl = init_str(), lbl_l = 0;
			nnmi = !!vx;
		}
		goto see;
	}
	if ((t_namelen == 8 && !casecmp(t_name, (unsigned char *)"OPTGROUP", 8)) || (t_namelen == 9 && !casecmp(t_name, (unsigned char *)"/OPTGROUP", 9))) {
		if (lbl) {
			if (!val[order - 1]) val[order - 1] = stracpy(lbl);
			if (!nnmi) new_menu_item(lbl, order - 1, 1), lbl = NULL;
			else mem_free(lbl), lbl = NULL;
		}
		if (group) new_menu_item(NULL, -1, 0), group = 0;
	}
	if (t_namelen == 8 && !casecmp(t_name, (unsigned char *)"OPTGROUP", 8)) {
		unsigned char *la;
		if (!(la = get_attr_val(t_attr, (unsigned char *)"label"))) if (!(la = stracpy((unsigned char *)""))) goto see;
		new_menu_item(la, -1, 0);
		group = 1;
	}
	goto see;

	end_parse:
	*end = en;
	if (!order) goto abort;
	if (!(fc = mem_alloc(sizeof(struct form_control)))) goto abort;
	memset(fc, 0, sizeof(struct form_control));
	if (!(lbls = mem_alloc(order * sizeof(char *)))) {
		mem_free(fc);
		goto abort;
	}
	memset(lbls, 0, order * sizeof(char *));
	fc->form_num = last_form_tag ? last_form_tag - startf : 0;
	fc->ctrl_num = last_form_tag ? attr - last_form_tag : attr - startf;
	fc->position = attr - startf;
	fc->method = form.method;
	fc->action = stracpy(form.action);
	fc->form_name= stracpy(form.form_name);
	fc->onsubmit= stracpy(form.onsubmit);
	fc->name = get_attr_val(attr, (unsigned char *)"name");
	fc->type = FC_SELECT;
	fc->default_state = preselect < 0 ? 0 : preselect;
	fc->default_value = order ? stracpy(val[fc->default_state]) : stracpy((unsigned char *)"");
	fc->ro = has_attr(attr, (unsigned char *)"disabled") ? 2 : has_attr(attr, (unsigned char *)"readonly") ? 1 : 0;
	fc->nvalues = order;
	fc->values = val;
	fc->menu = detach_menu();
	fc->labels = lbls;
	menu_labels(fc->menu, (unsigned char *)"", lbls);
	html_stack_dup();
	format.attr |= AT_FIXED;
	format.fontsize = 3;
	put_chrs((unsigned char *)"[", 1, put_chars_f, f);
	html_stack_dup();
	get_js_events(attr);
	format.form = fc;
	format.attr |= AT_BOLD | AT_FIXED;
	format.fontsize = 3;
	mw = 0;
	for (i = 0; i < order; i++) if (lbls[i] && strlen(( char *)lbls[i]) > mw) mw = strlen(( char *)lbls[i]);
	for (i = 0; i < mw; i++) put_chrs((unsigned char *)"_", 1, put_chars_f, f);
	kill_html_stack_item(&html_top);
	put_chrs((unsigned char *)"]", 1, put_chars_f, f);
	kill_html_stack_item(&html_top);
	special_f(ff, SP_CONTROL, fc);
	return 0;
}

void html_textarea(unsigned char *a)
{
	internal((unsigned char *)"This should be never called");
}

void do_html_textarea(unsigned char *attr, unsigned char *html, unsigned char *eof, unsigned char **end, void *f)
{
	struct form_control *fc;
	unsigned char *p, *t_name;
	int t_namelen;
	int cols, rows;
	int i;
	find_form_for_input(attr);
	while (html < eof && (*html == '\n' || *html == '\r')) html++;
	p = html;
	while (p < eof && *p != '<') {
		pp:
		p++;
	}
	if (p >= eof) {
		*end = eof;
		return;
	}
	if (parse_element(p, eof, &t_name, &t_namelen, NULL, end)) goto pp;
	if (t_namelen != 9 || casecmp(t_name, (unsigned char *)"/TEXTAREA", 9)) goto pp;
	if (!(fc = mem_alloc(sizeof(struct form_control)))) return;
	memset(fc, 0, sizeof(struct form_control));
	fc->form_num = last_form_tag ? last_form_tag - startf : 0;
	fc->ctrl_num = last_form_tag ? attr - last_form_tag : attr - startf;
	fc->position = attr - startf;
	fc->method = form.method;
	fc->action = stracpy(form.action);
	fc->form_name = stracpy(form.form_name);
	fc->onsubmit = stracpy(form.onsubmit);
	fc->name = get_attr_val(attr, (unsigned char *)"name");
	fc->type = FC_TEXTAREA;;
	fc->ro = has_attr(attr, (unsigned char *)"disabled") ? 2 : has_attr(attr, (unsigned char *)"readonly") ? 1 : 0;
	fc->default_value = memacpy(html, p - html);
	skip_nonprintable(fc->default_value);
	for (p = fc->default_value; p && p[0]; p++) if (p[0] == '\r') {
		if (p[1] == '\n' || (p > fc->default_value && p[-1] == '\n')) memcpy(p, p + 1, strlen(( char *)p)), p--;
		else p[0] = '\n';
	}
	if ((cols = get_num(attr, (unsigned char *)"cols")) <= 0) cols = HTML_DEFAULT_INPUT_SIZE;
	cols++;
	if ((rows = get_num(attr, (unsigned char *)"rows")) <= 0) rows = 1;
	if (cols > d_opt->xw) cols = d_opt->xw;
	if (rows > d_opt->yw) rows = d_opt->yw;
	fc->cols = cols;
	fc->rows = rows;
	fc->wrap = has_attr(attr, (unsigned char *)"wrap");
	if ((fc->maxlength = get_num(attr, (unsigned char *)"maxlength")) == -1) fc->maxlength = MAXINT;
	if (rows > 1) ln_break(1, line_break_f, f);
	else put_chrs((unsigned char *)" ", 1, put_chars_f, f);
	html_stack_dup();
	get_js_events(attr);
	format.form = fc;
	format.attr |= AT_BOLD | AT_FIXED;
	format.fontsize = 3;
	for (i = 0; i < rows; i++) {
		int j;
		for (j = 0; j < cols; j++) put_chrs((unsigned char *)"_", 1, put_chars_f, f);
		if (i < rows - 1) ln_break(1, line_break_f, f);
	}
	kill_html_stack_item(&html_top);
	if (rows > 1) ln_break(1, line_break_f, f);
	else put_chrs((unsigned char *)" ", 1, put_chars_f, f);
	special_f(f, SP_CONTROL, fc);
}

void html_iframe(unsigned char *a)
{
	unsigned char *name, *url;
	if (!(url = get_url_val(a, (unsigned char *)"src"))) return;
	if (!(name = get_attr_val(a, (unsigned char *)"name"))) name = stracpy((unsigned char *)"");
	if (*name) put_link_line((unsigned char *)"IFrame: ", name, url, d_opt->framename);
	else put_link_line((unsigned char *)"", (unsigned char *)"IFrame", url, d_opt->framename);
	mem_free(name);
	mem_free(url);
}

void html_noframes(unsigned char *a)
{
	if (d_opt->frames) html_skip(a);
}

void html_frame(unsigned char *a)
{
	unsigned char *name, *u2, *url;
	if (!(u2 = get_url_val(a, (unsigned char *)"src"))) {
		url = stracpy((unsigned char *)"");
	} else {
		url = join_urls(format.href_base, u2);
		mem_free(u2);
	}
	if (!url) return;
	name = get_attr_val (a,(unsigned char *) "name");
	if (!name)
		name = stracpy(url);
	else if (!name[0]) { /* When name doesn't have a value */
		mem_free(name);
		name = stracpy(url);
	}
	if (!d_opt->frames || !html_top.frameset) put_link_line((unsigned char *)"Frame: ", name, url, (unsigned char *)"");
	else {
		struct frame_param fp;
		fp.name = name;
		fp.url = url;
		fp.parent = html_top.frameset;
		fp.marginwidth = get_num(a, (unsigned char *)"marginwidth");
		fp.marginheight = get_num(a, (unsigned char *)"marginheight");
		if (special_f(ff, SP_USED, NULL)) special_f(ff, SP_FRAME, &fp);
	}
	mem_free(name);
	mem_free(url);
}

void parse_frame_widths(unsigned char *a, int ww, int www, int **op, int *olp)
{
	unsigned char *aa;
	int q, qq, i, d, nn;
	unsigned long n;
	int *oo, *o;
	int ol;
	ol = 0;
	o = DUMMY;
	new_ch:
	while (WHITECHAR(*a)) a++;
	n = strtoul((char *)a, (char **)(void *)&a, 10);
	q = n;
	if (*a == '%') q = q * ww / 100;
	else if (*a != '*') q = (q + (www - 1) / 2) / www;
	else if (!(q = -q)) q = -1;
	if ((oo = mem_realloc(o, (ol + 1) * sizeof(int)))) (o = oo)[ol++] = q;
	if ((aa =  (unsigned char *)strchr((char *)a, ','))) {
		a = aa + 1;
		goto new_ch;
	}
	*op = o;
	*olp = ol;
	q = gf_val(2 * ol - 1, ol);
	for (i = 0; i < ol; i++) if (o[i] > 0) q += o[i] - 1;
	if (q >= ww) {
		distribute:
		for (i = 0; i < ol; i++) if (o[i] < 1) o[i] = 1;
		q -= ww;
		d = 0;
		for (i = 0; i < ol; i++) d += o[i];
		qq = q;
		for (i = 0; i < ol; i++) {
			q -= o[i] - o[i] * (d - qq) / d;
			do_not_optimize_here(&d);
				/* SIGH! gcc 2.7.2.* has an optimizer bug! */
			o[i] = o[i] * (d - qq) / d;
		}
		while (q) {
			nn = 0;
			for (i = 0; i < ol; i++) {
				if (q < 0) o[i]++, q++, nn = 1;
				if (q > 0 && o[i] > 1) o[i]--, q--, nn = 1;
				if (!q) break;
			}
			if (!nn) break;
		}
	} else {
		int nn = 0;
		for (i = 0; i < ol; i++) if (o[i] < 0) nn = 1;
		if (!nn) goto distribute;
		if (!(oo = mem_alloc(ol * sizeof(int)))) {
			*olp = 0;
			return;
		}
		memcpy(oo, o, ol * sizeof(int));
		for (i = 0; i < ol; i++) if (o[i] < 1) o[i] = 1;
		q = ww - q;
		d = 0;
		for (i = 0; i < ol; i++) if (oo[i] < 0) d += -oo[i];
		qq = q;
		for (i = 0; i < ol; i++) if (oo[i] < 0) {
			o[i] += (-oo[i] * qq / d);
			q -= (-oo[i] * qq / d);
		}
		if (q < 0) internal((unsigned char *)"parse_frame_widths: q < 0");
		for (i = 0; i < ol; i++) if (oo[i] < 0) {
			if (q) o[i]++, q--;
		}
		if (q > 0) internal((unsigned char *)"parse_frame_widths: q > 0");
		mem_free(oo);
	}
	for (i = 0; i < ol; i++) if (!o[i]) {
		int j;
		int m = 0;
		int mj = 0;
		for (j = 0; j < ol; j++) if (o[j] > m) m = o[j], mj = j;
		if (m) o[i] = 1, o[mj]--;
	}
}

void html_frameset(unsigned char *a)
{
	int x, y;
	struct frameset_param fp;
	unsigned char *c, *d;
	if (!d_opt->frames || !special_f(ff, SP_USED, NULL)) return;
	if (!(c = get_attr_val(a, (unsigned char *)"cols"))) if (!(c = stracpy((unsigned char *)"100%"))) return;
	if (!(d = get_attr_val(a, (unsigned char *)"rows"))) if (!(d = stracpy((unsigned char *)"100%"))) {
		mem_free(c);
		return;
	}
	if (!html_top.frameset) {
		x = d_opt->xw;
		y = d_opt->yw;
	} else {
		struct frameset_desc *f = html_top.frameset;
		if (f->yp >= f->y) goto free_cd;
		x = f->f[f->xp + f->yp * f->x].xw;
		y = f->f[f->xp + f->yp * f->x].yw;
	}
	parse_frame_widths(c, x, gf_val(HTML_FRAME_CHAR_WIDTH, 1), &fp.xw, &fp.x);
	parse_frame_widths(d, y, gf_val(HTML_FRAME_CHAR_HEIGHT, 1), &fp.yw, &fp.y);
	fp.parent = html_top.frameset;
	if (fp.x && fp.y) 
	{
		html_top.frameset = special_f(ff, SP_FRAMESET, &fp);
#ifdef JS
		if (html_top.frameset)html_top.frameset->onload_code=get_attr_val(a,(unsigned char *)"onload");
#endif
	}
	mem_free(fp.xw);
	mem_free(fp.yw);
	free_cd:
	mem_free(c);
	mem_free(d);
}

/*void html_frameset(unsigned char *a)
{
	int w;
	int horiz = 0;
	struct frameset_param *fp;
	unsigned char *c, *d;
	if (!d_opt->frames || !special_f(ff, SP_USED, NULL)) return;
	if (!(c = get_attr_val(a, (unsigned char *)"cols"))) {
		horiz = 1;
		if (!(c = get_attr_val(a, (unsigned char *)"rows"))) return;
	}
	if (!(fp = mem_alloc(sizeof(struct frameset_param)))) goto f;
	fp->n = 0;
	fp->horiz = horiz;
	par_format.leftmargin = par_format.rightmargin = 0;
	d = c;
	while (1) {
		while (WHITECHAR(*d)) d++;
		if (!*d) break;
		if (*d == ',') {
			d++;
			continue;
		}
		if ((w = parse_width(d, 1)) != -1) {
			struct frameset_param *fpp;
			if ((fpp = mem_realloc(fp, sizeof(struct frameset_param) + (fp->n + 1) * sizeof(int)))) {
				fp = fpp;
				fp->width[fp->n++] = w;
			}
		}
		if (!(d = strchr(d, ','))) break;
		d++;
	}
	fp->parent = html_top.frameset;
	if (fp->n) html_top.frameset = special_f(ff, SP_FRAMESET, fp);
	mem_free(fp);
	f:
	mem_free(c);
}*/

void html_link(unsigned char *a)
{
	unsigned char *name, *url;
	if ((name = get_attr_val(a, (unsigned char *)"type"))) {
		if (casecmp(a, (unsigned char *)"text/css", 8)) {
			mem_free(name);
			return;
		}
		mem_free(name);
	}
	if (!(url = get_url_val(a,(unsigned char *) "href"))) return;
	if (!(name = get_attr_val(a, (unsigned char *)"rel"))) if (!(name = get_attr_val(a, (unsigned char *)"rev"))) name = stracpy(url);
	if (strcasecmp(( char *)name, ( char *)"STYLESHEET") && strcasecmp(( char *)name, ( char *)"made") && strcasecmp(( char *)name, ( char *)"SHORTCUT ICON")) put_link_line((unsigned char *)"Link: ", name, url, format.target_base);
	mem_free(name);
	mem_free(url);
}

struct element_info {
	char *name;
	void (*func)(unsigned char *);
	int linebreak;
	int nopair;
};

struct element_info elements[] = {
	{"SPAN",	html_span,	0, 0},
	{"B",		html_bold,	0, 0},
	{"STRONG",	html_bold,	0, 0},
	{"DFN",		html_bold,	0, 0},
	{"I",		html_italic,	0, 0},
	{"Q",		html_italic,	0, 0},
	{"EM",		html_italic,	0, 0},
	{"ABBR",	html_italic,	0, 0},
	{"U",		html_underline,	0, 0},
	{"S",		html_underline,	0, 0},
	{"STRIKE",	html_underline,	0, 0},
	{"FIXED",	html_fixed,	0, 0},
	{"CODE",	html_fixed,	0, 0},
	{"FONT",	html_font,	0, 0},
	{"A",		html_a,		0, 2},
	{"IMG",		html_img,	0, 1},
	{"IMAGE",	html_img,	0, 1},
	{"OBJECT",	html_object,	0, 0},
	{"EMBED",	html_embed,	0, 1},

	{"BASE",	html_base,	0, 1},
	{"BASEFONT",	html_font,	0, 1},

	{"BODY",	html_body,	0, 0},

/*	{"HEAD",	html_skip,	0, 0},*/
	{"TITLE",	html_title,	0, 0},
	{"SCRIPT",	html_script,	0, 0},
	{"NOSCRIPT",	html_noscript,	0, 0},
	{"STYLE",	html_skip,	0, 0},
	{"NOEMBED",	html_skip,	0, 0},

	{"BR",		html_br,	1, 1},
	{"DIV",		html_linebrk,	1, 0},
	{"CENTER",	html_center,	1, 0},
	{"CAPTION",	html_center,	1, 0},
	{"P",		html_p,		2, 2},
	{"HR",		html_hr,	2, 1},
	{"H1",		html_h1,	2, 2},
	{"H2",		html_h2,	2, 2},
	{"H3",		html_h3,	2, 2},
	{"H4",		html_h4,	2, 2},
	{"H5",		html_h5,	2, 2},
	{"H6",		html_h6,	2, 2},
	{"BLOCKQUOTE",	html_blockquote,2, 0},
	{"ADDRESS",	html_address,	2, 0},
	{"PRE",		html_pre,	2, 0},
	{"LISTING",	html_pre,	2, 0},

	{"UL",		html_ul,	1, 0},
	{"DIR",		html_ul,	1, 0},
	{"MENU",	html_ul,	1, 0},
	{"OL",		html_ol,	1, 0},
	{"LI",		html_li,	1, 3},
	{"DL",		html_dl,	1, 0},
	{"DT",		html_dt,	1, 1},
	{"DD",		html_dd,	1, 1},

	{"TABLE",	html_table,	2, 0},
	{"TR",		html_tr,	1, 0},
	{"TD",		html_td,	0, 0},
	{"TH",		html_th,	0, 0},

	{"FORM",	html_form,	1, 0},
	{"INPUT",	html_input,	0, 1},
	{"TEXTAREA",	html_textarea,	0, 1},
	{"SELECT",	html_select,	0, 0},
	{"OPTION",	html_option,	1, 1},
	{"BUTTON",	html_button,	0, 0},

	{"LINK",	html_link,	1, 1},
	{"IFRAME",	html_iframe,	1, 1},
	{"FRAME",	html_frame,	1, 1},
	{"FRAMESET",	html_frameset,	1, 0},
	{"NOFRAMES",	html_noframes,	0, 0},

	{NULL,		NULL, 0, 0},
};

unsigned char *skip_comment(unsigned char *html, unsigned char *eof)
{
	int comm = html + 4 <= eof && html[2] == '-' && html[3] == '-';
	html += comm ? 4 : 2;
	while (html < eof) {
		if (!comm && html[0] == '>') return html + 1;
		if (comm && html + 2 <= eof && html[0] == '-' && html[1] == '-') {
			html += 2;
			while (html < eof && *html == '-') html++;
			while (html < eof && WHITECHAR(*html)) html++;
			if (html >= eof) return eof;
			if (*html == '>') return html + 1;
			continue;
		}
		html++;
	}
	return eof;
}

void process_head(unsigned char *head)
{
	unsigned char *r, *p;
	struct refresh_param rp;
	if ((r = parse_http_header(head, (unsigned char *)"Refresh", NULL))) {
		if (!d_opt->auto_refresh) {
			if ((p = parse_header_param(r, (unsigned char *)"URL"))) {
				put_link_line((unsigned char *)"Refresh: ", p, p, d_opt->framename);
				mem_free(p);
			}
		} else {
			rp.url = parse_header_param(r, (unsigned char *)"URL");
			rp.time = atoi((char *)r);
			if (rp.time < 1) rp.time = 1;
			special_f(ff, SP_REFRESH, &rp);
			if (rp.url) mem_free(rp.url);
		}
		mem_free(r);
	}
}

void parse_html(unsigned char *html, unsigned char *eof, void (*put_chars)(void *, unsigned char *, int), void (*line_break)(void *), void *(*special)(void *, int, ...), void *f, unsigned char *head)
{
	/*unsigned char *start = html;*/
	unsigned char *lt;
	html_format_changed = 1;
	putsp = -1;
	line_breax = table_level ? 2 : 1;
	pos = 0;
	was_br = 0;
	put_chars_f = put_chars;
	line_break_f = line_break;
	special_f = special;
	ff = f;
	eoff = eof;
	if (head) process_head(head);
	set_lt:
	put_chars_f = put_chars;
	line_break_f = line_break;
	special_f = special;
	ff = f;
	eoff = eof;
	lt = html;
	while (html < eof) {
		unsigned char *name, *attr, *end;
		int namelen;
		struct element_info *ei;
		int inv;
		if (WHITECHAR(*html) && par_format.align != AL_NO) {
			unsigned char *h = html;
			/*if (putsp == -1) {
				while (html < eof && WHITECHAR(*html)) html++;
				goto set_lt;
			}
			putsp = 0;*/
			while (h < eof && WHITECHAR(*h)) h++;
			if (h + 1 < eof && h[0] == '<' && h[1] == '/') {
				if (!parse_element(h, eof, &name, &namelen, &attr, &end)) {
					put_chrs(lt, html - lt, put_chars, f);
					lt = html = h;
					putsp = 1;
					goto element;
				}
			}
			html++;
			if (!(pos + (html-lt-1))) goto skip_w; /* ??? */
			if (*(html - 1) == ' ') {
				if (html < eof && !WHITECHAR(*html) && !F) continue;	/* BIG performance win; not sure if it doesn't cause any bug */
				put_chrs(lt, html - lt, put_chars, f);
			} else {
				put_chrs(lt, html - 1 - lt, put_chars, f);
				put_chrs((unsigned char *)" ", 1, put_chars, f);
			}
			skip_w:
			while (html < eof && WHITECHAR(*html)) html++;
			/*putsp = -1;*/
			goto set_lt;
			put_sp:
			put_chrs((unsigned char *)" ", 1, put_chars, f);
			/*putsp = -1;*/
		}
		if (par_format.align == AL_NO) {
			putsp = 0;
			if (*html == 9) {
				put_chrs(lt, html - lt, put_chars, f);
				put_chrs((unsigned char *)"        ", 8 - pos%8, put_chars, f);
				html++;
				goto set_lt;
			} else if (*html == 13 || *html == 10) {
				put_chrs(lt, html - lt, put_chars, f);
				next_break:
				if (*html == 13 && html < eof-1 && html[1] == 10) html++;
				ln_break(1, line_break, f);
				html++;
				if (*html == 13 || *html == 10) {
					line_breax = 0;
					goto next_break;
				}
				goto set_lt;
			}
		}
		if (*html < ' ') {
			/*if (putsp == 1) goto put_sp;
			putsp = 0;*/
			put_chrs(lt, html - lt, put_chars, f);
			put_chrs((unsigned char *)".", 1, put_chars, f);
			html++;
			goto set_lt;
		}
		if (html + 2 <= eof && html[0] == '<' && (html[1] == '!' || html[1] == '?') && !(d_opt->plain & 1)) {
			/*if (putsp == 1) goto put_sp;
			putsp = 0;*/
			put_chrs(lt, html - lt, put_chars, f);
			html = skip_comment(html, eof);
			goto set_lt;
		}
		if (*html != '<' || d_opt->plain & 1 || parse_element(html, eof, &name, &namelen, &attr, &end)) {
			/*if (putsp == 1) goto put_sp;
			putsp = 0;*/
			html++;
			continue;
		}
		element:
		html_format_changed = 1;
		inv = *name == '/'; name += inv; namelen -= inv;
		if (!inv && putsp == 1 && !html_top.invisible) goto put_sp;
		put_chrs(lt, html - lt, put_chars, f);
		if (par_format.align != AL_NO) if (!inv && !putsp) {
			unsigned char *ee = end;
			unsigned char *nm;
			while (!parse_element(ee, eof, &nm, NULL, NULL, &ee))
				if (*nm == '/') goto ng;
			if (ee < eof && WHITECHAR(*ee)) {
				/*putsp = -1;*/
				put_chrs((unsigned char *)" ", 1, put_chars, f);
			}
			ng:;
		}
		html = end;
		for (ei = elements; ei->name; ei++) {
			if (ei->name &&
			   (strlen(( char *)ei->name) != namelen || casecmp((unsigned char *)ei->name, (unsigned char *)name, namelen)))
				continue;
			if (!inv) {
				char *a;
				ln_break(ei->linebreak, line_break, f);
				if (ei->name && ((a = (char *)get_attr_val(attr, (unsigned char *)"id")))) {
					special(f, SP_TAG, a);
					mem_free(a);
				}
				if (!html_top.invisible) {
					int a = par_format.align == AL_NO;
					struct par_attrib pa = par_format;
					if (ei->func == html_table && d_opt->tables && table_level < HTML_MAX_TABLE_LEVEL) {
						format_table(attr, html, eof, &html, f);
						ln_break(2, line_break, f);
						goto set_lt;
					}
					if (ei->func == html_select) {
						if (!do_html_select(attr, html, eof, &html, f))
							goto set_lt;
					}
					if (ei->func == html_textarea) {
						do_html_textarea(attr, html, eof, &html, f);
						goto set_lt;
					}
					if (ei->nopair == 2 || ei->nopair == 3) {
						struct html_element *e;
						if (ei->nopair == 2) {
							foreach(e, html_stack) {
								if (e->dontkill) break;
								if (e->linebreak || !ei->linebreak) break;
							}
						} else foreach(e, html_stack) {
							if (e->linebreak && !ei->linebreak) break;
							if (e->dontkill) break;
							if (e->namelen == namelen && !casecmp(e->name, name, e->namelen)) break;
						}
						if (e->namelen == namelen && !casecmp(e->name, name, e->namelen)) {
							while (e->prev != (void *)&html_stack) kill_html_stack_item(e->prev);
							if (e->dontkill != 2) kill_html_stack_item(e);
						}
					}
					if (ei->nopair != 1) {
						html_stack_dup();
						html_top.name = name;
						html_top.namelen = namelen;
						html_top.options = attr;
						html_top.linebreak = ei->linebreak;
					}
					if (ei->func) ei->func(attr);
					if (ei->func != html_br) was_br = 0;
					if (a) par_format = pa;
				}
			} else {
				struct html_element *e, *ff;
				int lnb = 0;
				int xxx = 0;
				was_br = 0;
				if (ei->nopair == 1 || ei->nopair == 3) break;
				/*debug_stack();*/
				foreach(e, html_stack) {
					if (e->linebreak && !ei->linebreak && ei->name) xxx = 1;
					if (e->namelen != namelen || casecmp(e->name, name, e->namelen)) {
						if (e->dontkill) break;
						else continue;
					}
					if (xxx) {
						kill_html_stack_item(e);
						break;
					}
					for (ff = e; ff != (void *)&html_stack; ff = ff->prev)
						if (ff->linebreak > lnb) lnb = ff->linebreak;
					ln_break(lnb, line_break, f);
					while (e->prev != (void *)&html_stack) kill_html_stack_item(e->prev);
					kill_html_stack_item(e);
					break;
				}
				/*debug_stack();*/
			}
			goto set_lt;
		}
		goto set_lt; /* unreachable */
	}
	put_chrs(lt, html - lt, put_chars, f);
	ln_break(1, line_break, f);
	putsp = -1;
	pos = 0;
	/*line_breax = 1;*/
	was_br = 0;
}

void scan_area_tag(unsigned char *attr, unsigned char *name, unsigned char **ptr, struct memory_list **ml)
{
	unsigned char *v;
	if ((v = get_attr_val(attr, name))) {
		*ptr = v;
		add_to_ml(ml, v, NULL);
	}
}

int get_image_map(unsigned char *head, unsigned char *s, unsigned char *eof, unsigned char *tag, struct menu_item **menu, struct memory_list **ml, unsigned char *href_base, unsigned char *target_base, int to, int def, int hdef, int gfx)
{
	unsigned char *name, *attr, *al, *label, *href, *target;
	int namelen, lblen;
	struct link_def *ld;
	struct menu_item *nm;
	int nmenu = 0;
	int i;
	unsigned char *hd = init_str();
	int hdl = 0;
	struct conv_table *ct;
	if (head) add_to_str(&hd, &hdl, head);
	scan_http_equiv(s, eof, &hd, &hdl, NULL, NULL, NULL);
	if (!gfx) ct = get_convert_table(hd, to, def, NULL, NULL, hdef);
	else ct = convert_table;
	mem_free(hd);
	if (!(*menu = mem_alloc(sizeof(struct menu_item)))) return -1;
	memset(*menu, 0, sizeof(struct menu_item));
	se:
	while (s < eof && *s != '<') {
		sp:
		s++;
	}
	if (s >= eof) {
		mem_free(*menu);
		return -1;
	}
	if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
		s = skip_comment(s, eof);
		goto se;
	}
	if (parse_element(s, eof, &name, &namelen, &attr, &s)) goto sp;
	if (namelen != 3 || casecmp(name, (unsigned char *)"MAP", 3)) goto se;
	if (tag && *tag) {
		if (!(al = get_attr_val(attr, (unsigned char *)"name"))) goto se;
		if (strcasecmp(( char *)al, ( char *)tag)) {
			mem_free(al);
			goto se;
		}
		mem_free(al);
	}
	*ml = getml(NULL);
	se2:
	while (s < eof && *s != '<') {
		sp2:
		s++;
	}
	if (s >= eof) {
		freeml(*ml);
		mem_free(*menu);
		return -1;
	}
	if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
		s = skip_comment(s, eof);
		goto se2;
	}
	if (parse_element(s, eof, &name, &namelen, &attr, &s)) goto sp2;
	if (namelen == 1 && !casecmp(name, (unsigned char *)"A", 1)) {
		unsigned char *ss;
		label = init_str();
		lblen = 0;
		se3:
		ss = s;
		while (ss < eof && *ss != '<') ss++;
		if (ss >= eof) {
			mem_free(label);
			freeml(*ml);
			mem_free(*menu);
			return -1;
		}
		add_bytes_to_str(&label, &lblen, s, ss - s);
		s = ss;
		if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
			s = skip_comment(s, eof);
			goto se3;
		}
		if (parse_element(s, eof, NULL, NULL, NULL, &ss)) goto se3;
		if (!((namelen == 1 && !casecmp(name,(unsigned char *) "A", 1)) ||
		      (namelen == 2 && !casecmp(name, (unsigned char *)"/A", 2)) ||
		      (namelen == 3 && !casecmp(name,(unsigned char *) "MAP", 3)) ||
		      (namelen == 4 && !casecmp(name,(unsigned char *) "/MAP", 4)) ||
		      (namelen == 4 && !casecmp(name, (unsigned char *)"AREA", 4)) ||
		      (namelen == 5 && !casecmp(name,(unsigned char *) "/AREA", 5)))) {
				s = ss;
				goto se3;
		}
	} else if (namelen == 4 && !casecmp(name,(unsigned char *) "AREA", 4)) {
		unsigned char *l = get_attr_val(attr, (unsigned char *)"alt");
		if (l) label = convert_string(ct, l, strlen(( char *)l), d_opt), mem_free(l);
		else label = NULL;
	} else if (namelen == 4 && !casecmp(name, (unsigned char *)"/MAP", 4)) goto done;
	else goto se2;
	if (!(href = get_url_val(attr, (unsigned char *)"href"))) {
		if (label) mem_free(label);
		goto se2;
	}
	if (!(target = get_target(attr)) && !(target = stracpy(target_base)))
		target = stracpy((unsigned char *)"");
	if (!(ld = mem_calloc(sizeof(struct link_def)))) {
		if (label) mem_free(label);
		mem_free(href);
		mem_free(target);
		goto se2;
	}
	if (!(ld->link = join_urls(href_base, href))) {
		mem_free(href);
		mem_free(target);
		f:
		mem_free(ld);
		if (label) mem_free(label);
		goto se2;
	}
	mem_free(href);
	ld->target = target;
	if (!gfx) for (i = 0; i < nmenu; i++) {
		struct link_def *ll = (*menu)[i].data;
		if (!strcmp(( char *)ll->link, ( char *)ld->link) && !strcmp(( char *)ll->target, ( char *)ld->target)) {
			mem_free(ld->link);
			mem_free(ld->target);
			goto f;
		}
	}
	if (label) clr_spaces(label);
	if (label && !*label) mem_free(label), label = NULL;
	if (!label) if (!(label = stracpy(ld->link))) {
		mem_free(href);
		mem_free(target);
		goto se2;
	}
	if ((nm = mem_realloc(*menu, (nmenu + 2) * sizeof(struct menu_item)))) {
		*menu = nm;
		memset(&nm[nmenu], 0, 2 * sizeof(struct menu_item));
		nm[nmenu].text = label;
		nm[nmenu].rtext = (unsigned char *)"";
		nm[nmenu].hotkey = (unsigned char *)"";
		nm[nmenu].func = MENU_FUNC map_selected;
		nm[nmenu].data = ld;
		nm[++nmenu].text = NULL;
	}
	add_to_ml(ml, ld, ld->link, ld->target, label, NULL);
	scan_area_tag(attr, (unsigned char *)"shape", &ld->shape, ml);
	scan_area_tag(attr, (unsigned char *)"coords", &ld->coords, ml);
	scan_area_tag(attr, (unsigned char *)"onclick", &ld->onclick, ml);
	scan_area_tag(attr, (unsigned char *)"ondblclick", &ld->ondblclick, ml);
	scan_area_tag(attr, (unsigned char *)"onmousedown", &ld->onmousedown, ml);
	scan_area_tag(attr, (unsigned char *)"onmouseup", &ld->onmouseup, ml);
	scan_area_tag(attr, (unsigned char *)"onmouseover", &ld->onmouseover, ml);
	scan_area_tag(attr, (unsigned char *)"onmouseout", &ld->onmouseout, ml);
	scan_area_tag(attr, (unsigned char *)"onmousemove", &ld->onmousemove, ml);
	goto se2;
	done:
	add_to_ml(ml, *menu, NULL);
	return 0;
}

void scan_http_equiv(unsigned char *s, unsigned char *eof, unsigned char **head, int *hdl, unsigned char **title, unsigned char **background, unsigned char **bgcolor)
{
	unsigned char *name, *attr, *he, *c;
	int namelen;
	int tlen = 0;
	if (background) *background = NULL;
	if (bgcolor) *bgcolor = NULL;
	if (title) *title = init_str();
	add_chr_to_str(head, hdl, '\n');
	se:
	while (s < eof && *s != '<') sp:s++;
	if (s >= eof) return;
	if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
		s = skip_comment(s, eof);
		goto se;
	}
	if (parse_element(s, eof, &name, &namelen, &attr, &s)) goto sp;
	ps:
	if (namelen == 4 && !casecmp(name, (unsigned char *)"BODY", 4)) {
		if (background) *background = get_attr_val(attr, (unsigned char *)"background");
		if (bgcolor) *bgcolor = get_attr_val(attr, (unsigned char *)"bgcolor");
		return;
	}
	if (title && !tlen && namelen == 5 && !casecmp(name, (unsigned char *)"TITLE", 5)) {
		unsigned char *s1;
		xse:
		s1 = s;
		while (s < eof && *s != '<') xsp:s++;
		add_bytes_to_str(title, &tlen, s1, s - s1);
		if (s >= eof) goto se;
		if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
			s = skip_comment(s, eof);
			goto xse;
		}
		if (parse_element(s, eof, &name, &namelen, &attr, &s)) goto xsp;
		clr_spaces(*title);
		goto ps;
	}
	if (namelen != 4 || casecmp(name, (unsigned char *)"META", 4)) goto se;
	if ((he = get_attr_val(attr, (unsigned char *)"charset"))) {
		add_to_str(head, hdl, (unsigned char *)"Charset: ");
		add_to_str(head, hdl, he);
		mem_free(he);
	}
	if (!(he = get_attr_val(attr, (unsigned char *)"http-equiv"))) goto se;
	c = get_attr_val(attr, (unsigned char *)"content");
	add_to_str(head, hdl, he);
	if (c) add_to_str(head, hdl, (unsigned char *)": "), add_to_str(head, hdl, c), mem_free(c);
	mem_free(he);
	add_to_str(head, hdl, (unsigned char *)"\r\n");
	goto se;
}

