#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <String.h>
#include "dat.h"

extern Tree *tree;
extern void fsopen(Req*, Fid*, int, Qid*);
extern void fsread(Req*, Fid*, void*, long*, vlong);
extern void fswrite(Req*, Fid*, void*, long*, vlong);
extern void fsclunkaux(Fid*);
extern void fsattach(Req*, Fid*, char*, Qid*);
extern void fsflush(Req*, Req*);
