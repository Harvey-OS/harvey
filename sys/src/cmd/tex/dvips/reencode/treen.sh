#!/bin/csh
#   Tests font reencoding
afm2tfm Times-Roman.afm -v ptmr0 rptmr0
afm2tfm Times-Roman.afm -P extex.enc -v ptmr1 rptmr1
afm2tfm Times-Roman.afm -T extex.enc -v ptmr2 rptmr2
afm2tfm Times-Roman.afm -t extex.enc -v ptmr3 rptmr3
afm2tfm Times-Roman.afm -T extex.enc -V ptmr4 rptmr4
# these three should all be the same
cmp rptmr4.tfm rptmr2.tfm
cmp rptmr4.tfm rptmr1.tfm
# these two should be the same
cmp rptmr0.tfm rptmr3.tfm
afm2tfm Times-Roman.afm -T extex.enc -V ptmr4 rptmr2
vptovf ptmr0.vpl ptmr0.vf ptmr0.tfm
vptovf ptmr1.vpl ptmr1.vf ptmr1.tfm
vptovf ptmr2.vpl ptmr2.vf ptmr2.tfm
vptovf ptmr3.vpl ptmr3.vf ptmr3.tfm
vptovf ptmr4.vpl ptmr4.vf ptmr4.tfm
tex testfont <<EOF
ptmr0
\leftline{\bf Normal (original) Times-Roman; Standard + Text}
\sample\init
rptmr0
\table\vfill\eject\init
ptmr1
\leftline{\bf Times-Roman; Extended + Text}
\sample\init
rptmr1
\table\vfill\eject\init
ptmr2
\leftline{\bf Times-Roman; Extended + Extended}
\sample\vfill\eject\init
ptmr3
\leftline{\bf Times-Roman; Standard + Extended}
\sample\vfill\eject\init
ptmr4
\leftline{\bf Times-Roman Small Caps; Extended + Extended}
\sample\bye
EOF
dvips testfont -t testfont.ps
