/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Auth Auth;
struct Auth {
	char *name;

	char* (*auth)(Fcall*, Fcall*);
	char* (*attach)(Fcall*, Fcall*);
	void (*init)(void);
	char* (*read)(Fcall*, Fcall*);
	char* (*write)(Fcall*, Fcall*);
	char* (*clunk)(Fcall*, Fcall*);
};

extern char remotehostname[];
extern char Eauth[];
extern char *autharg;

extern Auth authp9any;
extern Auth authrhosts;
extern Auth authnone;

extern ulong truerand(void);
extern void randombytes(uchar*, uint);

extern ulong  msize;

typedef struct Fid Fid;
Fid *newauthfid(int fid, void *magic, char **ep);
Fid *oldauthfid(int fid, void **magic, char **ep);

void safecpy(char *to, char *from, int len);
