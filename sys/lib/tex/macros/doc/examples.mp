beginfig(1);
a=.7in; b=0.5in;
z0=(0,0); z1=(a,0); z2=(0,b);
z0=.5[z1,z3]=.5[z2,z4];
draw z1..z2..z3..z4..cycle;
drawarrow z0..z1;
drawarrow z0..z2;
label.top(btex $a$ etex, .5[z0,z1]);
label.lft(btex $b$ etex, .5[z0,z2]);
endfig;




beginfig(2);

h=2in; w=2.7in;
path p[], q[], pp;
for i=1.5,2,4:
  ii := i**2;
  p[i] = (w/ii,h){1/ii,-1}...(w/i,h/i)...(w,h/ii){1,-1/ii};
endfor
for i=.5,1.5:
  q[i] = origin..(w,i*h) cutafter p1.5;
endfor

pp = buildcycle(q0.5, p2, q1.5, p4);

fill pp withcolor .8white;
z0=center pp;
picture lab; lab=thelabel(btex $f>0$ etex, z0);
unfill bbox lab; draw lab;
draw q0.5; draw p2; draw q1.5; draw p4;
makelabel.top(btex $P$ etex, p2 intersectionpoint q0.5);
makelabel.rt(btex $Q$ etex, p2 intersectionpoint q1.5);
endfig;




beginfig(3);

3.2scf = 2.4in;
path fun;
# = .1;  % Keep the function single-valued
fun = ((0,-1#)..(1,.5#){right}..(1.9,.2#){right}..{curl .1}(3.2,2#))
  scaled scf yscaled(1/#);

vardef vertline primary x = (x,-infinity)..(x,infinity) enddef;
primarydef f atx x = (f intersectionpoint vertline x) enddef;
primarydef f whenx x = xpart(f intersectiontimes vertline x) enddef;

z1a = (2.5scf,0);
z1 = fun atx x1a;
y2a=0; z1-z2a=whatever*direction fun whenx x1 of fun;
z2 = fun atx x2a;
y3a=0; z2-z3a=whatever*direction fun whenx x2 of fun;

draw fun withpen pencircle scaled 1pt;
drawarrow (0,0)..(3.2scf,0);

label.bot(btex $x_1$ etex, z1a);
draw z1a..z1 dashed evenly;
makelabel(nullpicture, z1);
draw z1..z2a withpen pencircle scaled .3;
label.bot(btex $x_2$ etex, z2a);
draw z2a..z2 dashed evenly;
makelabel(nullpicture, z2);
draw z2..z3a withpen pencircle scaled .3;
label.bot(btex $x_3$ etex, z3a);
endfig;



beginfig(4);
for i=0 upto 2:
  z[i]=(0,40i); z[i+3]-z[i]=(100,30);
endfor
pickup pencircle scaled 18;
def gray = withcolor .8white enddef;
draw z0..z3 gray;
linecap:=butt; draw z1..z4 gray;
linecap:=squared; draw z2..z5 gray;
labels.top(0,1,2,3,4,5);
endfig; linecap:=rounded;



beginfig(5);
for i=0 upto 2:
  z[i]=(0,50i); z[i+3]-z[i]=(60,40);
  z[i+6]-z[i]=(120,0);
endfor
pickup pencircle scaled 24;
def gray = withcolor .8white enddef;
draw z0--z3--z6 gray;
linejoin:=mitered; draw z1..z4--z7 gray;
linejoin:=beveled; draw z2..z5--z8 gray;
labels.bot(0,1,2,3,4,5,6,7,8);
endfig; linejoin:=rounded;



input boxes


\beginfig(6);
fill unitsquare xscaled 1.1in yscaled .7in withcolor .9white;
boxit(currentpicture);
dx = dy = .25in;
clearit; drawboxed();
forsuffixes $=n,c: makelabel.top(str $, $); endfor
makelabel.bot("s",s);
forsuffixes $=ne,e,se: makelabel.rt(str $, $); endfor
forsuffixes $=nw,w,sw: makelabel.lft(str $, $); endfor
pickup pencircle scaled .3bp;
vardef larrow@#(expr a, da, s) =
  drawdblarrow a..a+da; label@#(s,a+.5da); enddef;
larrow.rt(n, (0,-dy), "dy");
larrow.rt(s, (0,dy), "dy");
larrow.top(e, (-dx,0), "dx");
larrow.top(w, (dx,0), "dx");
endfig;



beginfig(7);
boxjoin(a.se=b.sw; a.ne=b.nw );
boxit.a(btex $\cdots$ etex);
boxit.ni(btex $n_i$ etex);
boxit.di(btex $d_i$ etex);
boxit.nii(btex $n_{i+1}$ etex);
boxit.dii(btex $d_{i+1}$ etex);
boxit.aa(pic_.a);
boxit.nk(btex $n_k$ etex);
boxit.dk(btex $d_k$ etex);

di.dy = 2;
drawboxed(a,ni,di,nii,dii,aa,nk,dk);
label.lft("ndtable:", a.w);

boxjoin(a.sw=b.nw; a.se=b.ne);
interim defaultdy:=7;
boxit.ba();
boxit.bb();
boxit.bc();
boxit.bd(btex $\vdots$ etex);
boxit.be();
boxit.bf();
bd.dx = 8;
ba.ne = a.sw - (15,10);

drawboxed(ba,bb,bc,bd,be,bf);
label.lft("hashtab:", ba.w);

def ndblock suffix $ =
  boxjoin(a.sw=b.nw; a.se=b.ne);
  forsuffixes $$=$a,$b,$c:
    boxit$$(); ($$dx,$$dy)=(5.5,4);
  endfor;
enddef;

ndblock nda;
ndblock ndb;
ndblock ndc;
nda.a.c - bb.c = ndb.a.c - nda.c.c = (whatever,0);
xpart ndb.c.se = xpart ndc.a.ne = xpart di.c;
ndc.a.c - be.c = (whatever,0);

drawboxes(nda.a,nda.b,nda.c, ndb.a,ndb.b,ndb.c, ndc.a,ndc.b,ndc.c);

drawarrow bb.c .. nda.a.w;
drawarrow be.c .. ndc.a.w;
drawarrow nda.c.c .. ndb.a.w;
drawarrow nda.a.c{right}..{curl0}ni.c cutafter bpath ni;
drawarrow nda.b.c{right}..{curl0}di.c cutafter bpath di;
drawarrow ndc.a.c{right}..{curl0}nii.c cutafter bpath nii;
drawarrow ndc.b.c{right}..{curl0}dii.c cutafter bpath dii;
drawarrow ndb.a.c{right}..nk.c cutafter bpath nk;
drawarrow ndb.b.c{right}..dk.c cutafter bpath dk;

x.ptr = xpart aa.c;
y.ptr = ypart ndc.a.ne;
drawarrow subpath (0,.7) of (z.ptr..{left}ndc.c.c) dashed evenly;
label.rt(btex ndblock etex, z.ptr);
endfig;




\beginfig(8)
interim circmargin := .07in;
fill unitsquare xscaled 1.1in yscaled .7in withcolor .9white;
circleit(currentpicture);
dx = dy;
clearit; drawboxed();
forsuffixes $=n,c: makelabel.top(str $, $); endfor
makelabel.bot("s",s);
makelabel.rt("e", e);
makelabel.lft("w", w);
pickup pencircle scaled .3bp;
vardef larrow@#(expr a, da, s) =
  drawdblarrow a..a+da; label@#(s,a+.5da); enddef;
larrow.rt(n, (0,-dy), "dy");
larrow.rt(s, (0,dy), "dy");
larrow.top(e, (-dx,0), "dx");
larrow.top(w, (dx,0), "dx");
endfig;




beginfig(9);
vardef cuta(suffix a,b) expr p =
  drawarrow p cutbefore bpath.a cutafter bpath.b;
  point .5*length p of p
enddef;

vardef self@# expr p =
  cuta(@#,@#) @#.c{curl0}..@#.c+p..{curl0}@#.c  enddef;

verbatimtex \def\stk#1#2{$\displaystyle{\matrix{#1\cr#2\cr}}$} etex
circleit.aa("Start"); aa.dx=aa.dy;
circleit.bb(btex \stk B{(a|b)^*a} etex);
circleit.cc(btex \stk C{b^*} etex);
circleit.dd(btex \stk D{(a|b)^*ab} etex);
circleit.ee("Stop"); ee.dx=ee.dy;

numeric hsep;
bb.c-aa.c = dd.c-bb.c = ee.c-dd.c = (hsep,0);
cc.c-bb.c = (0,.8hsep);
xpart(ee.e - aa.w) = 3.8in;
drawboxed(aa,bb,cc,dd,ee);

label.ulft(btex$b$etex, cuta(aa,cc) aa.c{dir50}..cc.c);
label.top(btex$b$etex, self.cc(0,30pt));
label.rt(btex$a$etex, cuta(cc,bb) cc.c..bb.c);
label.top(btex$a$etex, cuta(aa,bb) aa.c..bb.c);
label.llft(btex$a$etex, self.bb(-20pt,-35pt));
label.top(btex$b$etex, cuta(bb,dd) bb.c..dd.c);
label.top(btex$b$etex, cuta(dd,ee) dd.c..ee.c);
label.lrt(btex$a$etex, cuta(dd,bb) dd.c..{dir140}bb.c);
label.bot(btex$a$etex, cuta(ee,bb) ee.c..tension1.3 ..{dir115}bb.c);
label.urt(btex$b$etex, cuta(ee,cc) ee.c{(cc.c-ee.c)rotated-15}..cc.c);
endfig;

end
