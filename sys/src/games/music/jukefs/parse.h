Object	*getobject(Type, Object *);
Object	*getinclude(Object *);
void	childsort(Object *);
void	childenum(Object *);
Object	*newobject(Type, Object *);
void	freeobject(Object *, char *);
void	freetree(Object *);
void	*mymalloc(void *old, int size);
void	addchild(Object *, Object *, char*);
void	addcatparent(Object *, Object *);
void	inittokenlist(void);
void	initparse(void);
void	exit(int);

extern char *startdir;
