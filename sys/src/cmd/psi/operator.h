struct oobject {
	enum objtype otype;
	enum xattr oxattr;
	void (*v_op)(void);
};
struct	optab
{
	char	*name;
	struct oobject	operator;
};
