/*
 * c-runtime for support of
 * long long type.
 */
typedef
long long vlong;
typedef
struct
{
	long	ms;
	long	ls;
} xlong;
vlong	fscale(vlong, int);

#define	NORM	1074
#define	VEXP	0x7FF00000L
#define	VSIGN	0x80000000L
#define	XSIGN	0x00100000L

static
xlong
vtox(vlong v)
{
	union { vlong v; xlong x; } u;

	u.v = fscale(v, -NORM);
	u.x.ms &= ~VEXP;
	return u.x;
}

static
vlong
xtov(xlong x)
{
	union { vlong v; xlong x; } u;

	u.x = x;
	u.x.ms &= ~VEXP;
	return fscale(u.v, NORM);
}

vlong
_vland(vlong a, vlong b)
{
	union { vlong v; xlong x; } ua, ub;

	ua.x = vtox(a);
	ub.x = vtox(b);
	ua.x.ms &= ub.x.ms;
	ua.x.ls &= ub.x.ls;
	return xtov(ua.x);
}
	
vlong
_vlor(vlong a, vlong b)
{
	union { vlong v; xlong x; } ua, ub;

	ua.x = vtox(a);
	ub.x = vtox(b);
	ua.x.ms |= ub.x.ms;
	ua.x.ls |= ub.x.ls;
	return xtov(ua.x);
}
	
vlong
_vlxor(vlong a, vlong b)
{
	union { vlong v; xlong x; } ua, ub;

	ua.x = vtox(a);
	ub.x = vtox(b);
	ua.x.ms ^= ub.x.ms;
	ua.x.ls ^= ub.x.ls;
	return xtov(ua.x);
}
	
vlong
_vlcom(vlong a)
{
	union { vlong v; xlong x; } ua;

	ua.x = vtox(a);
	ua.x.ms ^= ~VEXP;
	ua.x.ls ^= ~0L;
	return xtov(ua.x);
}
	
vlong
_vllsh(vlong a, int b)
{

	return fscale(a, b);
}
	
vlong
_vlrsh(vlong a, int b)
{

	return fscale(fscale(a, -b-NORM), NORM);
}
