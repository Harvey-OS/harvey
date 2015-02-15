/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

 /* t0.c: storage allocation */
#
# include "t.h"
int expflg = 0;
int ctrflg = 0;
int boxflg = 0;
int dboxflg = 0;
int tab = '\t';
int linsize;
int pr1403;
int delim1, delim2;
int evenflg;
int *evenup;
int F1 = 0;
int F2 = 0;
int allflg = 0;
char *leftover = 0;
int textflg = 0;
int left1flg = 0;
int rightl = 0;
char *cstore, *cspace;
char *last;
struct colstr *table[MAXLIN];
int stynum[MAXLIN+1];
int fullbot[MAXLIN];
char *instead[MAXLIN];
int linestop[MAXLIN];
int (*style)[MAXHEAD];
char (*font)[MAXHEAD][2];
char (*csize)[MAXHEAD][4];
char (*vsize)[MAXHEAD][4];
int (*lefline)[MAXHEAD];
char (*cll)[CLLEN];
int (*flags)[MAXHEAD];
int qcol;
int *doubled, *acase, *topat;
int nslin, nclin;
int *sep;
int *used, *lused, *rused;
int nlin, ncol;
int iline = 1;
char *ifile = "Input";
int texname = 'a';
int texct = 0;
char texstr[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWYXZ0123456789";
int linstart;
char *exstore, *exlim, *exspace;
Biobuf *tabin  /*= stdin */;
Biobuf tabout  /* = stdout */;
