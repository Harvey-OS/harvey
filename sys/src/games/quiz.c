#include <u.h>
#include <libc.h>
#include <bio.h>

enum
{
	NF	= 10,
	NL	= 300,
	NC	= 200,
	SL	= 100,
	NA	= 10,
};

int	tflag;
int	xx[NL];
char	score[NL];
int	rights;
int	wrongs;
int	guesses;
Biobuf*	input;
Biobuf*	output;
int	nl = 0;
int	na = NA;
int	ptr = 0;
int	nc = 0;
char	line[150];
char	response[100];
char*	tmp[NF];
int	select[NF];
char*	eu;
char*	ev;

int	readline(void);
int	cmp(char *u, char *v);
int	disj(int s);
int	string(int s);
int	eat(int s, int c);
int	fold(int c);
void	publish(char *t);
void	pub1(int s);
int	segment(char *u, char *w[]);
int	perm(char *u[], int m, char *v[], int n, int p[]);
int	find(char *u[], int m);
void	readindex(void);
void	talloc(void);
void	main(int argc, char *argv[]);
int	query(char *r);
int	next(void);
void	done(void);
void	instruct(char *info);
void	badinfo(void);
void	dunno(void);
void	out(void*, char*);

int
readline(void)
{
	char *t;

loop:
	for(t=line; (*t=Bgetc(input)) != Beof; t++) {
		nc++;
		if(*t==' ' && (t==line || t[-1]==' '))
			t--;
		if(*t == '\n') {
			if(t[-1]=='\\')		/*inexact test*/
				continue;
			while(t>line && t[-1]==' ')
				*--t = '\n';
			t[1] = 0;
			return 1;
		}
		if(t-line >= NC) {
			Bprint(output, "Too hard for me\n");
			do {
				*line = Bgetc(input);
				if(*line == 0377)
					return 0;
			} while(*line != '\n');
			goto loop;
		}
	}
	return 0;
}

int
cmp(char *u, char *v)
{
	int x;

	eu = u;
	ev = v;
	x = disj(1);
	if(x != 1)
		return x;
	return eat(1,0);
}

int
disj(int s)
{
	int t, x;
	char *u;

	u = eu;
	t = 0;
	for(;;) {
		x = string(s);
		if(x > 1)
			return x;
		switch(*ev) {
		case 0:
		case ']':
		case '}':
			return t|x&s;
		case '|':
			ev++;
			t |= s;
			s = 0;
			continue;
		}
		if(s)
			eu = u;
		if(string(0) > 1)
			return 2;
		switch(*ev) {
		case 0:
		case ']':
			return 0;
		case '}':
			return 1;
		case '|':
			ev++;
			continue;
		default:
			return 2;
		}
	}
}

int
string(int s)
{
	int x;

	for(;;) {
		switch(*ev) {
		case 0:
		case '|':
		case ']':
		case '}':
			return 1;
		case '\\':
			ev++;
			if(*ev == 0)
				return 2;
			if(*ev == '\n') {
				ev++;
				continue;
			}
		default:
			if(eat(s,*ev) == 1)
				continue;
			return 0;
		case '[':
			ev++;
			x = disj(s);
			if(*ev!=']' || x>1)
				return 2;
			ev++;
			if(s == 0)
				continue;
			if(x == 0)
				return 0;
			continue;
		case '{':
			ev++;
			x = disj(s);
			if(*ev!='}' || x>1)
				return 2;
			ev++;
			continue;
		}
	}
	return 0;
}

int
eat(int s, int c)
{
	if(*ev != c)
		return 2;
	if(s == 0) {
		ev++;
		return 1;
	}
	if(fold(*eu) != fold(c))
		return 0;
	eu++;
	ev++;
	return 1;
}

int
fold(int c)
{
	if(c<'A' || c>'Z')
		return c;
	return c + ('a'-'A');
}

void
publish(char *t)
{
	ev = t;
	pub1(1);
}

void
pub1(int s)
{
	for(;; ev++){
		switch(*ev) {
		case '|':
			s = 0;
			ev;
			continue;
		case ']':
		case '}':
		case 0:
			return;
		case '[':
		case '{':
			ev++;
			pub1(s);
			ev;
			continue;
		case '\\':
			if(*++ev == '\n')
				continue;
		default:
			if(s)
				Bputc(output, *ev);
		}
	}
}

int
segment(char *u, char *w[])
{
	char *s;
	int i;
	char *t;

	s = u;
	for(i=0; i<NF; i++) {
		u = s;
		t = w[i];
		while(*s!=':' && *s!='\n' && s-u<SL) {
			if(*s == '\\')  {
				if(s[1] == '\n') {
					s += 2;
					continue;
				}
				*t++ = *s++;
			}
			*t++ = *s++;
		}

		while(*s!=':' && *s!='\n')
			s++;
		*t = 0;
		if(*s++ == '\n') {
			return i+1;
		}
	}
	Bprint(output, "Too many facts about one thing\n");
	return 0;
}

int
perm(char *u[], int m, char *v[], int n, int p[])
{
	int i, j;
	int x;

	for(i=0; i<m; i++) {
		for(j=0; j<n; j++) {
			x = cmp(u[i], v[j]);
			if(x > 1)
				badinfo();
			if(x == 0)
				continue;
			p[i] = j;
			goto uloop;
		}
		return 0;
	uloop:;
	}
	return 1;
}

int
find(char *u[], int m)
{
	int n;

	while(readline()) {
		n = segment(line,tmp);
		if(perm(u, m, tmp+1, n-1, select))
			return 1;
	}
	return 0;
}

void
readindex(void)
{
	xx[0] = nc = 0;
	while(readline()) {
		xx[++nl] = nc;
		if(nl >= NL) {
			Bprint(output, "I've forgotten some of it;\n");
			Bprint(output, "I remember %d items.\n", nl);
			break;
		}
	}
}

void
talloc(void)
{
	int i;

	for(i=0;i<NF;i++)
		tmp[i] = malloc(SL);
}

void
main(int argc, char *argv[])
{
	int i, j, x, z;
	char *info;
	int count;
	Biobuf obuf;

	output = &obuf;
	Binit(output, 1, OWRITE);
	info = "/sys/games/lib/quiz/INDEX";
	srand(time(0));

loop:
	if(argc>1 && *argv[1]=='-') {
		switch(argv[1][1]) {
		case 'i':
			if(argc > 2) 
				info = argv[2];
			argc -= 2;
			argv += 2;
			goto loop;
		case 't':
			tflag = 1;
			argc--;
			argv++;
			goto loop;
		}
	}
	input = Bopen(info, OREAD);
	if(input == 0) {
		Bprint(output, "No info\n");
		exits(0);
	}
	talloc();
	if(argc <= 2)
		instruct(info);
	notify(out);
	argv[argc] = 0;
	if(find(&argv[1], argc-1)==0)
		dunno();
	Bclose(input);
	input = Bopen(tmp[0], OREAD);
	if(input == 0)
		dunno();
	readindex();
	if(!tflag || na>nl)
		na = nl;
	for(;;) {
		i = next();
		Bseek(input, xx[i]+0L, 0);
		z = xx[i+1]-xx[i];
		for(j=0; j<z; j++)
			line[j] = Bgetc(input);
		segment(line, tmp);
		if(*tmp[select[0]] == '\0' || *tmp[select[1]] == '\0') {
			score[i] = 1;
			continue;
		}
		publish(tmp[select[0]]);
		Bprint(output, "\n");
		for(count=0;; count++) {
			if(query(response) == 0) {
				publish(tmp[select[1]]);
				Bprint(output, "\n");
				if(count == 0)
					wrongs++;
				score[i] = tflag? -1: 1;
				break;
			}
			x = cmp(response, tmp[select[1]]);
			if(x > 1)
				badinfo();
			if(x == 1) {
				Bprint(output, "Right!\n");
				if(count==0)
					rights++;
				if(++score[i]>=1 && na<nl)
					na++;
				break;
			}
			Bprint(output, "What?\n");
			if(count == 0)
				wrongs++;
			score[i] = tflag? -1: 1;
		}
		guesses += count;
	}
}

int
query(char *r)
{
	char *t;

	Bflush(output);
	for(t=r;; t++) {
		if(read(0, t, 1) == 0)
			done();
		if(*t==' ' && (t==r || t[-1]==' '))
			t--;
		if(*t == '\n') {
			while(t>r && t[-1]==' ')
				*--t = '\n';
			break;
		}
	}
	*t = 0;
	return t-r;
}

int
next(void)
{
	int flag;

	ptr = nrand(na);
	flag = 0;
	while(score[ptr] > 0)
		if(++ptr >= na) {
			ptr = 0;
			if(flag)
				done();
			flag = 1;
		}
	return ptr;
}

void
done(void)
{
	Bprint(output, "\nRights %d, wrongs %d, ", rights, wrongs);
	if(guesses)
		Bprint(output, "extra guesses %d, ", guesses);
	Bprint(output, "score %d%%\n", 100*rights/(rights+wrongs));
	exits(0);
}

void
out(void *v, char *t)
{
	USED(v);
	USED(t);
	done();
}

void
instruct(char *info)
{
	int i, n;

	Bprint(output, "Subjects:\n\n");
	while(readline()) {
		Bprint(output, "-");
		n = segment(line, tmp);
		for(i=1; i<n; i++) {
			Bprint(output, " ");
			publish(tmp[i]);
		}
		Bprint(output, "\n");
	}
	Bprint(output, "\n");
	input = Bopen(info, OREAD);
	if(input==0)
		abort();
	readline();
	segment(line,tmp);
	Bprint(output, "For example,\n");
	Bprint(output, "    games/quiz ");
	publish(tmp[1]);
	Bprint(output, " ");
	publish(tmp[2]);
	Bprint(output, "\nasks you a ");
	publish(tmp[1]);
	Bprint(output, " and you answer the ");
	publish(tmp[2]);
	Bprint(output, "\n    games/quiz ");
	publish(tmp[2]);
	Bprint(output, " ");
	publish(tmp[1]);
	Bprint(output, "\nworks the other way around\n");
	Bprint(output, "\nType empty line to get correct answer.\n");
	exits(0);
}

void
badinfo(void)
{
	Bprint(output, "Bad info %s\n",line);
}

void
dunno(void)
{
	Bprint(output, "I don't know about that\n");
	exits(0);
}
