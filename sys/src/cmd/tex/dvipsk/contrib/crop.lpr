%!
%   This file is for crop marks and registration marks if there is color.
%
%   First, we translate and draw the marks.  You can change the way the
%   marks are drawn, but the quarter inch border around the page is fixed.
%
%   This file uses bop-hook; sorry.
%
TeXDict begin
%
% CM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                            %
% x_center y_center  CM  -                                                   %
%                                                                            %
% Make a crop mark at x_center y_center.  This crop mark is just a cross.    %
% Checks to see if TeXcolorcmyk is defined---if there is no color, you can   %
% setgray instead of setcmykcolor.                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/cX 18 def      % the crop offset
/CM
{
   gsave
      3 1 roll
      translate
      rotate
      .3 setlinewidth
      /TeXcolorcmyk where {pop 1 1 1 1 setcmykcolor} {0 setgray} ifelse
      0 cX neg moveto
      0 cX 2 div neg lineto stroke
      cX neg 0 moveto
      cX 2 div neg 0 lineto stroke
   grestore
} def
%
% RegMark %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                            %
% x_center y_center  RegMark  -                                              %
%                                                                            %
% Make a registration mark at x_center y_center.  Consists of two concentric %
% circles, the inner one filled, and a cross hair through them.  Preferred   %
% by those who register films for proof.  Checks to see if TeXcolorcmyk is   %
% defined---if there is no color, you don't need registration marks!         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/RegMark
{
   /TeXcolorcmyk where
   {
      pop
      gsave
         translate
         .3 setlinewidth
         1 1 1 1 setcmykcolor
         0 0 3 0 360 arc fill
         0 0 6 0 360 arc stroke
         0 1 3 {
            pop
            90 rotate
            0 0 0 0 setcmykcolor
            0 0 moveto 3 0 lineto stroke
            1 1 1 1 setcmykcolor
            3 0 moveto 8 0 lineto stroke
         } for
      grestore
   } {pop pop} ifelse
} def
%
% DoLogo %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                            %
% x_left y_upper  DoLogo  -                                                  %
%                                                                            %
% If /CompositorName is defined, then the logo will be placed in the crop    %
% area. /JobDescription and /Contractor can also be defined in the TeX file  %
% to define the job. If there is no definition, they simply will not show    %
% up. Examples:                                                              %
%   /CompositorName (Meridian Creative Group) def                            %
%   /JobDescription (College Algebra: Concepts and Models 2/e) def           %
%   /Contractor     (D.C. Heath and Company) def                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/DoLogo
{
   moveto
   save
      %
      % Variables to control the look of the Logo:
      %
      /LogoVgap 1 def              % Vertical gap between logo string and box
      /LogoHgap 3 def              % Horizontal gap between logo string and box
      /LogoFont {6 /Helvetica} def % The font to use for this logo.
      %
      % Gather some useful information about the Logo Font.
      %
      /LogoAscent LogoFont findfont /FontBBox get 3 get abs 1000 div mul def
      /LogoDescent LogoFont findfont /FontBBox get 1 get abs 1000 div mul def
      /LogoHeight LogoAscent LogoDescent add def
      %
      % <string> AddToLogo <string width>
      %
      /LogoWidth 0 def
      /AddToLogo
      {
         dup show                         % leaving <string> on stack
         stringwidth pop                  % <swidth> on stack
      } def
      %
      % Start by setting line width, color, and establish logo font.
      %
      .24 setlinewidth
      /TeXcolorcmyk where {pop 1 1 1 1 setcmykcolor} {0 setgray} ifelse
      LogoFont findfont exch scalefont setfont
      %
      % Then move into position
      %
      LogoHgap LogoVgap LogoAscent add neg rmoveto
      %
      % If there is a logo, there will be a compositor, so start by setting
      % the compositor name and add to the LogoWidth.
      %
      CompositorName AddToLogo          % Add compositor name to logo
      LogoWidth add /LogoWidth exch def % Add compositor name to logo width
      %
      % If there is a job description, set it next.
      %
      /JobDescription where
      {
         pop                            % Get rid of dictionary
         LogoFont pop 0 rmoveto         % Move a bit to the right
         LogoWidth LogoFont pop add     % Add that on to the logo width
         JobDescription AddToLogo       % Add job description to logo
         add /LogoWidth exch def        % Add job description to logo width
      } if
      %
      % Same thing with Contractor.
      %
      /Contractor where
      {
         pop                            % Get rid of dictionary
         LogoFont pop 0 rmoveto         % Move a bit to the right
         LogoWidth LogoFont pop add     % Add that on to the logo width
         Contractor AddToLogo           % Add contractor to logo
         add /LogoWidth exch def        % Add contractor to logo width
      } if
      %
      % Add the gaps onto the logo dimensions for box drawing.
      %
      /LogoHeight LogoHeight 2 LogoVgap mul add def
      /LogoWidth LogoWidth 2 LogoHgap mul add def
      %
      % Move into lower right corner of box and draw it clockwise.
      %
      LogoHgap LogoVgap LogoDescent add neg rmoveto
      LogoWidth neg 0 rlineto 0 LogoHeight rlineto
      LogoWidth 0 rlineto 0 LogoHeight neg rlineto stroke
   restore
   stroke
} def
%
/DrawCenterTicks
{
   gsave
      .3 setlinewidth
      /TeXcolorcmyk where {pop 1 1 1 1 setcmykcolor} {0 setgray} ifelse
      hsize cX 2 mul sub 2 div  cX 2 div neg  moveto
         0  cX 2 div neg  rlineto
      hsize cX 1.5 mul sub  vsize cX 2 mul sub 2 div  moveto
         cX 2 div  0  rlineto
      hsize cX 2 mul sub 2 div  vsize cX 1.5 mul sub  moveto
         0  cX 2 div  rlineto
      cX 2 div neg  vsize cX 2 mul sub 2 div  moveto
         cX 2 div neg  0  rlineto
      stroke
   grestore
} def
%
% NamePlates %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% left bottom  NamePlates  -                                                %
%                                                                           %
% /FirstPlate, /SecondPlate, /ThirdPlate, and /FourthPlate have default     %
% names but may be overridden in the job.hdr.                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/NamePlates
{
      gsave
         translate
         % Make sure each plate has a name.
         /FirstPlate  where {pop} {/FirstPlate  (Cyan)    def} ifelse
         /SecondPlate where {pop} {/SecondPlate (Magenta) def} ifelse
         /ThirdPlate  where {pop} {/ThirdPlate  (Yellow)  def} ifelse
         /FourthPlate where {pop} {/FourthPlate (Black)   def} ifelse
         /Helvetica-Bold findfont 10 scalefont setfont
         % Cyan block
         1 0 0 0 setcmykcolor
         0 0 moveto
         9 0 lineto
         9 9 lineto
         0 9 lineto
         closepath
         fill
         0 0 0 0 setcmykcolor
         0.75 1 moveto
         (C) show
         % Magenta block
         0 1 0 0 setcmykcolor
          9 0 moveto
         18 0 lineto
         18 9 lineto
          9 9 lineto
         closepath
         fill
         0 0 0 0 setcmykcolor
         9.25 1 moveto
         (M) show
         % Yellow block
         0 0 1 0 setcmykcolor
         18 0 moveto
         27 0 lineto
         27 9 lineto
         18 9 lineto
         closepath
         fill
         0 0 0 0 setcmykcolor
         19.25 1 moveto
         (Y) show
         % Black block
         0 0 0 1 setcmykcolor
         27 0 moveto
         36 0 lineto
         36 9 lineto
         27 9 lineto
         closepath
         fill
         0 0 0 0 setcmykcolor
         27.5 1 moveto
         (K) show
         % Name Plates
         /Helvetica-Narrow findfont 9 scalefont setfont
         1 1 1 1 setcmykcolor
         ( Plate:) show
         1 0 0 0 setcmykcolor
         FirstPlate  show
         0 1 0 0 setcmykcolor
         SecondPlate show
         0 0 1 0 setcmykcolor
         ThirdPlate  show
         0 0 0 1 setcmykcolor
         FourthPlate show
      grestore
} def
%
end %TeXDict
%
/bop-hook {
   cX dup TR % move the origin a bit
   gsave
      % Draw center tic marks in the crop margin
      DrawCenterTicks
      % Do the logo
      /CompositorName where {pop cX 2 div dup neg DoLogo} if
      % Name the plates
      /TeXcolorcmyk where {pop hsize 2 div cX add -18 NamePlates} if
      % now draw four crop marks and four registration marks
      0 0 0 CM cX -2 div dup RegMark
      vsize cX 2 mul sub dup
      hsize cX 2 mul sub dup
      isls { 4 2 roll } if
      0 2 copy 90 CM cX 2 div sub exch cX 2 div add exch RegMark
      exch 2 copy 180 CM cX 2 div add exch cX 2 div add exch RegMark
      0 exch 2 copy 270 CM cX 2 div add exch cX 2 div sub exch RegMark
   grestore
   0 cX -2 mul TR % now move to where we start drawing
   isls { cX -2 mul 0 TR } if
} def
