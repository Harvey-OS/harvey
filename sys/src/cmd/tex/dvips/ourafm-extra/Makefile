#
# Makefile for resident fonts
# uses the naming scheme in Karl Berry's TUGboat article, 
# with the following exceptions:
#   (r)pstrni  StoneInformal-Italic
#   (r)pstrbi  StoneInformal-BoldItalic
#   (r)pstrsi  StoneInformal-SemiboldItalic
#   (r)pstrso  StoneSans-Oblique
#   rpadrxi    AGaramondExp-Italic
#   (r)padrai  AGaramondAlt-Italic
#   rpadsxi    AGaramondExp-SemiboldItalic
#   rpadbxi    AGaramondExp-BoldItalic
#

# Resident tfm files
RESIDS = rpagk.tfm rpagko.tfm rpagd.tfm rpagdo.tfm \
	rpbkd.tfm rpbkdi.tfm rpbkl.tfm rpbkli.tfm \
	rpcrb.tfm rpcrbo.tfm rpcrro.tfm rpcrr.tfm \
	rphvb.tfm rphvbo.tfm rphvro.tfm rphvr.tfm \
	rphvbrn.tfm rphvbon.tfm rphvron.tfm rphvrrn.tfm \
	rpncb.tfm rpncbi.tfm rpncri.tfm rpncr.tfm \
	rpplb.tfm rpplbi.tfm rpplbu.tfm rpplrrn.tfm rpplrre.tfm \
	rpplri.tfm rpplr.tfm rpplro.tfm rpplru.tfm \
	rpsyr.tfm rpsyro.tfm \
	rptmb.tfm rptmbi.tfm rptmrrn.tfm rptmrre.tfm \
	rptmri.tfm rptmro.tfm rptmr.tfm \
	rpzcmi.tfm rpzdr.tfm \
	psyr.tfm psyro.tfm pzdr.tfm \
	rpstrn.tfm rpstrni.tfm rpstbn.tfm rpstbni.tfm \
	rpstsn.tfm rpstsni.tfm rpstrnn.tfm \
	rpstrs.tfm rpstrso.tfm \
	rpstr.tfm rpstri.tfm rpsts.tfm rpstsi.tfm rpstb.tfm rpstbi.tfm \
	rpopr.tfm rpopro.tfm rpopb.tfm rpopbo.tfm rpoprrn.tfm rpoprre.tfm \
        rpopra.tfm rpopba.tfm \
	rpadr.tfm rpadrx.tfm rpadra.tfm \
	rpadri.tfm rpadrxi.tfm rpadrai.tfm \
	rpads.tfm rpadsx.tfm rpadsi.tfm rpadsxi.tfm \
	rpadb.tfm rpadbx.tfm rpadbi.tfm rpadbxi.tfm

# Virtual font (vf) files
VFS =	pagk.vf pagko.vf pagd.vf pagdo.vf pagkc.vf \
	pbkd.vf pbkdi.vf pbkl.vf pbkli.vf pbklc.vf \
	pcrb.vf pcrbo.vf pcrro.vf pcrr.vf \
	phvb.vf phvbo.vf phvro.vf phvr.vf phvrc.vf \
	phvbrn.vf phvbon.vf phvron.vf phvrrn.vf \
	pncb.vf pncbi.vf pncri.vf pncr.vf pncrc.vf \
	pplb.vf pplbi.vf pplbu.vf pplrrn.vf pplrre.vf \
	pplri.vf pplr.vf pplro.vf pplru.vf pplrc.vf \
	ptmb.vf ptmbi.vf ptmrrn.vf ptmrre.vf \
	ptmri.vf ptmro.vf ptmr.vf ptmrc.vf \
	pzcmi.vf \
	pstrn.vf pstrni.vf pstbn.vf pstbni.vf \
	pstsn.vf pstsni.vf pstrnn.vf \
	pstrs.vf pstrso.vf \
	pstr.vf pstri.vf psts.vf pstsi.vf pstb.vf pstbi.vf \
	popr.vf popro.vf popb.vf popbo.vf poprrn.vf poprre.vf poprc.vf \
        popra.vf popba.vf \
	padr.vf padra.vf padri.vf padrai.vf \
        padb.vf padbi.vf pads.vf padsi.vf

all:    $(RESIDS) $(VFS)

.SUFFIXES:	.vf .vpl .afm

.vpl.vf:
	vptovf $*.vpl $*.vf $*.tfm

rpagk.tfm pagk.vpl:
	afm2tfm AvantGarde-Book -v pagk  rpagk
pagkc.vpl:
	afm2tfm AvantGarde-Book -V pagkc  rpagk
rpagko.tfm pagko.vpl:
	afm2tfm AvantGarde-BookOblique -v pagko  rpagko
rpagd.tfm pagd.vpl:
	afm2tfm AvantGarde-Demi -v pagd  rpagd
rpagdo.tfm pagdo.vpl:
	afm2tfm AvantGarde-DemiOblique -v pagdo  rpagdo
rpbkd.tfm pbkd.vpl:
	afm2tfm Bookman-Demi -v pbkd  rpbkd
rpbkdi.tfm pbkdi.vpl:
	afm2tfm Bookman-DemiItalic -v pbkdi  rpbkdi
rpbkl.tfm pbkl.vpl:
	afm2tfm Bookman-Light -v pbkl  rpbkl
pbklc.vpl:
	afm2tfm Bookman-Light -V pbklc.vpl  rpbkl
rpbkli.tfm pbkli.vpl:
	afm2tfm Bookman-LightItalic -v pbkli  rpbkli
rpcrb.tfm pcrb.vpl:
	afm2tfm Courier-Bold -v pcrb  rpcrb
rpcrbo.tfm pcrbo.vpl:
	afm2tfm Courier-BoldOblique -v pcrbo  rpcrbo
rpcrro.tfm pcrro.vpl:
	afm2tfm Courier-Oblique -v pcrro  rpcrro
rpcrr.tfm pcrr.vpl:
	afm2tfm Courier -v pcrr  rpcrr
rphvb.tfm phvb.vpl:
	afm2tfm Helvetica-Bold -v phvb  rphvb
rphvbo.tfm phvbo.vpl:
	afm2tfm Helvetica-BoldOblique -v phvbo  rphvbo
rphvbrn.tfm phvbrn.vpl:
	afm2tfm Helvetica-Bold -e .82 -v phvbrn  rphvbrn
rphvbon.tfm phvbon.vpl:
	afm2tfm Helvetica-BoldOblique -e .82 -v phvbon  rphvbon
rphvron.tfm phvron.vpl:
	afm2tfm Helvetica-Oblique -e .82 -v phvron  rphvron
rphvrrn.tfm phvrrn.vpl:
	afm2tfm Helvetica -e .82 -v phvrrn  rphvrrn
rphvro.tfm phvro.vpl:
	afm2tfm Helvetica-Oblique -v phvro  rphvro
rphvr.tfm phvr.vpl:
	afm2tfm Helvetica -v phvr  rphvr
phvrc.vpl:
	afm2tfm Helvetica -V phvrc  rphvr
rpncb.tfm pncb.vpl:
	afm2tfm NewCenturySchlbk-Bold -v pncb  rpncb
rpncbi.tfm pncbi.vpl:
	afm2tfm NewCenturySchlbk-BoldItalic -v pncbi  rpncbi
rpncri.tfm pncri.vpl:
	afm2tfm NewCenturySchlbk-Italic -v pncri  rpncri
rpncr.tfm pncr.vpl:
	afm2tfm NewCenturySchlbk-Roman -v pncr  rpncr
pncrc.vpl:
	afm2tfm NewCenturySchlbk-Roman -V pncrc  rpncr
rpplb.tfm pplb.vpl:
	afm2tfm Palatino-Bold -v pplb  rpplb
rpplbi.tfm pplbi.vpl:
	afm2tfm Palatino-BoldItalic -v pplbi  rpplbi
rpplbu.tfm pplbu.vpl:
	afm2tfm Palatino-BoldItalic -s -.1763 -v pplbu  rpplbu
rpplrrn.tfm pplrrn.vpl:
	afm2tfm Palatino-Roman -e .82 -v pplrrn  rpplrrn
rpplrre.tfm pplrre.vpl:
	afm2tfm Palatino-Roman -e 1.2 -v pplrre  rpplrre
rpplri.tfm pplri.vpl:
	afm2tfm Palatino-Italic -v pplri  rpplri
rpplro.tfm pplro.vpl:
	afm2tfm Palatino-Roman -s .167 -v pplro  rpplro
rpplr.tfm pplr.vpl:
	afm2tfm Palatino-Roman -v pplr  rpplr
rpplru.tfm pplru.vpl:
	afm2tfm Palatino-Italic -s -.1763 -v pplru  rpplru
pplrc.vpl:
	afm2tfm Palatino-Roman -V pplrc  rpplr
rpsyr.tfm psyr.tfm:
	afm2tfm Symbol  rpsyr
	cp rpsyr.tfm psyr.tfm
rpsyro.tfm psyro.tfm:
	afm2tfm Symbol -s .167  rpsyro
	cp rpsyro.tfm psyro.tfm
rptmb.tfm ptmb.vpl:
	afm2tfm Times-Bold -v ptmb  rptmb
rptmbi.tfm ptmbi.vpl:
	afm2tfm Times-BoldItalic -v ptmbi  rptmbi
rptmrrn.tfm ptmrrn.vpl:
	afm2tfm Times-Roman -e .8 -v ptmrrn  rptmrrn
rptmrre.tfm ptmrre.vpl:
	afm2tfm Times-Roman -e 1.2 -v ptmrre  rptmrre
rptmri.tfm ptmri.vpl:
	afm2tfm Times-Italic -v ptmri  rptmri
rptmro.tfm ptmro_raw.vpl:
	afm2tfm Times-Roman -s .167 -v ptmro_raw  rptmro
ptmro.vpl: ptmro_raw.vpl
	ed ptmro_raw.vpl <ptmro.ed
rptmr.tfm ptmr_raw.vpl:
	afm2tfm Times-Roman -v ptmr_raw  rptmr
ptmr.vpl: ptmr_raw.vpl
	ed ptmr_raw.vpl <ptmr.ed
ptmrc.vpl:
	afm2tfm Times-Roman -V ptmrc.vpl  rptmr
rpzcmi.tfm pzcmi.vpl:
	afm2tfm ZapfChancery-MediumItalic -v pzcmi  rpzcmi
rpzdr.tfm pzdr.tfm:
	afm2tfm ZapfDingbats  rpzdr
	cp rpzdr.tfm pzdr.tfm

rpstrn.tfm pstrn.vpl:
	afm2tfm StoneInformal -v pstrn  rpstrn
rpstrnn.tfm pstrnn.vpl:
	afm2tfm StoneInformal -e .8 -v pstrnn  rpstrnn
rpstrni.tfm pstrni.vpl:
	afm2tfm StoneInformal-Italic -v pstrni  rpstrni
rpstbn.tfm pstbn.vpl:
	afm2tfm StoneInformal-Bold -v pstbn  rpstbn
rpstbni.tfm pstbni.vpl:
	afm2tfm StoneInformal-BoldItalic -v pstbni  rpstbni
rpstsn.tfm pstsn.vpl:
	afm2tfm StoneInformal-Semibold -v pstsn  rpstsn
rpstsni.tfm pstsni.vpl:
	afm2tfm StoneInformal-SemiboldItalic -v pstsni  rpstsni
rpstr.tfm pstr.vpl:
	afm2tfm StoneSerif -v pstr  rpstr
rpstri.tfm pstri.vpl:
	afm2tfm StoneSerif-Italic -v pstri  rpstri
rpstb.tfm pstb.vpl:
	afm2tfm StoneSerif-Bold -v pstb  rpstb
rpstbi.tfm pstbi.vpl:
	afm2tfm StoneSerif-BoldItalic -v pstbi  rpstbi
rpsts.tfm psts.vpl:
	afm2tfm StoneSerif-Semibold -v psts  rpsts
rpstsi.tfm pstsi.vpl:
	afm2tfm StoneSerif-SemiboldItalic -v pstsi  rpstsi
rpstrs.tfm pstrs.vpl:
	afm2tfm StoneSans -v pstrs  rpstrs
rpstrso.tfm pstrso.vpl:
	afm2tfm StoneSans -s .2126 -v pstrso  rpstrso
rpopr.tfm popr.vpl:
	afm2tfm Optima -v popr  rpopr
rpopb.tfm popb.vpl:
	afm2tfm Optima-Bold -v popb  rpopb
rpopbo.tfm popbo.vpl:
	afm2tfm Optima-Bold -s .1944 -v popbo  rpopbo
rpoprrn.tfm poprrn.vpl:
	afm2tfm Optima -e .82 -v poprrn  rpoprrn
rpoprre.tfm poprre.vpl:
	afm2tfm Optima -e 1.2 -v poprre  rpoprre
rpopro.tfm popro.vpl:
	afm2tfm Optima -s .1944 -v popro  rpopro
poprc.vpl:
	afm2tfm Optima -V poprc  rpopr
rpopra.tfm popra.vpl:
	afm2tfm Optima -v popra  rpopra
rpopba.tfm popba.vpl:
	afm2tfm Optima-Bold -v popba  rpopba
rpadr.tfm padr_raw.vpl:
	afm2tfm AGaramond-Regular -v padr_raw  rpadr
rpadrx.tfm:
	afm2tfm AGaramondExp-Regular  rpadrx
rpadra.tfm:
	afm2tfm AGaramondAlt-Regular  rpadra
padr.vpl: padr_raw.vpl
	ed padr_raw.vpl <padr.ed
padra.vpl: padr.vpl
	ed padr.vpl <padra.ed
rpadri.tfm padri_raw.vpl:
	afm2tfm AGaramond-Italic -v padri_raw  rpadri
rpadrxi.tfm:
	afm2tfm AGaramondExp-Italic  rpadrxi
rpadrai.tfm:
	afm2tfm AGaramondAlt-Italic  rpadrai
padri.vpl: padri_raw.vpl
	ed padri_raw.vpl <padri.ed
padrai.vpl: padri.vpl
	ed padri.vpl <padrai.ed
rpads.tfm pads_raw.vpl:
	afm2tfm AGaramond-Semibold -v pads_raw  rpads
rpadsx.tfm:
	afm2tfm AGaramondExp-Semibold  rpadsx
pads.vpl: pads_raw.vpl
	ed pads_raw.vpl <pads.ed
rpadsi.tfm padsi_raw.vpl:
	afm2tfm AGaramond-SemiboldItalic -v padsi_raw  rpadsi
rpadsxi.tfm:
	afm2tfm AGaramondExp-SemiboldItalic  rpadsxi
padsi.vpl: padsi_raw.vpl
	ed padsi_raw.vpl <padsi.ed
rpadb.tfm padb_raw.vpl:
	afm2tfm AGaramond-Bold -v padb_raw  rpadb
rpadbx.tfm:
	afm2tfm AGaramondExp-Bold  rpadbx
padb.vpl: padb_raw.vpl
	ed padb_raw.vpl <padb.ed
rpadbi.tfm padbi_raw.vpl:
	afm2tfm AGaramond-BoldItalic -v padbi_raw  rpadbi
rpadbxi.tfm:
	afm2tfm AGaramondExp-BoldItalic  rpadbxi
padbi.vpl: padbi_raw.vpl
	ed padbi_raw.vpl <padbi.ed


install:
	mv *.tfm ../PStfms
	mv *.vf ../PSvfs

clean:
	-rm -f *.vpl

veryclean:
	-rm -f *.vpl *.tfm *.vf
