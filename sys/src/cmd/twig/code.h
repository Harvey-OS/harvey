#define	CSEGSIZE	(512-2*sizeof(int))

typedef struct Code{
	struct Code	*prev;
	char	*firstFree;
	char	segment[CSEGSIZE];
}Code;

Code	*CodeStoreChar(Code*, char);
Code	*CodeGetBlock(void);
void	CodeFreeBlock(Code*);
Code	*CodeStoreString(Code*, char*);
Code	*CodeAppend(Code*, Code*);
void	CodeWrite(Biobuf*, Code*);
Code	*CodeMarkLine(Code*, int);
