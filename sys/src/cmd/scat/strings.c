char *greek[]={ 0,	/* 1-indexed */
	"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
	"iota", "kappa", "lambda", "mu", "nu", "xsi", "omicron", "pi", "rho",
	"sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
};

Rune greeklet[]={ 0,
	L'α', L'β', L'γ', L'δ', L'ε', L'ζ', L'η', L'θ', L'ι', L'κ', L'λ',
	L'μ', L'ν', L'ξ', L'ο', L'π', L'ρ', L'σ', L'τ', L'υ', L'φ', L'χ',
	L'ψ', L'ω',
};

char *constel[]={ 0,	/* 1-indexed */
	"and", "ant", "aps", "aql", "aqr", "ara", "ari", "aur", "boo", "cae",
	"cam", "cap", "car", "cas", "cen", "cep", "cet", "cha", "cir", "cma",
	"cmi", "cnc", "col", "com", "cra", "crb", "crt", "cru", "crv", "cvn",
	"cyg", "del", "dor", "dra", "equ", "eri", "for", "gem", "gru", "her",
	"hor", "hya", "hyi", "ind", "lac", "leo", "lep", "lib", "lmi", "lup",
	"lyn", "lyr", "men", "mic", "mon", "mus", "nor", "oct", "oph", "ori",
	"pav", "peg", "per", "phe", "pic", "psa", "psc", "pup", "pyx", "ret",
	"scl", "sco", "sct", "ser", "sex", "sge", "sgr", "tau", "tel", "tra",
	"tri", "tuc", "uma", "umi", "vel", "vir", "vol", "vul",
};
Name names[]={
	"eg",		Galaxy,
	"pn",		PlanetaryN,
	"oc",		OpenCl,
	"gc",		GlobularCl,
	"dn",		DiffuseN,
	"nc",		NebularCl,
	"xx",		Nonexistent,
	"galaxy",	Galaxy,
	"planetary",	PlanetaryN,
	"opencluster",	OpenCl,
	"globularcluster",	GlobularCl,
	"nebula",	DiffuseN,
	"nebularcluster",NebularCl,
	"nonexistent",	Nonexistent,
	"unknown",	Unknown,
	0,		0,
};
char *types[]={ 0,	/* 1-indexed */
	"eg", "pn", "oc", "gc", "dn", "nc", "xx", "??",
};
