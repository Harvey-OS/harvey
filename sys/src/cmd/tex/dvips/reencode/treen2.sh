#!/bin/csh
#   Tests font reencoding
afm2tfm Times-Roman.afm -T funky.enc -v ptmr9 rptmr9
vptovf ptmr9.vpl ptmr9.vf ptmr9.tfm
tex testfont <<EOF
ptmr9
\leftline{\bf Funky Times-Roman; Standard + Funky}
\sample
Y through x or x.  pp and then pp and then pp.  pp//pp//pp.  ++ // pippy
\bye
EOF
dvips testfont -o testfont.ps
