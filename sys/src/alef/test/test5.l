
adt Lock
{
	int	value;

	void	lock(*Lock, int);
	void	unlock(void);
};

aggr Foo
{
	int 	a;
	int 	y;
	Lock;
	int	blah;
};

void
main(void)
{
	Foo *section;

	section.lock(123);
	section->value = 1;

	lock();
	k = lock.head;
	lock.head = k->next;
	unlock();

}
