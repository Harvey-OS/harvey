Rune text[];
char *buttons[]={
	"cut",
	"paste",
	"snarf",
	"exit",
	0
};
Rune *snarfbuf=0;
int nsnarfbuf=0;
Panel *edit;
void hitmenu(int, int);
void snarf(void);
