%!
/setupforms{/landscape exch def /formsperpage exch def /rotation 1 def
userdict /currentform 0 put /slop 5 def /min{2 copy gt{exch}if pop}def
/columns formsperpage sqrt ceiling cvi def /rows formsperpage columns
div ceiling cvi def 6 array defaultmatrix 6 array currentmatrix 6 array
invertmatrix 6 array concatmatrix /tempmatrix exch def 0 slop tempmatrix
dtransform dup mul exch dup mul add sqrt /slop exch def newpath clippath
pathbbox 2 index sub dup /formheight exch def slop 2 mul sub /pageheight
exch def 2 index sub dup /formwidth exch def slop 2 mul sub /pagewidth
exch def userdict /gotpagebbox known userdict /pagebbox known and{
newpath pagebbox 0 get pagebbox 1 get tempmatrix transform moveto
pagebbox 0 get pagebbox 3 get tempmatrix transform lineto pagebbox 2 get
pagebbox 3 get tempmatrix transform lineto pagebbox 2 get pagebbox 1 get
tempmatrix transform lineto closepath pathbbox 2 index sub /formheight
exch def 2 index sub /formwidth exch def}{2 copy}ifelse /ycorner exch
def /xcorner exch def translate slop slop translate pagewidth pageheight
gt{rows columns /rows exch def /columns exch def}if pagewidth formwidth
columns mul div pageheight formheight rows mul div min pageheight
formwidth columns mul div pagewidth formheight rows mul div min 2 copy
lt{rotation 1 eq{landscape{0 pageheight translate -90 rotate}{pagewidth
0 translate 90 rotate}ifelse}{landscape{pagewidth 0 translate 90 rotate}
{0 pageheight translate -90 rotate}ifelse}ifelse pagewidth pageheight
/pagewidth exch def /pageheight exch def exch}if pop dup dup scale
xcorner neg ycorner neg translate 0 rows 1 sub formheight mul translate
dup pagewidth exch div formwidth columns mul sub 2 div exch pageheight
exch div formheight rows mul sub 2 div translate /showpage{formsperpage
1 gt{gsave .1 setlinewidth outlineform stroke grestore}if formwidth 0
translate userdict /currentform currentform 1 add put currentform
columns mod 0 eq{columns formwidth mul neg formheight neg translate}if
currentform formsperpage mod 0 eq{gsave showpage grestore currentform
columns mod formwidth mul neg formsperpage columns idiv formheight mul
translate userdict /currentform 0 put}if}bind def /outlineform{newpath
xcorner ycorner moveto formwidth 0 rlineto 0 formheight rlineto
formwidth neg 0 rlineto closepath}bind def /lastpage{formsperpage 1 gt{
currentform 0 ne{0 1 formsperpage currentform sub formsperpage mod{pop
showpage}for}if}if}def userdict /end-hook{lastpage}put newpath}def
