%!
% PostScript prologue for pst-node.tex.
% Created 1993/3/12. Source file was pst-node.doc
% Version 0.93a, 93/03/12.
% For use with Rokicki's dvips.
/tx@NodeDict 200 dict def tx@NodeDict begin
/NewNode { gsave /next ED dict dup 3 -1 roll ED begin tx@Dict begin STV
CP T exec end /NodeMtrx CM def next end grestore } def
/InitPnode { /Y ED /X ED /NodePos { Nodesep Cos mul Nodesep Sin mul } def
} def
/InitCnode { /r ED /Y ED /X ED /NodePos { Nodesep r add dup Cos mul exch
Sin mul } def } def
/GetRnodePos { Cos 0 gt { /dx r Nodesep add def } { /dx l Nodesep sub def
} ifelse Sin 0 gt { /dy u Nodesep add def } { /dy d Nodesep sub def }
ifelse dx Sin mul abs dy Cos mul abs gt { dy Cos mul Sin div dy } { dx
dup Sin mul Cos Div } ifelse } def
/InitRnode { /r ED r mul neg /l ED /r r l add def /X l neg def { neg /d
ED /u ED /Y 0 def } { neg /Y ED Y sub /u ED u mul neg /d ED /u u d add
def /Y Y d sub def } ifelse /NodePos { GetRnodePos } def } def
/InitRNode { /Y ED /X ED /r ED /X r 2 div X add def /r r X sub def /l X
neg def Y add neg /d ED Y sub /u ED /NodePos { GetRnodePos } def } def
/GetOnodePos { /ww w Nodesep add def /hh h Nodesep add def Sin ww mul Cos
hh mul Atan dup cos ww mul exch sin hh mul } def
/GetCenter { begin X Y NodeMtrx transform CM itransform end } def
/GetAngle { nodeA GetCenter nodeB GetCenter 3 -1 roll sub 3 1 roll sub
neg Atan  } def
/GetEdge { begin /Nodesep ED dup 1 0 NodeMtrx dtransform CM idtransform
exch atan sub dup sin /Sin ED cos /Cos ED NodePos Y add exch X add exch
NodeMtrx transform CM itransform end 4 2 roll 1 index 0 eq { pop pop } {
2 copy 5 2 roll cos mul add 4 1 roll sin mul sub exch } ifelse } def
/GetPos { OffsetA AngleA NodesepA nodeA GetEdge /y1 ED /x1 ED OffsetB
AngleB NodesepB nodeB GetEdge /y2 ED /x2 ED } def
/InitNC { /nodeB ED /nodeA ED /NodesepB ED /NodesepA ED /OffsetB ED
/OffsetA ED tx@NodeDict nodeA known tx@NodeDict nodeB known and dup {
/nodeA nodeA load def /nodeB nodeB load def } if } def
/LineMP { 4 copy 1 t sub mul exch t mul add 3 1 roll 1 t sub mul exch t
mul add exch 6 2 roll sub 3 1 roll sub Atan  } def
/NCCoor { GetAngle /AngleA ED /AngleB AngleA 180 add def GetPos /LPutVar
[ x2 x1 y2 y1 ] cvx def /LPutPos { LPutVar LineMP } def x1 y1 x2 y2 }
def
/NCLine { NCCoor tx@Dict begin ArrowB 4 2 roll ArrowA lineto end } def
/BezierMidpoint { /y3 ED /x3 ED /y2 ED /x2 ED /y1 ED /x1 ED /y0 ED /x0 ED
/t ED /cx x1 x0 sub 3 mul def /cy y1 y0 sub 3 mul def /bx x2 x1 sub 3
mul cx sub def /by y2 y1 sub 3 mul cy sub def /ax x3 x0 sub cx sub bx
sub def /ay y3 y0 sub cy sub by sub def ax t 3 exp mul bx t t mul mul
add cx t mul add x0 add ay t 3 exp mul by t t mul mul add cy t mul add
y0 add 3 ay t t mul mul mul 2 by t mul mul add cy add 3 ax t t mul mul
mul 2 bx t mul mul add cx add atan } def
/GetArms { /x1a armA AngleA cos mul x1 add def /y1a armA AngleA sin mul
y1 add def /x2a armB AngleB cos mul x2 add def /y2a armB AngleB sin mul
y2 add def } def
/NCCurve { GetPos x1 x2 sub y1 y2 sub Pyth 2 div dup 3 -1 roll mul /armA
ED mul /armB ED GetArms x1a y1a x1 y1 tx@Dict begin ArrowA end x2a y2a
x2 y2 tx@Dict begin ArrowB end curveto /LPutVar [ x1 y1 x1a y1a x2a y2a
x2 y2 ] cvx def /LPutPos { t LPutVar BezierMidpoint } def } def
/AnglesMP { LPutVar t 3 gt { /t t 3 sub def } { t 2 gt { /t t 2 sub def
10 -2 roll } { t 1 gt { /t t 1 sub def 10 -4 roll } { 10 4 roll } ifelse
} ifelse } ifelse 6 { pop } repeat 3 -1 roll exch LineMP  } def
/NCAngles { GetPos GetArms /mtrx AngleA matrix rotate def x1a y1a mtrx
transform pop x2a y2a mtrx transform exch pop mtrx itransform /y0 ED /x0
ED mark armB 0 ne { x2 y2 } if x2a y2a x0 y0 x1a y1a armA 0 ne { x1 y1 }
if tx@Dict begin false Line end /LPutVar [ x2 y2 x2a y2a x0 y0 x1a y1a
x1 y1 ] cvx def /LPutPos { AnglesMP } def } def
/NCAngle { GetPos /x2a armB AngleB cos mul x2 add def /y2a armB AngleB
sin mul y2 add def /mtrx AngleA matrix rotate def x2a y2a mtrx transform
pop x1 y1 mtrx transform exch pop mtrx itransform /y0 ED /x0 ED mark
armB 0 ne { x2 y2 } if x2a y2a x0 y0 x1 y1 tx@Dict begin false Line end
/LPutVar [ x2 y2 x2 y2 x2a y2a x0 y0 x1 y1 ] cvx def /LPutPos { AnglesMP
} def } def
/NCBar { GetPos GetArms /mtrx AngleA matrix rotate def x1a y1a mtrx
transform pop x2a y2a mtrx transform pop sub dup 0 mtrx itransform 3 -1
roll 0 gt { /y2a exch y2a add def /x2a exch x2a add def } { /y1a exch
neg y1a add def /x2a exch neg x2a add def } ifelse mark x2 y2 x2a y2a
x1a y1a x1 y1 tx@Dict begin false Line end /LPutVar [ x2 y2 x2 y2 x2a
y2a x1a y1a x1 y1 ] cvx def /LPutPos { LPutVar AnglesMP } def } def
/NCDiag { GetPos GetArms mark x2 y2 x2a y2a x1a y1a x1 y1 tx@Dict begin
false Line end /LPutVar [ x2 y2 x2 y2 x2a y2a x1a y1a x1 y1 ] cvx def
/LPutPos { AnglesMP } def } def
/NCDiagg { OffsetA AngleA NodesepA nodeA GetEdge /y1 ED /x1 ED /x1a armA
AngleA cos mul x1 add def /y1a armA AngleA sin mul y1 add def nodeB
GetCenter y1a sub exch x1a sub Atan 180 add /AngleB ED OffsetB AngleB
NodesepB nodeB GetEdge /y2 ED /x2 ED mark x2 y2 x1a y1a x1 y1 tx@Dict
begin false Line end /LPutVar [ x2 y2 x2 y2 x2 y2 x1a y1a x1 y1] cvx def
/LPutPos { AnglesMP } def } def
/LoopMP { /t t abs def [ LPutVar ] length 2 div 1 sub dup t lt { /t ED }
{ pop } ifelse mark LPutVar t cvi { /t t 1 sub def pop pop } repeat
counttomark 1 add 4 roll cleartomark 3 -1 roll exch LineMP  } def
/NCLoop { GetPos GetArms /mtrx AngleA matrix rotate def x1a y1a mtrx
transform loopsize add /y1b ED /x1b ED /x2b x2a y2a mtrx transform pop
def x2b y1b mtrx itransform /y2b ED /x2b ED x1b y1b mtrx itransform /y1b
ED /x1b ED mark armB 0 ne { x2 y2 } if x2a y2a x2b y2b x1b y1b x1a y1a
armA 0 ne { x1 y1 } if tx@Dict begin false Line end /LPutVar [ x2 y2 x2a
y2a x2b y2b x1b y1b x1a y1a x1 y1 ] cvx def /LPutPos { LoopMP } def }
def
/NCCircle { nodeA GetCenter 0 0 NodesepA nodeA GetEdge pop 3 1 roll /Y ED
/X ED X sub 2 div dup 2 exp r r mul sub abs sqrt atan 2 mul /a ED r
AngleA 90 add PtoC Y add exch X add exch 2 copy /LPutVar [ 4 2 roll r a
] def /LPutPos { LPutVar aload pop t 360 mul add dup 5 1 roll 90 sub
PtoC 3 -1 roll add 3 1 roll add exch 3 -1 roll } def r AngleA 90 sub a
add AngleA 270 add a sub tx@Dict begin /angleB ED /angleA ED /r ED /c
57.2957 r Div def /y ED /x ED } def
/LPutCoor { tx@NodeDict /LPutPos known { gsave LPutPos tx@Dict begin
/langle ED CM 3 1 roll STV CP 3 -1 roll sub neg 3 1 roll sub exch moveto
setmatrix CP end grestore } { 0 0 tx@Dict /langle 0 def end } ifelse }
def
end
