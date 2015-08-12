/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

static char *PS_head[] = {
    "%ps_include: begin\n",
    "save\n",
    "/ed {exch def} def\n",
    "{} /showpage ed\n",
    "{} /copypage ed\n",
    "{} /erasepage ed\n",
    "{} /letter ed\n",
    "currentdict /findfont known systemdict /findfont known and {\n",
    "	/findfont systemdict /findfont get def\n",
    "} if\n",
    "36 dict dup /PS-include-dict-dw ed begin\n",
    "/context ed\n",
    "count array astore /o-stack ed\n",
    "%ps_include: variables begin\n",
    0};

static char *PS_setup[] = {
    "%ps_include: variables end\n",
    "{llx lly urx ury} /bbox ed\n",
    "{newpath 2 index exch 2 index exch dup 6 index exch\n",
    " moveto 3 {lineto} repeat closepath} /boxpath ed\n",
    "{dup mul exch dup mul add sqrt} /len ed\n",
    "{2 copy gt {exch} if pop} /min ed\n",
    "{2 copy lt {exch} if pop} /max ed\n",
    "{transform round exch round exch A itransform} /nice ed\n",
    "{6 array} /n ed\n",
    "n defaultmatrix n currentmatrix n invertmatrix n concatmatrix /A ed\n",
    "urx llx sub 0 A dtransform len /Sx ed\n",
    "0 ury lly sub A dtransform len /Sy ed\n",
    "llx urx add 2 div lly ury add 2 div A transform /Cy ed /Cx ed\n",
    "rot dup sin abs /S ed cos abs /C ed\n",
    "Sx S mul Sy C mul add /H ed\n",
    "Sx C mul Sy S mul add /W ed\n",
    "sy H div /Scaley ed\n",
    "sx W div /Scalex ed\n",
    "s 0 eq {Scalex Scaley min dup /Scalex ed /Scaley ed} if\n",
    "sx Scalex W mul sub 0 max ax 0.5 sub mul cx add /cx ed\n",
    "sy Scaley H mul sub 0 max ay 0.5 sub mul cy add /cy ed\n",
    "urx llx sub 0 A dtransform exch atan rot exch sub /rot ed\n",
    "n currentmatrix initgraphics setmatrix\n",
    "cx cy translate\n",
    "Scalex Scaley scale\n",
    "rot rotate\n",
    "Cx neg Cy neg translate\n",
    "A concat\n",
    "bbox boxpath clip newpath\n",
    "w 0 ne {gsave bbox boxpath 1 setgray fill grestore} if\n",
    "end\n",
    "gsave\n",
    "%ps_include: inclusion begin\n",
    0};

static char *PS_tail[] = {
    "%ps_include: inclusion end\n",
    "grestore\n",
    "PS-include-dict-dw begin\n",
    "o 0 ne {gsave A defaultmatrix /A ed llx lly nice urx ury nice\n",
    "	initgraphics 0.1 setlinewidth boxpath stroke grestore} if\n",
    "clear o-stack aload pop\n",
    "context end restore\n",
    "%ps_include: end\n",
    0};
