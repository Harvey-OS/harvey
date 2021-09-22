#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <libsec.h>
#include <auth.h>
#include "httpd.h"
#include "httpsrv.h"

static	Hio	houtb;
static	Hio	*hout;
static	HConnect	*connect;

static	char	me[] = "/magic/coffee/";

enum
{
	MNAMELEN	= 64
};

typedef struct Page Page;
struct Page
{
	char	*uri;
	void	(*fn)(HConnect*);
	int	check;
};

int monthly = 10;
char monthlydate[] = "October 2004";

static void	dobag(HConnect*);
static void	dohello(HConnect*);
static void	doaccount(HConnect*);
static char*	getval(char*, char*, char*, int);
static void	head(char*);
static void	foot(void);
static void	paragraph(char*);
static void	h2(char*);
static void	errorpage(char*);
static void	radioform(char *page, char *button, char *name, char *val1, char *val2);
static void	textinput(char *page, char *button, char *name, int len);
static void	inventory(void);
static void	account(char *who);

Page pages[] = {
	{ "hello",	dohello,	0	},
	{ "bag",	dobag,		0	},
	{ "account",	doaccount,		0	},
	{ "",		dohello,	0	},
	{ 0, 0 }
};

int regular;
int decaf;
Tm today;

void
main(int argc, char **argv)
{
	Page *pg;
	char *uri;
	char *uriparts[3];

	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);

	connect = init(argc, argv);
	hout = &connect->hout;
	if(hparseheaders(connect, HSTIMEOUT) < 0)
		exits("failed");

	if(strcmp(connect->req.meth, "GET") != 0 && strcmp(connect->req.meth, "HEAD") != 0){
		hunallowed(connect, "GET, HEAD");
		exits("unallowed");
	}
	if(connect->head.expectother || connect->head.expectcont){
		hfail(connect, HExpectFail, nil);
		exits("failed");
	}

	/* find the current page */
	uri = strdup(connect->req.uri);
	memset(uriparts, 0, sizeof(uriparts));
	getfields(uri, uriparts, 3, 1, "/");
	for(pg = pages; pg->uri != 0; pg++){
		if(uriparts[0] == nil)
			break;
		if(strcmp(pg->uri, uriparts[0]) == 0)
			break;
	}

	if(pg->uri == nil)
		errorpage("unknown page");

	(*pg->fn)(connect);
	hflush(&connect->hout);

	exits(nil);
}

static void
dohello(HConnect*)
{
	char b[128];
	char *x;

	inventory();
	head("coffee club");
	h2("Coffee inventory");
	snprint(b, sizeof b, "regular: %d bags", regular);
	paragraph(b);
	snprint(b, sizeof b, "decaf: %d bags", decaf);
	paragraph(b);
	paragraph("If you've taken a bag of coffee from my office select the type and click 'consume'");
	radioform("bag", "consume", "type", "regular", "decaf");
	h2("Check your account");
	paragraph("Enter your account name and and click 'account'");
	textinput("account", "account", "account", 50);
	h2("New");
	x = smprint("%s: The coffee fund is $%d per month", monthlydate, monthly);
	paragraph(x);
	free(x);
	foot();
}

static void
doaccount(HConnect *c)
{
	char who[128];

	getval(c->req.search, "account", who, sizeof(who));

	head("coffee club account");
	account(who);
	foot();
}

static void
decrement(char *type)
{
	int fd;
	
	today = *localtime(time(0));
	fd = open("/lib/coffee/inventory", ORDWR);
	if(fd < 0)
		errorpage("can't open file, contact sape");
	seek(fd, 0, 2);
	fprint(fd, "%s -1 %d/%d/%d %d:%2.2d\n", type, today.mon+1, today.mday, today.year+1900,
		today.hour, today.min);
}

static void
dobag(HConnect *c)
{
	char b[128];

	getval(c->req.search, "type", b, sizeof(b));
	if(strcmp(b, "regular") == 0)
		decrement("regular");
	else if(strcmp(b, "decaf") == 0)
		decrement("decaf");
	else {
		errorpage("Specify regular or decaf!");
	}

	dohello(c);
}

/*
 *  get a field out of a search string
 */
static char*
getval(char *search, char *attr, char *buf, int len)
{
	int n;
	char *a = buf;

	len--;
	n = strlen(attr);
	while(search != nil){
		if(strncmp(search, attr, n) == 0 && *(search+n) == '='){
			search += n+1;
			while(len > 0 && *search && *search != '&'){
				*buf++ = *search++;
				len--;
			}
			break;
		}
		search = strchr(search, '&');
		if(search)
			search++;
	}
	*buf = 0;

	if(strcmp(a, "none") == 0 || strcmp(a, "auto") == 0)
		*a = 0;

	return a;
}

static void
head(char *title)
{
	hokheaders(connect);
	hprint(hout, "Content-type: text/html\r\n");
	hprint(hout, "\r\n");
	hprint(hout, "<HEAD><TITLE>%se</TITLE></HEAD><BODY>\n", title);
}

static void
h2(char *txt)
{
	hprint(hout, "<H2>%s</H2>\n", txt);
}

static void
foot(void)
{
	hprint(hout, "</body></html>\n");
}

static void
paragraph(char *txt)
{
	hprint(hout, "<p>\n");
	hprint(hout, "%s", txt);
	hprint(hout, "</p>\n");
}

static void
radioform(char *page, char *button, char *name, char *val1, char *val2)
{
	hprint(hout, "<FORM ACTION=\"/magic/coffee/%s\" METHOD=GET>\n", page);
	hprint(hout, "<INPUT TYPE=radio NAME=%s VALUE=%s>\n", name, val1);
	hprint(hout, "%s\n", val1);
	hprint(hout, "<br>\n");
	hprint(hout, "<INPUT TYPE=radio NAME=%s VALUE=%s>\n", name, val2);
	hprint(hout, "%s\n", val2);
	hprint(hout, "<br>\n");
	hprint(hout, "<INPUT type=submit value=\"%s\">\n", button);
	hprint(hout, "</FORM>\n");
}

static void
textinput(char *page, char *button, char *name, int len)
{
	hprint(hout, "<FORM ACTION=\"/magic/coffee/%s\" METHOD=GET>\n", page);
	hprint(hout, "<input type=text name=%s maxLength=%d size=%d>\n", name, len, len);
	hprint(hout, "<br>\n");
	hprint(hout, "<INPUT type=submit value=\"%s\">\n", button);
	hprint(hout, "</FORM>\n");
}

static void
errorpage(char *msg)
{
	head("coffee error");
	h2("You screwed up");
	paragraph(msg);
	foot();
	hflush(&connect->hout);
	exits(nil);
}

static void
inventory(void)
{
	Biobuf *inv;
	char *p;
	char *f[10];
	int n;

	inv = Bopen("/lib/coffee/inventory", OREAD);
	if(inv == nil)
		return;
	regular = 0;
	while(p = Brdline(inv, '\n')){
		p[Blinelen(inv)-1] = 0;
		n = tokenize(p, f, nelem(f));
		if(n < 3)
			continue;
		if(strcmp(f[0], "regular") == 0)
			regular += atoi(f[1]);
		if(strcmp(f[0], "decaf") == 0)
			decaf += atoi(f[1]);
	}
	Bterm(inv);
}

typedef struct Trans Trans;
struct Trans
{
	Trans	*next;
	int	amount;
	Tm	when;
	uchar	expense;
	uchar	away;
};

typedef struct Person Person;
struct Person
{
	char	*name;
	Person	*next;
	Trans	*first;
	Trans	*last;
	int	year;
	int	mon;

	int	balance;
};

Person	*people, *owner;

Tm start;
int uflag;

static Person*
findperson(char *name)
{
	Person **l, *p;

	for(l = &people; *l != nil; l = &(*l)->next){
		p = *l;
		if(strcmp(p->name, name) == 0)
			return p;
	}
	p = *l = mallocz(sizeof(*p), 1);
	p->name = strdup(name);
	p->year = start.year;
	p->mon = start.mon;
	return p;
}

static void
getlogin(char **f, int nf, char *name)
{
	int i;

	*name = 0;
	for(i = 0; i < nf; i++){
		if(strpbrk(f[i], "$/") == 0){
			strcpy(name, f[i]);
			break;
		}
	}
}	

static void
getdate(char **f, int nf, Tm *when)
{
	int i;
	Tm t;
	char *p, *q;

	t = today;
	for(i = 0; i < nf; i++){
		if(p = strchr(f[i], '/')){
			t.mon = atoi(f[i])-1;
			p++;
			q = strchr(p, '/');
			if(q == nil){
				t.year = atoi(p);
			} else {
				t.mday = atoi(p);
				t.year = atoi(q+1);
			}
			if(t.year < 10)
				t.year += 100;
			if(t.year > 1900)
				t.year -= 1900;
			
			*when = t;
			break;
		}
	}
	
}

static void
getamount(char **f, int nf, int *amount)
{
	int i, mult;
	char *p;

	*amount = 0;
	for(i = 0; i < nf; i++){
		if(p = strchr(f[i], '$')){
			if(f[i][0] == '-')
				mult = -1;
			else
				mult = 1;
			*amount = mult*atoi(p+1);
			break;
		}
	}
}

static int
starting(char **f, int nf)
{
	int i;

	for(i = 0; i < nf; i++){
		if(strcmp(f[i], "^") == 0)
			return 1;
	}
	return 0;
}

static int
expense(char **f, int nf)
{
	int i;

	for(i = 0; i < nf; i++){
		if(strcmp(f[i], "!") == 0)
			return 1;
	}
	return 0;
}

static int
away(char **f, int nf)
{
	int i;

	for(i = 0; i < nf; i++){
		if(strcmp(f[i], "-") == 0)
			return 1;
	}
	return 0;
}

static void
paytoowner(void)
{
	Person *person;
	Trans *t;

	if(owner == nil){
		fprint(2, "!no owner\n");
		return;
	}
	for(person = people; person != nil; person = person->next){
		for(t = person->first; t != nil; t = t->next)
			if(!t->expense && !t->away)
				owner->balance -= t->amount;
	}
}

static int
future(Tm *now, Tm *t)
{
	if(t->year > now->year)
		return 1;
	if(t->year < now->year)
		return 0;
	if(t->mon > now->mon)
		return 1;
	return 0;
}

static void
chargepermonth(void)
{
	Person *person;
	int charge, start;
	int change = 12*104+1;

	charge = (12*today.year + today.mon);
	if(charge < change)
		charge *= monthly/2;
	else
		charge = (change)*monthly/2 + (charge-change)*monthly;

	for(person = people; person != nil; person = person->next){
		person->balance -= charge;
		start = 12*person->year + person->mon;
		if(start < change)
			person->balance += start*monthly/2;
		else
			person->balance += change*monthly/2 + (start-change)*monthly;
	}
}

void
printaccount(char *who)
{
	Person *person;
	char b[128];
	Trans *t;

	for(person = people; person != nil; person = person->next){
		if(strcmp(person->name, who) == 0)
			break;
	}
	if(person == nil)
		return;
	snprint(b, sizeof b, "%s's account status", who);
	h2(b);
	if(person->balance == 0)
		paragraph("You are even!");
	else if(person->balance < 0){
		snprint(b, sizeof b, "You owe the fund $%d.", -person->balance);
		paragraph(b);
	} else {
		snprint(b, sizeof b, "You are ahead of the game $%d.<br>Cool!", person->balance);
		paragraph(b);
	}
	h2("Payment history");
	hprint(hout, "<table>\n");
	for(t = person->first; t; t = t->next){
		if(t->when.year == 100)
			continue;
		if(t->away)
			hprint(hout, "<tr><td>%2.2d/%2.2d/%d</td><td>away</td></tr>\n", t->when.mon+1, t->when.mday, t->when.year+1900);
		else
			hprint(hout, "<tr><td>%2.2d/%2.2d/%d</td><td>$%d</td></tr>\n", t->when.mon+1, t->when.mday, t->when.year+1900, t->amount);
	}
	hprint(hout, "</table>\n");
}

static void
account(char *who)
{
	char *line, *p;
	Person *person;
	Tm tm;
	char name[256];
	int amount;
	Trans *t;
	char *f[10];
	int nf;
	Biobuf *in;

	today = *localtime(time(0));
	start = today;
	today.hour = 12;
	today.min = today.sec = 0;

	
	in = Bopen("/lib/coffee/payments", OREAD);
	if(in == nil)
		return;
	while(line = Brdline(in, '\n')){
		line[Blinelen(in)-1] = 0;
		if(*line == 0)
			continue;
		p = strdup(line);
		nf = tokenize(p, f, nelem(f));
		if(*line == '#'){
			if(strcmp(f[0]+1, "start") == 0)
				getdate(f, nf, &start);
			else if(strcmp(f[0]+1, "owner") == 0)
				owner = findperson(f[1]);
			free(p);
			continue;
		}

		getlogin(f, nf, name);
		if(*name == 0){
//			fprint(2, "no name %s\n", line);
			free(p);
			continue;
		}

		getamount(f, nf, &amount);
		if(*name == 0){
//			fprint(2, "no amount %s\n", line);
			free(p);
			continue;
		}

		getdate(f, nf, &tm);

		// ignore the future
		if(future(&today, &tm))
			continue;

		t = mallocz(sizeof(*t), 1);
		t->amount = amount;
		t->when = tm;
		if(expense(f, nf))
			t->expense = 1;
		if(away(f, nf)){
			amount = 10;
			t->away = 1;
		}

		person = findperson(name);
		if(starting(f, nf)){
			person->year = t->when.year;
			person->mon = t->when.mon;
		}
		if(person->first == nil)
			person->first = t;
		else
			person->last->next = t;
		person->last = t;
		person->balance += amount;
	}

	chargepermonth();
	paytoowner();
	printaccount(who);
	Bterm(in);
}
