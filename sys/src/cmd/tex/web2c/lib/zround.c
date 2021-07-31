/* zround.c: round R to the nearest whole number.  */

#include "config.h"

integer
zround (r)
  double r;
{
  return r >= 0.0 ? (r + 0.5) : (r - 0.5);
}
