#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "system.h"
#include "stdio.h"
#include "object.h"
/*dummy up output routines for makefonts*/
void
done(int i){
exits("");
}
double fwr_bm_g31(void)
{
}

fwr_bm_g4(void)
{
}
void
putbitmap(void)
{
}
/*dummy image operators*/
void
imageOP(void)
{
}
void
imagemaskOP(void)
{
}
void
printCTM(char *label)
{
}
/*dummy clip operators*/
void
clipOP(void)
{
}
int
do_clip(void)
{
}
void
printpath(char *ss, struct path path, int max, int plot)
{
}
void
outlineOP(void)
{
}
void
eoclipOP(void)
{
}
/*dummy type1 stuff*/
struct object n_FontName;
struct dict *Internaldict;
int T1Dirleng;
struct object T1Names, T1Dir;
void
type1init(void)
{
}
int
df_getc(FILE *f)
{
}
void
ExecCharString(void)
{
}
void
eexecOP(void)
{
}
void
internaldictOP(void)
{
}
void
sdo_fill(void (*caller)(void))
{
}
void hsbpath(struct path path)
{
}
