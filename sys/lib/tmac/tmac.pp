.fp 1 E
.fp 2 EI
.fp 3 EB
'tr *\(**
'ie t 'ds _ \d\(mi\u
'el 'ds _ _
'ps 10p
'vs 10p
'ds - \(mi
'ds /* \\h'\\w' 'u-\\w'/'u'/*
'bd S 3
'nr cm 0
'nf
'wh 0 he
'de he
'ev 2
'tl '\-\-''\-\-'
'ft 1
'sp .35i
'tl '\s14\f3\\*(=F\fP\s0'\\*(=H'\f3\s14\\*(=F\fP\s0'
'sp .25i
'ft 1
\f2\s12\h'\\n(.lu-\w'\\*(=f'u'\\*(=f\fP\s0\h'|0u'
.sp .05i
'ev
'ds =G \\*(=F
..
'wh -1i fo
'de fo
'ev 2
'sp .35i
'tl '\f2\\*(=M''Page % of \\*(=G\fP'
'bp
'ev
'ft 1
'if \\n(cm=1 'ft 2
..
'de ()
'pn 1
..
'de +C
'nr cm 1
'ft 2
'ds +K
'ds -K
..
'de -C
'nr cm 0
'ft 1
'ie t 'ds +K \f3
'el 'ds +K \f2
'ds -K \fP
..
'+C
'-C
'am +C
'ne 3
..
'de FN
\f2\s14\h'\\n(.lu-\w'\\$1'u'\\$1\fP\s0\h'|0u'\c
.if \\nx .tm \\$1 \\*(=F \\n%
\\$1\c
'ds =f \&...\\$1
..
'de -F
'rm =f
..
'ft 1
'lg 0
