%!
% PostScript prologue for pst-coil.tex.
% Created 1993/3/12. Source file was pst-coil.doc
% Version 0.93a, 93/03/12.
% For use with Rokicki's dvips.
/tx@CoilDict 40 dict def tx@CoilDict begin
/CoilLoop { /t ED t sin AspectSin mul t 180 div AspectCos mul add t cos
lineto } def
/Coil { /Inc ED dup sin /AspectSin ED cos /AspectCos ED /ArmB ED /ArmA ED
/h ED /w ED /y1 ED /x1 ED /y0 ED /x0 ED x0 y0 translate y1 y0 sub x1 x0
sub 2 copy Pyth /TotalLength ED Atan rotate /BeginAngle ArmA AspectCos
Div w h mul Div 360 mul def /EndAngle TotalLength ArmB sub AspectCos Div
w h mul Div 360 mul def 1 0 0 0 ArrowA ArmA 0 lineto /mtrx CM def w h
mul 2 Div w 2 Div scale BeginAngle Inc EndAngle { CoilLoop } for
EndAngle CoilLoop mtrx setmatrix TotalLength ArmB sub 0 lineto CP
TotalLength 0 ArrowB lineto } def
/AltCoil { /Inc ED dup sin /AspectSin ED cos /AspectCos ED /h ED /w ED
/EndAngle ED /BeginAngle ED /mtrx CM def w h mul 2 Div w 2 Div scale
BeginAngle sin AspectSin mul BeginAngle 180 div AspectCos mul add
BeginAngle cos /lineto load stopped { moveto } if BeginAngle Inc
EndAngle { CoilLoop } for EndAngle CoilLoop mtrx setmatrix } def
/ZigZag { /ArmB ED /ArmA ED 2 div /w ED w mul /h ED /y1 ED /x1 ED /y0 ED
/x0 ED x1 y1 translate y0 y1 sub x0 x1 sub 2 copy Pyth /TotalLength ED
Atan rotate TotalLength ArmA sub ArmB sub dup h div cvi /n ED n h mul
sub 2 div dup ArmA add /ArmA ED ArmB add /ArmB ED /x ArmB h 2 div add
def mark 0 0 ArmB 0 n { x w /w w neg def /x x h add def } repeat
TotalLength ArmA sub 0 TotalLength 0 } def
end
