struct Reqpool {
	Intmap	*map;
	Srv		*srv;
};

void _postfd(char*, int);

#define emalloc _lib9p_emalloc
#define erealloc _lib9p_erealloc
#define estrdup _lib9p_estrdup
