/* extensible Strings */
typedef struct String {
	char *base;	/* base of String */
	char *end;	/* end of allocated space+1 */
	char *ptr;	/* ptr into String */
} String;

#define s_putc(s,c)\
	if ((s)->ptr<(s)->end) *((s)->ptr)++ = c; else s_grow(s,c)
#define s_terminate(s)\
	if ((s)->ptr<s->end) *(s)->ptr = 0; else{ s_grow(s,0); (s)->ptr--; }
#define s_clone(s) s_copy((s)->base)
#define s_to_c(s) (s)->base
#define s_len(s) ((s)->ptr-(s)->base)

extern String	*s_alloc(void);
extern void	s_error(char*, char*);
extern void	s_simplegrow(String*, int);
extern int	s_grow(String*, int);
extern String	*s_new(void);
extern String	*s_newalloc(int);
extern void	s_free(String*);
extern String	*s_append(String*, char*);
extern String	*s_nappend(String*, char*, int);
extern String	*s_memappend(String*, char*, int);
extern String	*s_array(char*, int);
extern String	*s_copy(char*);
extern String	*s_parse(String*, String*);
extern void	s_tolower(String*);
extern String*	s_reset(String*);
extern String*	s_restart(String*);

#ifdef BGETC
extern int	s_read(Biobuf*, String*, int);
extern char	*s_read_line(Biobuf*, String*);
extern char	*s_getline(Biobuf*, String*);
#endif BGETC
