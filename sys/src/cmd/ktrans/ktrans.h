struct	map {
	char	*roma;
	char	*kana;
	char	advance;
};

struct map *match(uchar *p, int *nc, struct map *table);
int changelang(int);
Rune	*hlook(Rune *);
int	nrune(char *);
int	hcmp (Rune *, Rune  *, Rune **);
void	unrune(char *, Rune *);
extern	Rune dict[];
extern	int ndict;


