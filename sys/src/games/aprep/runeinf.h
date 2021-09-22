typedef	struct	Runeinf	Runeinf;
struct	Runeinf
{
	uchar	class;
	Rune	lower;
	Rune	upper;
	char	latin[4];
};

enum
{
	Unknown	= 0,
	Alpha,
	Control,
	Null,
	Numer,
	Punct,
	Space,
};

void	runeinf(Rune, Runeinf*);
