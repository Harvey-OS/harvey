/* split out for interoperability with factotum */

#include <thread.h>
#include "/sys/src/lib9p/impl.h"
#include <9p.h>

typedef struct Config Config;
typedef struct Key Key;
typedef struct Proto Proto;
typedef struct Symbol Symbol;

struct Symbol {
	char *name;
	String *val;
};

struct Config {
	Key **key;
	int nkey;

	Symbol *sym;
	int nsym;
};

enum {
	Fdirty = 1<<0,
	Fconfirmopen = 1<<1,
	Fconfirmuse = 1<<2,
};
struct Key {
	String *data;

	char *file;
	Proto *proto;
	int flags;
	Symbol *sym;
};

struct Proto {
	char	*name;
	void	*(*open)(Key*, char*, int);
	int	(*read)(void*, void*, int);
	int	(*write)(void*, void*, int);
	void	(*close)(void*);
	int perm;	/* 0666, 0444, 0222 */
};

extern Config *strtoconfig(Config*, String*);
extern String *configtostr(Config*, int unsafe);
extern void freeconfig(Config*);

extern Proto *prototab[];

void setconfig(Config*);
extern int verbose;

enum {
	STACK = 16384
};

void memrandom(void*, int);
char *safecpy(char*, char*, int);

#define ANAMELEN NAMELEN
char *user;	/* host id */

int confirm(char*);
void agentlog(char*, ...);
#pragma varargck argpos agentlog 1
