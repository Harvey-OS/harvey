#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#define	NLIFE	512		/* life array size */
#define	PX	4		/* cell spacing */
#define	BX	3		/* box size */
/*
 * life[i][j] stores L+2*N, where L is 0 if the cell is dead and 1
 * if alive, and N is the number of the cell's 8-connected neighbours
 * which live.
 * row[i] indicates how many cells are alive in life[i][*].
 * col[j] indicates how many cells are alive in life[*][j].
 * Adjust contains pointers to cells that need to have their neighbour
 * counts adjusted in the second pass of the generation procedure.
 *
 * In an attempt to stave off edge-effect trouble as long as possible,
 * the program re-centers the array whenever a border cell turns on.
 * The display jumps when this happens -- it should be rewritten so that
 * it doesn't.
 */
char life[NLIFE][NLIFE];
int row[NLIFE];
int col[NLIFE];
char action[18];		/* index by cell contents to find action */
#define	NADJUST	(NLIFE*NLIFE)
char *adjust[NADJUST];
Point cen;
Bitmap *box;
#define	setbox(i, j) bitblt(&screen, Pt(cen.x+(j)*PX, cen.y+(i)*PX), box, box->r, F)
#define	clrbox(i, j) bitblt(&screen, Pt(cen.x+(j)*PX, cen.y+(i)*PX), box, box->r, Zero)
int i0, i1, j0, j1;
void setrules(char *);
void main(int, char *[]);
int interest(int [NLIFE], int);
generate(void);
void birth(int, int);
void death(int, int);
void readlife(char *);
int min(int, int);
void centerlife(void);
void redraw(void);
void window(void);
void
setrules(char *r){
	register char *a;
	for(a=action;a!=&action[18];*a++=*r++);
}

void
usage(void){
	print("Usage: %s [-3o] [-r rules] [-s millisec] file\n", argv0);
	exits("usage");
}
void
main(int argc, char *argv[]){
	char *rules, *s;
	int delay=0;
	setrules(".d.d..b..d.d.d.d.d");			/* regular rules */
	ARGBEGIN{
	case '3': setrules(".d.d.db.b..d.d.d.d"); break;	/* 34-life */
	case 'o': setrules(".d.d.db.b.b..d.d.d"); break;	/* lineosc? */
	case 'r':						/* rules from cmdline */
		rules=ARGF();
		if(!rules) usage();
		setrules(rules);
		break;
	case 's':
		s=ARGF();
		if(s==0) usage();
		delay=atoi(s);
		break;
	default:
		usage();
	}ARGEND
	if(argc!=1) usage();
	binit(0,0,0);
	cen=div(sub(add(screen.r.min, screen.r.max), Pt(NLIFE*PX, NLIFE*PX)), 2);
	box=balloc(Rect(0, 0, BX, BX), screen.ldepth);
	redraw();
	readlife(argv[0]);
	do{
		bflush();
		sleep(delay);
	}while(generate());
}
/*
 * We can only have interest in a given row (or column) if there
 * is something alive in it or in the neighbouring rows (or columns.)
 */
int interest(int rc[NLIFE], int i){
	return(rc[i-1]!=0 || rc[i]!=0 || rc[i+1]!=0);
}
/*
 * A life generation proceeds in two passes.  The first pass identifies
 * cells that have births or deaths.  The `alive bit' is updated, as are
 * the screen and the row/col count deltas.  Also, a record is made
 * of the cell's address.  In the second pass, the neighbours of all changed
 * cells get their neighbour counts updated, and the row/col deltas get
 * merged into the row/col count arrays.
 *
 * The border cells (i==0 || i==NLIFE-1 || j==0 || j==NLIFE-1) are not
 * processed, purely for speed reasons.  With a little effort, a wrap-around
 * universe could be implemented.
 *
 * Generate returns 0 if there was no change from the last generation,
 * and 1 if there were changes.
 */
#define	neighbour(di, dj, op) lp[(di)*NLIFE+(dj)]op=2
#define	neighbours(op)\
	neighbour(-1, -1, op);\
	neighbour(-1,  0, op);\
	neighbour(-1,  1, op);\
	neighbour( 0, -1, op);\
	neighbour( 0,  1, op);\
	neighbour( 1, -1, op);\
	neighbour( 1,  0, op);\
	neighbour( 1,  1, op)
generate(void){
	char *lp;
	char **p, **addp, **delp;
	int i, j;
	int drow[NLIFE], dcol[NLIFE];
	int j0=NLIFE, j1 = -1;
	for(j=1;j!=NLIFE-1;j++){
		drow[j]=0;
		dcol[j]=0;
		if(interest(col, j)){
			if(j<j0) j0=j;
			if(j1<j) j1=j;
		}
	}
	addp = adjust;
	delp = &adjust[NADJUST];
	for(i=1;i!=NLIFE-1;i++) if(interest(row, i)){
		for(j=j0,lp = &life[i][j0];j<=j1;j++,lp++) switch(action[*lp]){
		case 'b':
			++*lp;
			++drow[i];
			++dcol[j];
			setbox(i, j);
			*addp++=lp;
			break;
		case 'd':
			--*lp;
			--drow[i];
			--dcol[j];
			clrbox(i, j);
			*--delp=lp;
			break;
		}
	}
	if(addp==adjust && delp==&adjust[NADJUST])
		return 0;
	if(delp<addp){
		print("Out of space (delp<addp)\n");
		exits("no space");
	}
	p=adjust;
	while(p!=addp){
		lp = *p++;
		neighbours(+);
	}
	p=delp;
	while(p!=&adjust[NADJUST]){
		lp = *p++;
		neighbours(-);
	}
	for(i=1;i!=NLIFE-1;i++){
		row[i]+=drow[i];
		col[i]+=dcol[i];
	}
	if(row[1] || row[NLIFE-2] || col[1] || col[NLIFE-2])
		centerlife();
	return 1;
}
/*
 * Record a birth at (i,j).
 */
void birth(int i, int j){
	char *lp;
	if(i<1 || NLIFE-1<=i || j<1 || NLIFE-1<=j || life[i][j]&1)
		return;
	lp=&life[i][j];
	++*lp;
	++row[i];
	++col[j];
	neighbours(+);
	setbox(i, j);
}
/*
 * Record a death at (i,j)
 */
void death(int i, int j){
	char *lp;
	if(i<1 || NLIFE-1<=i || j<1 || NLIFE-1<=j || !(life[i][j]&1))
		return;
	lp=&life[i][j];
	--*lp;
	--row[i];
	--col[j];
	neighbours(-);
	clrbox(i, j);
}
void readlife(char *filename){
	int i, j;
	char c;
	char name[1024];
	Biobuf *bp;
	if((bp=Bopen(filename, OREAD))==0){
		sprint(name, "/sys/games/lib/life/%s", filename);
		if((bp=Bopen(name, OREAD))==0){
			perror(filename);
			exits("open input");
		}
	}
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	for(i=0;i!=NLIFE;i++){
		row[i]=0;
		col[i]=0;
		for(j=0;j!=NLIFE;j++)
			life[i][j]=0;
	}
	for(i=1;i!=NLIFE-1;i++){
		j=1;
		while((c=Bgetc(bp))>=0 && c!='\n'){
			if(j!=NLIFE-1)switch(c){
			case '.':
				j++;
				break;
			case 'x':
				birth(i, j);
				j++;
				break;
			}
		}
		if(c<0) break;
	}
	centerlife();
	Bterm(bp);
}
int min(int a, int b){return(a<b?a:b);}
void centerlife(void){
	int i, j, di, dj, iinc, jinc, t;
	window();
	di=NLIFE/2-(i0+i1)/2;
	if(i0+di<1)
		di=1-i0;
	else if(i1+di>=NLIFE-1)
		di=NLIFE-2-i1;
	dj=NLIFE/2-(j0+j1)/2;
	if(j0+dj<1)
		dj=1-j0;
	else if(j1+dj>=NLIFE-1)
		dj=NLIFE-2-j1;
	if(di!=0 || dj!=0){
		if(di>0){ iinc=-1; t=i0; i0=i1; i1=t; } else iinc=1;
		if(dj>0){ jinc=-1; t=j0; j0=j1; j1=t; } else jinc=1;
		for(i=i0;i*iinc<=i1*iinc;i+=iinc)
			for(j=j0;j*jinc<=j1*jinc;j+=jinc)
				if(life[i][j]&1){
					birth(i+di, j+dj);
					death(i, j);
				}
	}
}
void redraw(void){
	int i, j;
	window();
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	for(i=i0;i<=i1;i++) for(j=j0;j<=j1;j++)
		if(life[i][j]&1)
			setbox(i, j);
}
void window(void){
	for(i0=1;i0!=NLIFE-2 && row[i0]==0;i0++);
	for(i1=NLIFE-2;i1!=i0 && row[i1]==0;--i1);
	for(j0=1;j0!=NLIFE-2 && col[j0]==0;j0++);
	for(j1=NLIFE-2;j1!=j0 && col[j1]==0;--j1);
}
