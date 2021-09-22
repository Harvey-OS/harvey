/*
 * , operators
 */

int	v1, v2, v3, v4;

void
f1(void)
{
	(v2++, v3=v4);
}

void
f2(void)
{
	v1 = (v2++, v3=v4);
}

void
f3(void)
{
	v1 = (v2++, v3)*v4;
}

void
f4(void)
{
	(v1++, ++v3);
}

void
f5(void)
{
	v1 = (v2++, (v3++, v1))*v4;
}

void
f6(void)
{
	v1 = -((v2=v2+1), v3);
}

void
f7(void)
{
	v1 = v2==v3? (v4++, v2): (v3++, v1);
}

void
f8(void)
{
	v1 = (v2++, v3)==v1 && (v3++, v4)==2;
}

char
f9(void)
{
	return (char)((v1 = v2++), (v3==v4? v1+1: v2+2));
}

int g(int, int, int);

void
f10(void)
{
	g(((v1=v2++,v1)), (v3==v4? v1+1: v2+2), ((v2=v3),v4++,v1+1));
}
