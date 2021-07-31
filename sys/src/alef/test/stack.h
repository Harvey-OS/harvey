
enum
{
	NSTACK	= 10,
};

adt Stack[T]
{
	int	tos;
	T	data[NSTACK];

	void	push(*Stack, T);
	T	pop(*Stack);
	void	init(*Stack);
	int	empty(*Stack);
};
