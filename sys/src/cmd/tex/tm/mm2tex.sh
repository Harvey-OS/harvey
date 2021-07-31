#!/bin/sh
# Filter from troff/mm to LaTex/tm.sty
# Created by Terry L Anderson 91 May 28
# Standard in: file in MM format.
# Standard out: file in LaTex format using tm.sty
MACROS=`echo $TEXINPUTS|sed -n -e "s/^\([^:]*macros\).*$/\1/p" \
                     -e "s/^.*:\([^:]*macros\).*$/\1/p" `
MM2TEX=${MM2TEX:-$MACROS}
sed -f $MM2TEX/mm2tex.sed | \
    nawk -f $MM2TEX/mm2tex.awk|sed -f $MM2TEX/mm2tex.sed2
