/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>

void
secp256r1(mpint *p, mpint *a, mpint *b, mpint *x, mpint *y, mpint *n, mpint *h){
  strtomp("FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF", nil, 16, p);
  uitomp(3UL, a);
  mpsub(p,a,a);
  strtomp("5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B", nil, 16, b);
  strtomp("6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296", nil, 16, x);
  strtomp("4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5", nil, 16, y);
  strtomp("FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551", nil, 16, n);
  mpassign(mpone, h);
}
