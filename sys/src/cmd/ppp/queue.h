typedef aggr Qelem;

adt Queue {
	Lock;
	Qelem	*elems;

	void	putq(*Queue, void*);
	void*	getq(*Queue);
	int	isempty(*Queue);
};
