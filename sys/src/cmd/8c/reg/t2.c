int t1, t2, t3, t4;
int f(int);
int x;
void
f11(void)
{
	x=((t1=t2*2), f(t1))-((t3=t4*5), t3);
}
