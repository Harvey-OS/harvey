#include <alef.h>
#include <bio.h>

void	spout(int, byte*);

Biobuf bout;

void
main(int argc, byte **argv)
{
	int i, fd;

	bout.init(1, OWRITE);
	if(argc == 1)
		spout(0, "");
	else
		for(i=1; i<argc; i++){
			fd = open(argv[i], OREAD);
			if(fd < 0){
				fprint(2, "spell: can't open %s: %r\n", argv[i]);
				continue;
			}
			spout(fd, argv[i]);
			close(fd);
		}
	exits(nil);
}

#define	alpha(c)	(('a'<=(c) && (c)<='z') || ('A'<=(c) && (c)<='Z'))
Biobuf b;

void
spout(int fd, byte *name)
{
	byte *s, *t, *w;
	Rune r;
	int inword, wordchar;
	int n, wn, wid, c, m;
	byte buf[1024];

	b.init(fd, OREAD);
	n = 0;
	wn = 0;
	while((s = b.rdline('\n')) != nil){
		if(s[0] == '.')
			for(c=0; c<3 && *s>' '; c++){
				n++;
				s++;
			}
		inword = 0;
		w = s;
		t = s;
		do{
			c = *t;
			if(c < Runeself)
				wid = 1;
			else{
				wid = chartorune(&r, t);
				c = r;
			}
			wordchar = 0;
			if(alpha(c))
				wordchar = 1;
			if(inword && !wordchar){
				if(c=='\'' && alpha(t[1]))
					goto Continue;
				m = t-w;
				if(m > 1){
					memmove(buf, w, m);
					buf[m] = 0;
					bout.print("%s:#%d,#%d:%s\n", name, wn, n, buf);
				}
				inword = 0;
			}else if(!inword && wordchar){
				wn = n;
				w = t;
				inword = 1;
			}
			if(c=='\\' && (alpha(t[1]) || t[1]=='(')){
				switch(t[1]){
				case '(':
					m = 4;
					break;
				case 'f':
					if(t[2] == '(')
						m = 5;
					else
						m = 3;
					break;
				case 's':
					if(t[2] == '+' || t[2]=='-'){
						if(t[3] == '(')
							m = 6;
						else
							m = 4;
					}else{
						if(t[2] == '(')
							m = 5;
						else if(t[2]=='1' || t[2]=='2' || t[2]=='3')
							m = 4;
						else
							m = 3;
					}
					break;
				default:
					m = 2;
				}
				while(m-- > 0){
					if(*t == '\n')
						break;
					n++;
					t++;
				}
				continue;
			}
	Continue:
			n++;
			t += wid;
		}while(c != '\n');
	}
	bout.flush();
}
