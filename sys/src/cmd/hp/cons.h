/*  console state (for consctl) */
typedef struct Consstate	Consstate;
struct Consstate{
	int raw;
	int hold;
};

extern Consstate*	consctl(void);
extern Consstate*	cs;
