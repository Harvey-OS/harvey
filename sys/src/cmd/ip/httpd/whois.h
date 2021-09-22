extern Ndbtuple*	whois(char*, char*);
extern int		classify(char*, Ndbtuple*);

enum {
	Cok,
	Cbadgov,
	Cbadc,
	Cunknown
};
