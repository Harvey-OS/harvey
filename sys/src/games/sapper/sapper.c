#include <u.h>
#include <libc.h>
#include <libg.h>

#include "eb/blank"
#include "eb/one"
#include "eb/two"
#include "eb/three"
#include "eb/four"
#include "eb/five"
#include "eb/six"
#include "eb/seven"
#include "eb/eight"
#include "eb/bad"
#include "eb/bomb"
#include "eb/flag"
#include "eb/full"
#include "eb/question"
#include "eb/led_0"
#include "eb/led_1"
#include "eb/led_2"
#include "eb/led_3"
#include "eb/led_4"
#include "eb/led_5"
#include "eb/led_6"
#include "eb/led_7"
#include "eb/led_8"
#include "eb/led_9"
#include "eb/led_minus"
#include "eb/led_blank"
#include "eb/title"

#define	Etimer		4
#define	HARDNESS	4.8		/* number of squares per mine */

#define	IS_BOMB		0x80		/* there is a bomb here */
#define	ME_FLAG		0x40		/* I said there was a bomb here */
#define	ME_QUST		0x20		/* I put a question here */
#define	EXPOSE		0x10		/* This one is exposed */
#define	BOMB_COUNT	0x0f		/* # of surrounding bombs */
#define	CLOCK_DIG	5		/* # of digits in the clock */
#define	BOMBS_DIG	5		/* # of digits in the bomb count */
enum bit_ptrs {
	blank_bitmap = 0,
	one_bitmap,
	two_bitmap,
	three_bitmap,
	four_bitmap,
	five_bitmap,
	six_bitmap,
	seven_bitmap,
	eight_bitmap,
	bomb_bitmap,
	bad_bitmap,
	flag_bitmap,
	full_bitmap,
	quest_bitmap,
	led_0,
	led_1,
	led_2,
	led_3,
	led_4,
	led_5,
	led_6,
	led_7,
	led_8,
	led_9,
	led_minus,
	led_blank,
	num_bitmaps
};
typedef enum bit_ptrs bit_ptrs;

enum play_st {
	quitting,
	to_push,
	in_game,
	end_game,
	dead_game,
	reshape_game
};
typedef enum play_st play_st;
char	*piccys[] = {
	&blank_bits[0],
	&one_bits[0],
	&two_bits[0],
	&three_bits[0],
	&four_bits[0],
	&five_bits[0],
	&six_bits[0],
	&seven_bits[0],
	&eight_bits[0],
	&bomb_bits[0],
	&bad_bits[0],
	&flag_bits[0],
	&full_bits[0],
	&question_bits[0],
	&led_0_bits[0],
	&led_1_bits[0],
	&led_2_bits[0],
	&led_3_bits[0],
	&led_4_bits[0],
	&led_5_bits[0],
	&led_6_bits[0],
	&led_7_bits[0],
	&led_8_bits[0],
	&led_9_bits[0],
	&led_minus_bits[0],
	&led_blank_bits[0]};
Bitmap	*bitmaps[num_bitmaps];
Bitmap	*title_bitmap;

Rectangle b_rect;		/* board rectangle */
Rectangle t_rect;		/* title rectangle */

char	*board = 0;		/* game board */
Point	*pstack = 0;		/* stack of spots to clear */
int	stackp = 0;		/* position in stack */
int	num_games = 0;
int	wid;			/* width of the board */
int	ht;			/* height of the board */
play_st	playing;		/* true while the game is on */
int	ticks = 0;		/* number of ticks for this game */
Point	clockp;
Point	bombsp;
int	num_bombs;
int	cur_bombs;
int	bleft;
int	cleared;
float	hardness = HARDNESS;

extern	Bitmap	screen;

/*
 * functions...
 */
void	screen_init(void);
void	bitmap_init(void);
void	put_map(int, Point);
void	assign_board(int, int, int);
void	set_board(int, int, int);
void	inc_board(int, int);
int	get_board(int, int);
void	init_board(void);
void	draw_board(void);
void	play(void);
void	again(void);
void	inc_clock(void);
void	mouse_event(struct Mouse);
void	push_posn(Point);
void	clear_board(int, int);
void	put_num(Point, int, int);
void	island(void);

void
screen_init(void)
{
	binit(0, 0, 0);
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	border(&screen, screen.r, 2, F);
}

void
bitmap_init(void)
{
	int	i;
	Bitmap	*b;

	for(i = 0; i < num_bitmaps; i++) {
		b = balloc(Rect(0, 0, 16, 16), 0);
		if (b == 0) {
			print("Bad alloc\n");
			exits("Bad alloc");
		}
		wrbitmap(b, 0, 16, (uchar *)piccys[i]);
		bitmaps[i] = b;
	}
	b = balloc(Rect(0, 0, 64, 32), 0);
	if (b == 0) {
		print("Bad alloc\n");
		exits("Bad alloc");
	}
	wrbitmap(b, 0, 32, (uchar *)title_bits);
	title_bitmap = b;
}

void
put_map(int map, Point p)
{
	if (map < 0 || map >= num_bitmaps)
		return;
	p.x = (p.x - 1) * 16;
	p.y = (p.y - 1) * 16;
	bitblt(&screen, add(b_rect.min, p), bitmaps[map], bitmaps[map]->r, S);
}

void
assign_board(int x, int y, int v)
{
	*(board + x + y * (wid + 2)) = v;
}

void
set_board(int x, int y, int v)
{
	*(board + x + y * (wid + 2)) |= v;
}

void
inc_board(int x, int y)
{
	*(board + x + y * (wid + 2))+=1;
}

int
get_board(int x, int y)
{
	return *(board + x + y * (wid + 2));
}

void
init_board(void)
{
	int	x;
	int	y;
	int	num;

	if (board != (char *)0) {
		free(board);
		free(pstack);
	}
	if ((board = (char *)malloc((wid + 2) * (ht + 2))) == (char *)0) {
		print("Bad Alloc of the board\n");
		exits("Bad alloc");
	}
	if ((pstack = (Point *)malloc(wid * ht * sizeof(Point))) == (Point *)0) {
		print("Bad Alloc of the stack\n");
		exits("Bad alloc");
	}
	memset((void *)board, 0, (wid + 2) * (ht + 2));	/* clear the board */
	num_games++;
	bleft = num_bombs = cur_bombs = num = (wid * ht) / hardness;
	for(; num; num--) {
		x = nrand(wid) + 1;
		y = nrand(ht) + 1;
		if (get_board(x, y) & IS_BOMB) {
			/*
			 * one already here
			 */
			num++;
			continue;
		}
		inc_board(x - 1, y - 1);
		inc_board(x - 1, y + 0);
		inc_board(x - 1, y + 1);
		inc_board(x + 0, y - 1);
		inc_board(x + 0, y + 1);
		inc_board(x + 1, y - 1);
		inc_board(x + 1, y + 0);
		inc_board(x + 1, y + 1);
		set_board(x, y, IS_BOMB);
	}
}

void
draw_board(void)
{
	int	x;
	int	y;

	border(&screen, inset(b_rect, -1), 1, F);
	for(x = 1; x <= wid; x++) {
		for(y = 1; y <= ht; y++) {
			put_map(full_bitmap, Pt(x, y));
		}
	}
	put_num(clockp, CLOCK_DIG, 0);
	ticks = -1;	/* for init delay */
	put_num(bombsp, BOMBS_DIG, cur_bombs);	
	bflush();
}

void
main(int argc, char *argv[])
{
	char	user[64];
	int	fd;
	char	buf[100];

	if (argc != 1) {
		hardness = atof(argv[1]);
		if (hardness < 1.0)
			hardness = HARDNESS;
	} else
		hardness = HARDNESS;
	screen_init();
	bitmap_init();
	srand(time(0));
	einit(Ekeyboard|Emouse);
	etimer(Etimer, 1000);	/* once a second please */
	if ((fd=open("/dev/user", 0)) >= 0) {
		read(fd, user, sizeof(user));
		close(fd);
	} else
		strcpy(user, "Anonymous");
	if ((fd=open("/sys/games/lib/sapper_scores", OWRITE)) < 0) {
		/* print("Cannot open /sys/games/lib/sapper_scores"); */
		/* exits("Scores File"); */
	}
	playing = to_push;	/* init */
	while(playing != quitting) {
		playing = to_push;
		wid = (screen.r.max.x - screen.r.min.x - 8)/16;
		ht = (screen.r.max.y - screen.r.min.y - 42)/16;
		if (wid < 16 || ht < 10) {
			print("Board too small - please make it bigger\n");
			exits("Board too small");
		}
		clockp = Pt(screen.r.max.x - (CLOCK_DIG+2)*16, screen.r.min.y+8);
		bombsp = add(screen.r.min, Pt(16, 8));
		cleared = 0;
		b_rect.min = add(screen.r.min, Pt(((screen.r.max.x - screen.r.min.x) - (wid * 16))/2, 38));
		b_rect.max = add(b_rect.min, Pt(wid*16, ht*16));
		t_rect.min = Pt((b_rect.max.x - b_rect.min.x - 64) / 2 + b_rect.min.x, (b_rect.max.y - b_rect.min.y - 32) / 2 + b_rect.min.y);
		t_rect.max = add(t_rect.min, Pt(64, 32));
		init_board();
		draw_board();
		play();
		if (playing != reshape_game && ticks > 30) {
			sprint(buf, "%s\t%g\t%dx%d\t%d\t%d\t%ld\n", user, hardness, wid, ht, bleft, cleared, ticks);
			write(fd, buf, strlen(buf));	
		}
		if (playing == end_game || playing == dead_game) {
			again();
		}
	}
	exits(0);
}

void
ereshaped(Rectangle r)
{
	screen.r = r;
	playing = reshape_game;
}

void
again(void)
{
	Event	e;
	int		cnt;

	cnt = 0;
	bitblt(&screen, t_rect.min, title_bitmap, title_bitmap->r, S);
	for(;;) {
		switch(event(&e)) {
		case Emouse:
			if(e.mouse.buttons&7)
				if(++cnt > 1) {
					playing = to_push;
					return;
				}
			break;
		case Etimer:
			break;
		case Ekeyboard:
			switch(e.kbdc) {
			case 'Q':
			case 'q':
			case 0x04:
			case 'n':
			case 'N':
				playing = quitting;
				return;
			case 'y':
			case 'Y':
				playing = to_push;
				return;
			}
			break;
		}
	}
}


void
play(void)
{
	Event	e;

	while(playing == in_game || playing == to_push) {
		switch(event(&e)) {
		case Emouse:
			mouse_event(e.mouse);
			break;
		case Etimer:
			inc_clock();
			break;
		case Ekeyboard:
			switch(e.kbdc) {
			case 'i':
				if(cleared == 0)
					island();
				break;
			case 'Q':
			case 'q':
			case 0x04:
				playing = quitting;
				break;
			}
			break;
		}
	}
}

void
inc_clock(void)
{
	if (playing == in_game)
		put_num(clockp, CLOCK_DIG, ++ticks);
}

Point
mouse_to_board(Point m)
{
	if (!ptinrect(m, b_rect))
		return Pt(0, 0);
	return add(div(sub(m, b_rect.min), 16), Pt(1, 1));
}

void
mouse_event(Mouse m)
{
	static	Mouse om = {0};
	int		up;
	Point		p;
	int		v;
	int		c;
	int		x;
	int		y;

	if ((om.buttons ^ m.buttons) == 0)
		return;
	/*
	 * The buttons have changed
	 */
	up = om.buttons;
	om = m;
	if ((up &= ~(m.buttons)) != 0) {
		playing = in_game;		/* we are off and running now */
		p = mouse_to_board(m.xy);
		if (p.x == 0)
			return;
		if (up & 1) {		/* Left */
			push_posn(p);
		}
		if (up & 2) {		/* Middle */
			x = p.x;
			y = p.y;
			v = get_board(x, y);
			if (v & EXPOSE) {
				c = 0;
				if (get_board(x-1, y-1) & ME_FLAG) c++;
				if (get_board(x-1, y) & ME_FLAG) c++;
				if (get_board(x-1, y+1) & ME_FLAG) c++;
				if (get_board(x, y-1) & ME_FLAG) c++;
				if (get_board(x, y) & ME_FLAG) c++;
				if (get_board(x, y+1) & ME_FLAG) c++;
				if (get_board(x+1, y-1) & ME_FLAG) c++;
				if (get_board(x+1, y) & ME_FLAG) c++;
				if (get_board(x+1, y+1) & ME_FLAG) c++;
				if (c == (v & BOMB_COUNT)) {
					push_posn(add(p, Pt(-1, -1)));
					push_posn(add(p, Pt(-1, 0)));
					push_posn(add(p, Pt(-1, 1)));
					push_posn(add(p, Pt(0, -1)));
					push_posn(add(p, Pt(0, 0)));
					push_posn(add(p, Pt(0, 1)));
					push_posn(add(p, Pt(1, -1)));
					push_posn(add(p, Pt(1, 0)));
					push_posn(add(p, Pt(1, 1)));
				}
			}
		}
		if (up & 4) {		/* Right */
			v = get_board(p.x, p.y);
			if (v & EXPOSE)
				return;
			if (v & ME_FLAG) {
				assign_board(p.x, p.y, (v & ~ME_FLAG)|ME_QUST);
				put_map(quest_bitmap, p);
				put_num(bombsp, BOMBS_DIG, ++cur_bombs);
				if (v & IS_BOMB)
					bleft++;	
			} else if (v & ME_QUST) {
				assign_board(p.x, p.y, v & ~(ME_QUST|ME_FLAG));
				put_map(full_bitmap, p);
			} else {
				assign_board(p.x, p.y, (v & ~ME_QUST)|ME_FLAG);
				put_map(flag_bitmap, p);
				put_num(bombsp, BOMBS_DIG, --cur_bombs);
				if (v & IS_BOMB)
					bleft--;	
			}
		}
		if (cleared + num_bombs == wid * ht) {
			/*
			 * We have cleared all the areas, we know
			 * That only bombs are left, so show them,
			 * and we are done.
			 */
			for (x = 1; x <= wid; x++) {
				for(y = 1; y <= ht; y++) {
					v = get_board(x, y);
					if (v & IS_BOMB && (~v) & ME_FLAG) {
						put_map(flag_bitmap, Pt(x, y));
					}
				}
			}
			put_num(bombsp, BOMBS_DIG, 0);
			playing = end_game;
			bleft = 0;
		}
				
	}
}

void
push_posn(Point p)
{
	int	v;
	int	x;
	int	y;

	if (p.x > wid || p.y > ht || p.x < 1 || p.y < 1 || playing != in_game)
		return;
	v = get_board(p.x, p.y);
	if (v & EXPOSE || v & ME_FLAG)
		return;		/* already shown it, or marked it */
	if (v & IS_BOMB) {
		/*
		 * Bang!
		 */
		for(x = 1; x <= wid; x++) {
			for(y = 1; y <= ht; y++) {
				v = get_board(x, y);
				if ((v & ME_FLAG) && ((~v) & IS_BOMB))
					put_map(bad_bitmap, Pt(x, y));
				else if ((v & IS_BOMB) && (v & ME_FLAG))
					continue;	/* got it !! */
				else if ((v & IS_BOMB) && ((~v) & EXPOSE))
					put_map(bomb_bitmap, Pt(x, y));
			}
		}
		playing = dead_game;
	} else if ((v & ME_FLAG) == 0) {
		cleared++;
		put_map(v & BOMB_COUNT, p);
		set_board(p.x, p.y, EXPOSE);
		if ((get_board(p.x, p.y) & BOMB_COUNT) == 0) {
			clear_board(p.x, p.y);
		}
	}
}

void
clear_board(int x, int y)
{
	int	tx;
	int	ty;
	int	v;
	int	nx;
	int	ny;

	stackp = 0;
	for(;;) {
		for(tx = -1; tx < 2; tx++) {
			nx = x + tx;
			for(ty = -1; ty < 2; ty++) {
				ny = y + ty;
				if (nx < 1 || ny < 1 || nx > wid || ny > ht)
					continue;
				v = get_board(nx, ny);
				if (v & (EXPOSE | ME_FLAG))
					continue;
				cleared++;
				set_board(nx, ny, EXPOSE);
				if ((v & BOMB_COUNT) == 0) {
					pstack[stackp] = Pt(nx, ny);
					stackp++;
				}
				put_map(v & BOMB_COUNT, Pt(nx, ny));
			}
		}
		bflush();
		if (stackp == 0)
			break;
		stackp--;
		x = pstack[stackp].x;
		y = pstack[stackp].y;
	}
}

void
put_num(Point p, int dig, int num)
{
	int	map;
	int	pos;

	if (num < 0) {
		map = led_minus;
		num = -num;
	} else
		map = led_blank;
	for(pos = 1; pos < dig; pos++)
		bitblt(&screen, add(p, Pt(pos*16, 0)), bitmaps[led_blank], bitmaps[led_blank]->r, S);
	bitblt(&screen, p, bitmaps[map], bitmaps[map]->r, S);
	pos = 0;
	do {
		int	digit = led_0 + (num % 10);
		bitblt(&screen, add(p, Pt((dig-pos)*16, 0)), bitmaps[digit], bitmaps[digit]->r, S);
		pos++;
		num /= 10;
	} while (num != 0);
}

void
island(void)
{
	int	x, y;

	for(;;) {
		x = nrand(wid)+1;
		y = nrand(ht)+1;
		if (get_board(x, y) == 0) {
			playing = in_game;
			push_posn((Point){x, y});
			return;
		}
	}
}
