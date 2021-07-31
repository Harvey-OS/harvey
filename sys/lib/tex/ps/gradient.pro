%!
% PostScript header file gradient.ps
% For the PSTricks gradient fillstyle.
% Version: 0.93a
% Date:    93/03/12
% For copying restrictions, see pstricks.doc or pstricks.tex.
%
% Based on some EPS files by leeweyr!bill@nuchat.sccsi.com (W. R. Lee).
%
% Syntax:
%   R0 G0 B0 R1 G1 B1 MidPoint NumLines Angle GradientFill

/tx@GradientDict 40 dict def
tx@GradientDict begin
/GradientFill {
  rotate
  /MidPoint ED
  /NumLines ED
  /LastBlue ED
  /LastGreen ED
  /LastRed ED
  /FirstBlue ED
  /FirstGreen ED
  /FirstRed ED
  clip
  pathbbox           %leave llx,lly,urx,ury on stack
  /y ED /x ED
  2 copy translate
  y sub neg /y ED
  x sub neg /x ED
  /b {
    x 0 rlineto
    0 YSizePerLine rlineto
    x neg 0 rlineto
    closepath
  } def
  /MidLine NumLines 1 MidPoint sub mul abs cvi def
  MidLine NumLines gt { /Midline NumLines def } if
  /RedIncrement LastRed FirstRed sub MidLine div def
  /GreenIncrement LastGreen FirstGreen sub MidLine div def
  /BlueIncrement LastBlue FirstBlue sub MidLine div def
  /YSizePerLine y NumLines div def
  /CurrentY 0 def
  /Red FirstRed def
  /Green FirstGreen def
  /Blue FirstBlue def
  % This avoids gaps due to rounding errors:
  gsave Red Green Blue setrgbcolor fill grestore
  MidLine {
    0 CurrentY moveto b
    Red Green Blue setrgbcolor fill
    CurrentY YSizePerLine add /CurrentY exch def
    Blue BlueIncrement add dup 1 gt { pop 1 } if
      dup 0 lt { pop 0 } if /Blue exch def
    Green GreenIncrement add dup 1 gt { pop 1 } if
      dup 0 lt { pop 0 } if /Green exch def
    Red RedIncrement add dup 1 gt { pop 1 } if
      dup 0 lt { pop 0 } if /Red exch def
  } repeat
  Blue BlueIncrement sub /Blue exch def
  Green GreenIncrement sub /Green exch def
  Red RedIncrement sub /Red exch def
  /RedIncrement LastRed FirstRed sub NumLines MidLine sub div def
  /GreenIncrement LastGreen FirstGreen sub NumLines MidLine sub div def
  /BlueIncrement LastBlue FirstBlue sub NumLines MidLine sub div def
  Blue BlueIncrement sub /Blue exch def
  Green GreenIncrement sub /Green exch def
  Red RedIncrement sub /Red exch def
  NumLines MidLine sub 1 add {
    0 CurrentY moveto b
    Red Green Blue setrgbcolor fill
    CurrentY YSizePerLine add /CurrentY exch def
    Blue BlueIncrement sub dup 1 gt { pop 1 } if
      dup 0 lt { pop 0 } if /Blue exch def
    Green GreenIncrement sub dup 1 gt { pop 1 } if
      dup 0 lt { pop 0 } if /Green exch def
    Red RedIncrement sub dup 1 gt { pop 1 } if
      dup 0 lt { pop 0 } if /Red exch def
  } repeat
} def
end
% END gradient.ps
