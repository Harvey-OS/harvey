#include	"fizz.h"
#include	"crack.h"

#define		NPLANES		100
#define		NWIRES		100
#define		NDRILLSZ	50

Drillsz dfdrillsz[] = {
'A', 33, 'a', (Point *) 0, 0,
'B', 34, 'a', (Point *) 0, 0,
'C', 39, 'a', (Point *) 0, 0,
'D', 42, 'a', (Point *) 0, 0,
'E', 50, 'a', (Point *) 0, 0,
'F', 62, 'a', (Point *) 0, 0,
'G', 106, 'a', (Point *) 0, 0,
'H', 107, 'a', (Point *) 0, 0,
'I', 108, 'a', (Point *) 0, 0,
'J', 20, 'a', (Point *) 0, 0,
'K', 110, 'a', (Point *) 0, 0,
'L', 111, 'a', (Point *) 0, 0,
'M', 112, 'a', (Point *) 0, 0,
'N', 113, 'a', (Point *) 0, 0,
'O', 114, 'a', (Point *) 0, 0,
'P', 115, 'a', (Point *) 0, 0,
'Q', 116, 'a', (Point *) 0, 0,
'R', 117, 'a', (Point *) 0, 0,
'S', 118, 'a', (Point *) 0, 0,
'T', 119, 'a', (Point *) 0, 0,
'U', 100, 'a', (Point *) 0, 0,
'V', 20, 'B', (Point *) 0, 0,
'W', 122, 'a', (Point *) 0, 0,
'X', 123, 'a', (Point *) 0, 0,
'Y', 124, 'a', (Point *) 0, 0,
'Z', 125, 'a', (Point *) 0, 0
};

static Keymap keys[] = {
	"name", (VFN)1,
	"align", (VFN)2,
	"plane", (VFN)3,
	"layer", (VFN)4,
	"datums", (VFN)5,
	"wires", (VFN)6,
	"drillsz", (VFN)7,
	0
};

void
f_board(s)
	char *s;
{
	register nm, loop;
	int i;
	char *os;
	Plane planes[NPLANES], wires[NWIRES];
	Drillsz  drillsz[NDRILLSZ];
	register Plane *pp = planes, *qq = wires;
	Drillsz *rr = drillsz;


	BLANK(s);
	if(loop = *s == '{')
		s = f_inline();
	do {
		if(*s == '}') break;
		switch((int)f_keymap(keys, &s))
		{
		case 1:
			f_b->name = f_strdup(s);
			break;
		case 2:
			COORD(s, f_b->align[0].x, f_b->align[0].y);
			COORD(s, f_b->align[1].x, f_b->align[1].y);
			COORD(s, f_b->align[2].x, f_b->align[2].y);
			COORD(s, f_b->align[3].x, f_b->align[3].y);
			break;
		case 3:
			NUM(s, i)
			if((i < 0) || (i >= MAXLAYER))
				f_minor("bad layer number %d", i);
			pp->layer = LAYER(i);
			BLANK(s);
			if(*s == '+')
				pp->sense = 1;
			else if(*s == '-')
				pp->sense = -1;
			else
				f_minor("expected a sense [+-], got '%c'", *s);
			s++;
			BLANK(s);
			os = s;
			NAME(s);
			pp->sig = f_strdup(os);
			NUM(s, pp->r.min.x)
			NUM(s, pp->r.min.y)
			NUM(s, pp->r.max.x)
			NUM(s, pp->r.max.y)
			if(++pp >= &planes[NPLANES])
				f_major("Too many (%d) plane defns", NPLANES);
			break;
		case 4:
			BLANK(s);
			os = s;
			NAME(s);
			NUM(s, i);
			if((i < 0) || (i >= MAXLAYER)){
				f_minor("bad layer number %d", i);
				break;
			}
			if(f_b->layer[i])
				f_minor("Redefining layer %d from %s to %s", i, f_b->layer[i], os);
			else
				f_b->layer[i] = f_strdup(os);
			break;
		case 5:
			NUM(s, f_b->datums[0].p.x)
			NUM(s, f_b->datums[0].p.y)
			NUM(s, i)
			if((i != 45) && (i != 135))
				f_minor("bad datum orientation %d", i);
			else
				f_b->datums[0].drill = i == 135 ? '\\' : '/';
			NUM(s, f_b->datums[1].p.x)
			NUM(s, f_b->datums[1].p.y)
			NUM(s, i)
			if((i != 45) && (i != 135))
				f_minor("bad datum orientation %d", i);
			else
				f_b->datums[1].drill = i == 135 ? '\\' : '/';
			NUM(s, f_b->datums[2].p.x)
			NUM(s, f_b->datums[2].p.y)
			NUM(s, i)
			if((i != 45) && (i != 135))
				f_minor("bad datum orientation %d", i);
			else
				f_b->datums[2].drill = i == 135 ? '\\' : '/';
			break;
		case 6:
			BLANK(s);
			if(*s == '+')
				qq->sense = 1;
			else if(*s == '-')
				qq->sense = -1;
			else
				f_minor("expected a sense [+-], got '%c'", *s);
			s++;
			BLANK(s);
			NUM(s, qq->r.min.x)
			NUM(s, qq->r.min.y)
			NUM(s, qq->r.max.x)
			NUM(s, qq->r.max.y)
			if(++qq >= &wires[NWIRES])
				f_major("Too many (%d) wires defns", NPLANES);
			break;
		case 7:
			BLANK(s);
			if(loop = *s == '{')
				s = f_inline();
			do {
				BLANK(s);
				if(*s == '}') break;
				rr->letter = *s++;
				BLANK(s);
				NUM(s, rr->dia);
				BLANK(s);
				rr->type = *s++;
				if(*s) EOLN(s)
				if(++rr >= &drillsz[NDRILLSZ])
					f_major("Too many (%d) drillsz defns", NDRILLSZ);
			} while(loop && (s = f_inline()));
			break;

			
		default:
			f_minor("Board: unknown field: '%s'\n", s);
			break;
		}
	} while(loop && (s = f_inline()));
	if(pp != planes){
		f_b->nplanes = pp-planes;
		f_b->planes = (Plane *)f_malloc(f_b->nplanes*(long)sizeof(Plane));
		memcpy((char *)f_b->planes, (char *)planes, f_b->nplanes*sizeof(Plane));
	}
	if(rr != drillsz){
		f_b->ndrillsz = rr-drillsz;
		f_b->drillsz = (Drillsz *)f_malloc(f_b->ndrillsz*(long)sizeof(Drillsz));
		memcpy((char *)f_b->drillsz, (char *)drillsz, f_b->ndrillsz*sizeof(Drillsz));
	}
	else {
		f_b->drillsz = dfdrillsz;
		f_b->ndrillsz = 26;
	}
	if(qq != wires){
		f_b->nkeepouts = qq-wires;
		f_b->keepouts = (Plane *)f_malloc(f_b->nkeepouts*(long)sizeof(Plane));
		memcpy((char *)f_b->keepouts, (char *)wires, f_b->nkeepouts*sizeof(Plane));
	}
}
