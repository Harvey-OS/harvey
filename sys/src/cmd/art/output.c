/*
 * Low level output.  Mostly routines simulating or replacing jerq functions.
 */
#include "art.h"
void rectf(Bitmap *dp, Rectangle r, Fcode code){
	bitblt(dp, r.min, dp, r, code);
}
