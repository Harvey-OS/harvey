extern void init_vars(void);
extern char * __cdecl getval(char *name);
extern char *setval(char *name,char *value);
extern char *setval_default(char *name,char *value);
extern char **grep(char *regexp,char *line,int num_vars) ;

extern boolean parse_variable(char *line);
extern char *my_basename(char *name,char *suffix);
extern char *my_dirname(const char *name);
extern char *concat_pathes(const char *p1,const char *p2);

extern char *normalize(char *path);
extern char *strcasestr(char *s1,char *s2);

extern char *subst(char *line, char *from, char *to);
extern string expand_var(string);
