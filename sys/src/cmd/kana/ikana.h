struct	map {
	char	*roma;
	char	*kana;
	char	advance;
};

struct map *match(uchar *p, int *nc, struct map *table);
