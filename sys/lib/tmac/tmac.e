.nr _0 \n(.c
.\"**********************************************************************
.\"*									*
.\"*	******  - M E   N R O F F / T R O F F   M A C R O S  ******	*
.\"*									*
.\"*	Produced for your edification and enjoyment by:			*
.\"*		Eric Allman						*
.\"*		Electronics Research Laboratory				*
.\"*		U.C. Berkeley.						*
.\"*									*
.\"*	VERSION 2.9	First Release: 11 Sept 1978			*
.\"*	See file \*(||/revisions for revision history			*
.\"*									*
.\"*	Documentation is available.					*
.\"*									*
.\"**********************************************************************
.\"
.\"	@(#)tmac.e	2.9	12/10/80
.\" This version has had comments stripped; an unstripped version is available.
.if !\n(.V .tm You are using the wrong version of NROFF/TROFF!!
.if !\n(.V .tm This macro package works only on the version seven
.if !\n(.V .tm release of NROFF and TROFF.
.if !\n(.V .ex
.if \n(pf \
.	nx \*(||/null.me
.de @C
.nr _S \\n(.s
.nr _V \\n(.v
.nr _F \\n(.f
.nr _I \\n(.i
.ev \\$1
.ps \\n(_Su
.vs \\n(_Vu
.ft \\n(_F
'in \\n(_Iu
.xl \\n($lu
.lt \\n($lu
.rr _S
.rr _V
.rr _F
.rr _I
.ls 1
'ce 0
..
.de @D
.ds |p "\\$3
.nr _d \\$1
.ie "\\$2"C" \
.	nr _d 1
.el .ie "\\$2"L" \
.	nr _d 2
.el .ie "\\$2"I" \
.	nr _d 3
.el .ie "\\$2"M" \
.	nr _d 4
.el \
.	ds |p "\\$2
..
.de @z
.if !"\\n(.z"" \
\{\
.	tm Line \\n(c. -- Unclosed block, footnote, or other diversion (\\n(.z)
.	di
.	ex
.\}
.if \\n(?a \
.	bp
.rm bp
.rm @b
.if t \
.	wh -1p @m
.br
..
.de @I
.rm th
.rm ac
.rm lo
.rm sc
.rm @I
..
.de he
.ie !\\n(.$ \
\{\
.	rm |4
.	rm |5
.\}
.el \
\{\
.	ds |4 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
.	ds |5 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
.\}
..
.de eh
.ie !\\n(.$ \
.	rm |4
.el \
.	ds |4 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.de oh
.ie !\\n(.$ \
.	rm |5
.el \
.	ds |5 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.de fo
.ie !\\n(.$ \
\{\
.	rm |6
.	rm |7
.\}
.el \
\{\
.	ds |6 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
.	ds |7 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
.\}
..
.de ef
.ie !\\n(.$ \
.	rm |6
.el \
.	ds |6 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.de of
.ie !\\n(.$ \
.	rm |7
.el \
.	ds |7 "\\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.de ep
.if \\n(nl>0 \
\{\
.	wh 0
.	rs
.	@b
.\}
..
.de @h
.if (\\n(.i+\\n(.o)>=\\n(.l \
.	tm Line \\n(c. -- Offset + indent exceeds line length
.if t .if (\\n(.l+\\n(.o)>7.75i \
.	tm Line \\n(c. -- Offset + line length exceeds paper width
.nr ?h \\n(?H
.rr ?H
.nr ?c \\n(?C
.rr ?C
.rn |4 |0
.rn |5 |1
.rn |6 |2
.rn |7 |3
.nr _w 0
.nr ?W 0
.nr ?I 1
.ev 2
.rs
.if t .@m
.if \\n(hm>0 \
.	sp |\\n(hmu
.if \\n($T=2 \\!.
.@t $h
.if \\n(tm<=0 \
.	nr tm \n(.Vu
.sp |\\n(tmu
.ev
.mk _k
.if \\n(?n .nm 1
.nr $c 1
.ie \\n(?s \
\{\
.	rr ?s
.	rs
'	@b
.\}
.el \
.	@n
..
.de @m
.@O 0
.lt 7.5i
.tl '\(rn''\(rn'
.@O
.lt
..
.de @n
.if \\n(bm<=0 \
.	nr bm \\n(.Vu
.if (\\n(_w<=\\n($l)&(\\n(?W=0) \
\{\
.	nr _b (\\n(ppu*\\n($ru)/2u
.	if \\n(_bu>((\\n(bmu-\\n(fmu-(\\n(tpu*\\n($ru))/2u) \
.		nr _b (\\n(ppu*\\n($ru)-\n(.Vu
.	nr _b +\\n(bmu
.\}
.nr _B \\n(_bu
.ch @f
.wh -\\n(_bu @f
.nr ?f 0
.if \\n(?o \
\{\
.	(f _
.	nf
.	|o
.	fi
.	)f
.	rm |o
.\}
.nr ?o 0
.if \\n(?T \
\{\
.	nr _i \\n(.i
.	in \\n($iu
.	|h
.	in \\n(_iu
.	rr _i
.	mk #T
.	ns
.\}
.if (\\n(?a)&((\\n($c<2):(\\n(?w=0)) \
\{\
.	nr ?a 0
.	@k |t
.	if \\n(?w \
.		mk _k
.	nr ?w 0
.\}
.os
.$H
.ns
..
.de @f
.ec
.if \\n(?T \
\{\
.	nr T. 1
.	T# 1
.	br
.\}
.ev 2
.ce 0
.if \\n(?b \
\{\
.	nr ?b 0
.	@k |b
.\}
.if \\n(?f \
.	@o
.ie \\n($c<\\n($m \
.	@c
.el \
.	@e
.ev
..
.de @o
.nf
.ls 1
.in 0
.wh -\\n(_Bu @r
.|f
.fi
.if \\n(?o \
.	di
.	if \\n(dn=0 \
\{\
.		rm |o
.		nr ?o 0
.	\}
.	nr dn \\n(_D
.	rr _D
.\}
.rm |f
.ch @r
..
.de @c
.rs
.sp |\\n(_ku
.@O +\\n($lu+\\n($su
.nr $c +1
.@n
..
.de @e
.@O \\n(_ou
.rs
.sp |\\n(.pu-\\n(fmu-(\\n(tpu*\\n($ru)
.@t $f
.nr ?h 0
.bp
..
.de @t
.if !\\n(?h \
\{\
.	sz \\n(tp
.	@F \\n(tf
.	lt \\n(_Lu
.	nf
.	\\$1
.	br
.\}
..
.de $h
.rm |z
.if !\\n(?c \
\{\
.	if e .ds |z "\\*(|0
.	if o .ds |z "\\*(|1
.\}
.if !\(ts\\*(|z\(ts\(ts \
'	tl \\*(|z
.rm |z
..
.de $f
.rm |z
.if \\n(?c \
\{\
.	if e .ds |z "\\*(|0
.	if o .ds |z "\\*(|1
.\}
.if \(ts\\*(|z\(ts\(ts \
\{\
.	if e .ds |z "\\*(|2
.	if o .ds |z "\\*(|3
.\}
.if !\(ts\\*(|z\(ts\(ts \
'	tl \\*(|z
.rm |z
..
.de @r
.di |o
.nr ?o 1
.nr _D \\n(dn
.ns
..
.rn bp @b
.de bp
.nr $c \\n($m
.ie \\n(nl>0 \
.	@b \\$1
.el \
\{\
.	if \\n(.$>0 \
.		pn \\$1
.	if \\n(?I \
.		@h
.\}
.br
.wh 0 @h
..
.rn ll xl
.de ll
.xl \\$1
.lt \\$1
.nr $l \\n(.l
.if (\\n($m<=1):(\\n($l>\\n(_L) \
.	nr _L \\n(.l
..
.rn po @O
.de po
.@O \\$1
.nr _o \\n(.o
..
.de hx
.nr ?H 1
..
.de ix
'in \\$1
..
.de bl
.br
.ne \\$1
.rs
.sp \\$1
..
.de n1
.nm 1
.xl -\w'0000'u
.nr ?n 1
..
.de n2
.nm \\$1
.ie \\n(.$ \
.	xl -\w'0000'u
.el \
.	xl \\n($lu
..
.de pa
.bp \\$1
..
.de ro
.af % i
..
.de ar
.af % 1
..
.de m1
.nr _0 \\n(hmu
.nr hm \\$1v
.nr tm +\\n(hmu-\\n(_0u
.rr _0
..
.de m2
.nr tm \\n(hmu+\\n(tpp+\\$1v
..
.de m3
.nr bm \\n(fmu+\\n(tpp+\\$1v
..
.de m4
.nr _0 \\n(fmu
.nr fm \\$1v
.nr bm +\\n(fmu-\\n(_0u
..
.de sk
.if \\n(.$>0 \
.	tm Line \\n(c. -- I cannot skip multiple pages
.nr ?s 1
..
.de re
.ta 0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i +0.5i
..
.if t .ig
.de re
.ta 0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i +0.8i
..
.de ba
.ie \\n(.$ \
.	nr $i \\$1n
.el \
.	nr $i \\n(siu*\\n($0u
..
.de hl
.br
\l'\\n(.lu-\\n(.iu'
.sp
..
.de pp
.lp \\n(piu
..
.de lp
.@p
.if \\n(.$ \
.	ti +\\$1
.nr $p 0 1
..
.de ip
.if (\\n(ii>0)&(\\n(ii<1n) \
.	nr ii \\n(iin
.nr _0 \\n(ii
.if \\n(.$>1 \
.	nr _0 \\$2n
.@p \\n(_0u
.if \\w"\\$1" \
\{\
.	ti -\\n(_0u
.	ie \\w"\\$1">=\\n(_0 \
\{\
\&\\$1
.		br
.	\}
.	el \&\\$1\h'|\\n(_0u'\c
.\}
.rr _0
..
.de np
.nr $p +1
.ip (\\n($p)
..
.de @p
.@I
.if "\\n(.z"|e" .tm Line \\n(c. -- Unmatched continued equation
.in \\n($iu+\\n(pou
.if \\n(.$ \
.	in +\\$1n
.ce 0
.fi
.@F \\n(pf
.sz \\n(ppu
.sp \\n(psu
.ne \\n(.Lv+\\n(.Vu
.ns
..
.de sh
.rn sh @T
.so \\*(||/sh.me
.sh "\\$1" "\\$2" \\$3 \\$4 \\$5 \\$6 \\$7 \\$8
.rm @T
..
.de $p
.if (\\n(si>0)&(\\n(.$>2) \
.	nr $i \\$3*\\n(si
.in \\n($iu
.ie !"\\$1\\$2"" \
\{\
.	sp \\n(ssu
.	ne \\n(.Lv+\\n(.Vu+\\n(psu+(\\n(spu*\\n($ru*\\n(.Lu)
.	ie \\n(.$>2 \
.		ti -(\\n(siu-\\n(sou)
.	el \
.		ti +\\n(sou
.	@F \\n(sf
.	sz \\n(spu
.	if \\$3>0 \
.		$\\$3
.	if \w"\\$2">0 \\$2.
.	if \w"\\$1">0 \\$1\f1\ \  \"
.\}
.el \
.	sp \\n(psu
.@F \\n(pf
.sz \\n(ppu
..
.de uh
.rn uh @T
.so \\*(||/sh.me
.uh "\\$1"
.rm @T
..
.de 2c
.br
.if \\n($m>1 \
.	1c
.nr $c 1
.nr $m 2
.if \\n(.$>1 \
.	nr $m \\$2
.if \\n(.$>0 \
.	nr $s \\$1n
.nr $l (\\n(.l-((\\n($m-1)*\\n($s))/\\n($m
.xl \\n($lu
.mk _k
.ns
..
.de 1c
.br
.nr $c 1
.nr $m 1
.ll \\n(_Lu
.sp |\\n(.hu
.@O \\n(_ou
..
.de bc
.sp 24i
..
.de (z
.rn (z @V
.so \\*(||/float.me
.(z \\$1 \\$2
.rm @V
..
.de )z
.tm Line \\n(c. -- unmatched .)z
..
.de (t
.(z \\$1 \\$2
..
.de )t
.)z \\$1 \\$2
..
.de (b
.br
.@D 3 \\$1 \\$2
.sp \\n(bsu
.@(
..
.de )b
.br
.@)
.if (\\n(bt=0):(\\n(.t<\\n(bt) \
.	ne \\n(dnu
.ls 1
.nf
.|k
.ec
.fi
.in 0
.xl \\n($lu
.ev
.rm |k
.sp \\n(bsu+\\n(.Lv-1v
..
.de @(
.if !"\\n(.z"" .tm Line \\n(c. -- Illegal nested keep \\n(.z
.@M
.di |k
\!'rs
..
.de @M
.nr ?k 1
.@C 1
.@F \\n(df
.vs \\n(.su*\\n($Ru
.nf
.if "\\*(|p"F" \
.	fi
.if \\n(_d=4 \
.	in 0
.if \\n(_d=3 \
\{\
.	in +\\n(biu
.	xl -\\n(biu
.\}
.if \\n(_d=1 \
.	ce 10000
..
.de @)
.br
.if !"\\n(.z"|k" .tm Line \\n(c. -- Close of a keep which has never been opened
.nr ?k 0
.di
.in 0
.ce 0
..
.de (c
.if "\\n(.z"|c" .tm Line \\n(c. -- Nested .(c requests
.di |c
..
.de )c
.if !"\\n(.z"|c" .tm Line \\n(c. -- Unmatched .)c
.br
.di
.ev 1
.ls 1
.in (\\n(.lu-\\n(.iu-\\n(dlu)/2u
.nf
.|c
.ec
.in
.ls
.ev
.rm |c
..
.de (q
.br
.@C 1
.fi
.sp \\n(qsu
.in +\\n(qiu
.xl -\\n(qiu
.sz \\n(qp
..
.de )q
.br
.ev
.sp \\n(qsu+\\n(.Lv-1v
.nr ?k 0
..
.de (l
.br
.sp \\n(bsu
.@D 3 \\$1 \\$2
.@M
..
.de )l
.br
.ev
.sp \\n(bsu+\\n(.Lv-1v
.nr ?k 0
..
.de EQ
.rn EQ @T
.so \\*(||/eqn.me
.EQ \\$1 \\$2
.rm @T
..
.de TS
.rn TS @W
.so \\*(||/tbl.me
.TS \\$1 \\$2
.rm @W
..
.de sz
.ps \\$1
.vs \\n(.su*\\n($ru
.bd S B \\n(.su/3u
..
.de r
.nr _F \\n(.f
.ul 0
.ft 1
.if \\n(.$ \&\\$1\f\\n(_F\\$2
.rr _F
..
.de i
.nr _F \\n(.f
.ul 0
.ft 2
.if \\n(.$ \&\\$1\f\\n(_F\\$2
.rr _F
..
.de b
.nr _F \\n(.f
.ul 0
.ie t \
.	ft 3
.el \
.	ul 10000
.if \\n(.$ \&\\$1\f\\n(_F\\$2
.if \\n(.$ \
.	ul 0
.rr _F
..
.de rb
.nr _F \\n(.f
.ul 0
.ft 3
.if \\n(.$ \&\\$1\f\\n(_F\\$2
.rr _F
..
.de u
\&\\$1\l'|0\(ul'\\$2
..
.de q
\&\\*(lq\\$1\\*(rq\\$2
..
.de bi
.ft 2
.ie t \&\k~\\$1\h'|\\n~u+(\\n(.su/3u)'\\$1\fP\\$2
.el \&\\$1\fP\\$2
..
.de bx
.ie \\n($T \&\f2\\$1\fP\\$2
.el \k~\(br\|\\$1\|\(br\l'|\\n~u\(rn'\l'|\\n~u\(ul'\^\\$2
..
.de @F
.nr ~ \\$1
.if \\n~>0 \
\{\
.	ul 0
.	ie \\n~>4 \
\{\
.		if n .ul 10000
.		if t .ft 3
.	\}
.	el \
.		ft \\n~
.\}
.rr ~
..
.de (f
.rn (f @U
.so \\*(||/footnote.me
.(f \\$1 \\$2
.rm @U
..
.de )f
.tm Line \\n(c. -- unmatched .)f
..
.de $s
\l'2i'
.if n \
.	sp 0.3
..
.de (d
.rn (d @U
.so \\*(||/deltext.me
.(d \\$1 \\$2
.rm @U
..
.de )d
.tm Line \\n(c. -- unmatched .)d
..
.de (x
.rn (x @U
.so \\*(||/index.me
.(x \\$1 \\$2
.rm @U
..
.de )x
.tm Line \\n(c. -- unmatched .)x
..
.de th
.so \\*(||/thesis.me
.rm th
..
.de +c
.ep
.if \\n(?o:\\n(?a \
\{\
.	bp
.	rs
.	ep
.\}
.nr ?C 1
.nr $f 1 1
.ds * \\*[1\\*]\k*
.if \\n(?R \
.	pn 1
.bp
.in \\n($iu
.rs
.ie \\n(.$ \
.	$c "\\$1"
.el \
.	sp 3
..
.de ++
.nr _0 0
.if "\\$1"C" \
.	nr _0 1
.if "\\$1"RC" \
.	nr _0 11
.if "\\$1"A" \
.	nr _0 2
.if "\\$1"RA" \
.	nr _0 12
.if "\\$1"P" \
.	nr _0 3
.if "\\$1"B" \
.	nr _0 4
.if "\\$1"AB" \
.	nr _0 5
.if \\n(_0=0 \
.	tm Line \\n(c. -- Bad mode to .++
.nr ?R 0
.if \\n(_0>10 \
.\{
.	nr ?R 1
.	nr _0 -10
.\}
.nr ch 0 1
.if (\\n(_0=3):(\\n(_0=5) \
.	pn 1
.ep
.if \\n(_0=1 \
\{\
.	af ch 1
.	af % 1
.\}
.if \\n(_0=2 \
\{\
.	af ch A
.	af % 1
.\}
.if \\n(_0=3 \
.	af % i
.if \\n(_0=4 \
.	af % 1
.if \\n(_0=5 \
.	af % 1
.if \\n(.$>1 \
.	he \\$2
.if !\\n(_0=\\n(_M .if \\n(_M=3 \
.	pn 1
.nr _M \\n(_0
.rr _0
..
.de $c
.sz 12
.ft B
.ce 1000
.if \\n(_M<3 \
.	nr ch +1
.ie \\n(_M=1 CHAPTER\ \ \\n(ch
.el .if \\n(_M=2 APPENDIX\ \ \\n(ch
.if \w"\\$1" .sp 3-\\n(.L
.if \w"\\$1" \\$1
.if (\\n(_M<3):(\w"\\$1") \
.	sp 4-\\n(.L
.ce 0
.ft
.sz
.ie \\n(_M=1 \
.	$C Chapter \\n(ch "\\$1"
.el .if \\n(_M=2 \
.	$C Appendix \\n(ch "\\$1"
..
.de tp
.hx
.bp
.br
.rs
.pn \\n%
..
.de ac
.rn ac @T
.so \\*(||/acm.me
.ac "\\$1" "\\$2"
.rm @T
..
.de lo
.so \\*(||/local.me
.rm lo
..
.if \n(mo=1 .ds mo January
.if \n(mo=2 .ds mo February
.if \n(mo=3 .ds mo March
.if \n(mo=4 .ds mo April
.if \n(mo=5 .ds mo May
.if \n(mo=6 .ds mo June
.if \n(mo=7 .ds mo July
.if \n(mo=8 .ds mo August
.if \n(mo=9 .ds mo September
.if \n(mo=10 .ds mo October
.if \n(mo=11 .ds mo November
.if \n(mo=12 .ds mo December
.if \n(dw=1 .ds dw Sunday
.if \n(dw=2 .ds dw Monday
.if \n(dw=3 .ds dw Tuesday
.if \n(dw=4 .ds dw Wednesday
.if \n(dw=5 .ds dw Thursday
.if \n(dw=6 .ds dw Friday
.if \n(dw=7 .ds dw Saturday
.ds td \*(mo \n(dy, 19\n(yr
.if (1m<0.1i)&(\nx!=0) \
.	vs 9p
.rr x
.nr $r \n(.v/\n(.s
.nr $R \n($r
.nr hm 4v
.nr tm 7v
.nr bm 6v
.nr fm 3v
.nr tf 3
.nr tp 10
.hy 14
.nr bi 4n
.nr pi 5n
.nr pf 1
.nr pp 10
.nr qi 4n
.nr qp -1
.nr ii 5n
.nr $m 1
.nr $s 4n
.ds || /sys/lib/tmac/me
.bd S B 3
.ds [ \u\x'-0.25v'
.ds ] \d
.ds < \d\x'0.25v'
.ds > \u
.ds - --
.if t \
\{\
.	ds [ \v'-0.4m'\x'-0.2m'\s-3
.	ds ] \s0\v'0.4m'
.	ds < \v'0.4m'\x'0.2m'\s-3
.	ds > \s0\v'-0.4m'
.	ds - \-
.	nr fi 0.3i
.\}
.if n \
\{\
.	nr fi 3n
.\}
.nr _o \n(.o
.if n .po 1i
.if \n(.V=1v \
.	nr $T 2
.if \n(.T=0 \
.	nr $T 1
.if t \
\{\
.	nr $T 0
.	po -0.5i
.\}
.if \nv \
.	po 1i
.if \n($T \
\{\
.	if \n($T=1 \
.		po 0
.	ds [ [
.	ds ] ]
.	ds < <
.	ds > >
.\}
.nr ps 0.5v
.if \n($T \
.	nr ps 1v
.if t .nr ps 0.35v
.nr bs \n(ps
.nr qs \n(ps
.nr zs 1v
.nr xs 0.2v
.nr fs 0.2v
.if \n($T \
.	nr fs 0
.if n .nr es 1v
.if t .nr es 0.5v
.wh 0 @h
.nr $l \n(.lu
.nr _L \n(.lu
.nr $c 1
.nr $f 1 1
.ds * \*[1\*]\k*\"
.nr $d 1 1
.ds # [1]\k#\"
.nr _M 1
.ds lq \&"\"
.ds rq \&"\"
.if t \
.	ds lq ``
.if t \
.	ds rq ''
.em @z
.de sc
.so \\*(||/chars.me
.rm sc
..
.ll 6.0i
.lt 6.0i
