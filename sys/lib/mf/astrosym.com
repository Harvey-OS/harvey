% ASTROSYM.COM : AstroSym (Version 1.00, May 1, 1992) - file 4 of 7
% Peter Schmitt                     eMail: a8131dal@awiuni11.bitnet
% Institute of Mathematics, University of Vienna    Vienna, Austria
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

beginchar(O+2,52u#,80u#,0u#); "Venus";
   corners; centers;
   VV:=(TC-TL) rotated -90;
   CL:=TL+VV; CR:=TR+VV; bC:=TC+2VV;
    PEN; draw TC..CR..bC..CL..TC..cycle;                   % top circle
   SPEN; cross (bC,BC, 4/3,-2P);                           % bottom cross
   endchar;

beginchar(O+3,52u#,80u#,0u#); "Terra";
   corners; centers;
   VV:=(BC-BL) rotated 90;
   CL:=BL+VV; CR:=BR+VV; tC:=BC+2VV;
    PEN; draw tC..CR..BC..CL..tC..cycle;                   % bottom circle
   SPEN; cross (tC,TC, 1,2P);                              % top cross
   endchar;

beginchar(O+4,52u#,88u#,0u#); "Mars";
   corners; centers;
   VV:=(BC-BL) rotated 90;
   CL:=BL+VV; CR:=BR+VV; tC:=BC+2VV;
   CC:=BC+VV;
   Cr:=(tC+2vP) rotatedaround (CC,-15);             % temporary
   TR:=TR+vd+hd;                                    % top arrow: top
   tr:=.45[Cr,TR];                                   % top arrow: base
   Cr:=Cr-(2hP rotated angle(TR-Cr));               % top arrow: stem base
   a:=angle(TR-tr); b:=cosd(a)/sind(a);             % arrow: calculate breadth
   PEN; draw tC..CR..BC..CL..tC..cycle;             % bottom circle
   PEN; draw Cr--tr;                                % top arrow: stem
   Pen; arrow (tr,TR, 2b, .1,.5);                   % top arrow
   endchar;

beginchar(O+7,52u#,74u#,10u#); "Uranus";
   corners; centers;
   VV:=(BC-BL) rotated 90;
   bL:=BL+VV; bR:=BR+VV; CC:=BC+2VV;
   bC:=BC+VV;
   TC:=TC+vd; tC:=.4[CC,TC];
   PEN; draw CC..bR..BC..bL..CC..cycle;                   % bottom circle
     fill fullcircle scaled 2P shifted bC;                % bottom circle: dot
     draw CC--tC;                                         % top arrow: stem
   Pen; arrow (tC,TC, 1.2, .1,.5);                        % top arrow
   endchar;

beginchar(O+8,88u#,80u#,0u#); "Neptunus";
   Corners(p,p,P,p); Centers(.5,.2);
   HOR (9/88,79/88);
   Top(.725); Bot(.2);
   b:=length(Tl-TL)/length(Tl-tl);                 % arrows: breadth of points
   SPEN; draw bl--br;                              % horizontal bar
         draw bC--BC;                              % vertical stem
         draw bl--tl; draw bC--tC; draw br--tr;    % three arrows: arms
   Pen; arrow (tl,Tl, 2b, .2,.5);                  % three arrows: points
        arrow (tC,TC, 2b, .2,.5);
        arrow (tr,Tr, 2b, .2,.5);
   endchar;

beginchar(O+9,52u#,80u#,0u#); "Pluto";
   corners; centers;
   VV:=(TC-TL) rotated -90;
   bC:=TC+2VV; bL:=TL+2VV;
   BOT (0,2/3);                                  % Br : bottom bar (right end)
    PEN; draw TL--TC{right}..{left}bC--bL;       % top right arc
   SPEN; draw BL--TL; draw BL--Br;               % left bar and bottom bar
   endchar;

beginchar(O+21,76u#,58u#,-6u#); "Aquarius";
   Corners(P,P,(h+d)/2+P,P);
                                                % upper strokes:
   centers;                                    % CL and CR : left and right end
   BOT (3/12,11/12); BC:=.5[Bl,Br];             % Bl,BC,Br : upper endpoints
   TOP (1/12,9/12); TC:=.5[Tl,Tr];              % Tl,TC,Tr : lower endpoints
%  PEN yscaled (p/P) rotated (90-angle(Tl-CL));
   PEN yscaled (p/P) rotated angle(Tl-CL);
   draw CL--Tl--Bl--TC--BC--Tr--Br--CR;
   picture upper; upper:= currentpicture;                     % lower strokes
   addto currentpicture also upper shifted (0,-(h+d)/2);
   endchar;

beginchar(O+24,88u#,80u#,0u#); "Neptunus 2";
   Corners(p,p,P,p); Centers(.5,.2);
   HOR (9/88,79/88);
   Top(.725); Bot(.2);
   b:=length(Tl-TL)/length(Tl-tl);                 % arrows: breadth of points
   SPEN; draw bC--tC;                              % middle arm
   cross (BC,bC, 1, -P);                           % lower cross
    PEN; draw tl{down}..{up}tr;                    % arc from left to right arm
   Pen; arrow (tl,Tl, 2b, .2,.5);                  % three arrows: points
        arrow (tC,TC, 2b, .2,.5);
        arrow (tr,Tr, 2b, .2,.5);
   endchar;

beginchar(O+25,68u#,80u#,0u#); "Pluto 2";
   Corners(p,p,P,p); Centers(.5,.2);
   HOR (2/17,15/17);
   Top(.725); Bot(.2)
   b:=length(Tl-TL)/length(Tl-tl);                 % arrows: breadth of points
   SPEN; draw bl--br;                              % horizontal bar
         draw bC--BC;                              % vertical stem
         draw bl--tl; draw br--tr;                 % two arrows: arms
   Pen; arrow (tl,Tl, 2b, .2,.5);                  % two arrows; points
        arrow (tr,Tr, 2b, .2,.5);
   endchar;

beginchar(O+26,1U#,1U#,0); "Libra 2";
   corners; Centers (1/2,2/3-1/12);
   LFT (1/3,1); RT (1/3,1);
   PEN; draw CC{right}..TC..{right}CC;             % top circle
   SPEN; draw bL--bR; draw CL--CR;                 % lower and upper bar
   endchar;

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%% end of ASTROSYM.COM %%%
