/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

"/Page null def\n"
"/Page# 0 def\n"
"/PDFSave null def\n"
"/DSCPageCount 0 def\n"
"/DoPDFPage {dup /Page# exch store pdfgetpage pdfshowpage } def\n"
"\n"
"/pdfshowpage_mysetpage {	% <pagedict> pdfshowpage_mysetpage <pagedict>\n"
"  dup /CropBox pget {\n"
"      boxrect\n"
"      2 array astore /PageSize exch 4 2 roll\n"
"      4 index /Rotate pget {\n"
"        dup 0 lt {360 add} if 90 idiv {exch neg} repeat\n"
"      } if\n"
"      exch neg exch 2 array astore /PageOffset exch\n"
"      << 5 1 roll >> setpagedevice\n"
"  } if\n"
"} bind def\n"
"\n"
"GS_PDF_ProcSet begin\n"
"pdfdict begin\n"
