batchmode;
delimiters ();
def -- = {curl 1}..{curl 1} enddef;
showstats;

begingroup
save MF, TF, MS, TS;
interim warningcheck:=0;

vardef MF(text p)(text q) = show p/q (4096) enddef;
vardef TF(text p)(text q) = show p/4096 (q), p/4096 (-q) enddef;
vardef MS(expr p, q) = show p/q, (-p)/q, p/(-q), (-p)/(-q) enddef;
vardef TS(expr p, q) = show p*q, (-p)*q, p*(-q), (-p)*(-q) enddef;

% Hard cases for arithmetic routines (taken from mp/mathtest.c)
TF(1618.00821)(19313.86989);   % p*q=(10^9+1)*2^27-6 => 7629.39453
TS(12.91296)(12.91296);        % => 166.74463
MF(4793.68996)(5477.58977);    % => 3584.59740
MS(12.91296)(12.91296);        % => 1.00000
TF(2048.00000)(15258.78908);   % p*q=(10^9+1)*2^27 => 7629.39455
TF(2644.17595)(11818.42685);   % p*q=(10^9+1)*2^27+2 => 7629.39455
TF(1164.89432)(26826.46791);   % p*q=(10^9+1)*2^27+2^14+2 => 7629.39455
TF(3310.94371)(9438.39665);    % p*q=(10^9+1)*2^27-2^14-3 => 7629.39453
TS(13107.20000)(2.50000);      % p*q=2^47-2^15 => *
TS(124.77580)(262.61502);      % p*q=2^47-2^15+6 => *
TS(1038.61232)(31.54979);      % p*q=2^47-2^15-1 => 32767.99998
TS(64.03127)(511.75000);       % p*q=2^47-2^14 => *
TS(39.46178)(830.37315);       % p*q=2^47-2^14+1 => *
TS(1207.05434)(27.14708);      % p*q=2^47-2^14-1 => *
TS(7.25000)(4519.72414);       % p*q=2^47-3*2^14 => 32767.99998
TS(215.31056)(152.18947);      % p*q=2^47-3*2^14+1 => 32767.99998
TS(14833.24916)(2.20909);      % p*q=2^47-3*2^14-1 => 32767.99998
TS(339.62500)(22.46417);       % p*q=(10^9+1)*2^15 => 7629.39455
TS(47.62218)(160.20676);       % p*q=(10^9+1)*2^15+2 => 7629.39455
TS(74.64186)(102.21335);       % p*q=(10^9+1)*2^15-2 => 7629.39453
TF(2.50000)(13107.20000);      % p*q=2^47-2^15 => 8.00000
TF(47.62218)(160.20676);       % p*q=(10^9+1)*2^15+2 => 1.86264
MF(7629.39455)(8192.00000);    % 2^29*p+q=2q*(5*10^8+1)+0 => 3814.69728
MF(15258.78906)(24998.04695);  % 2^29*p+q=2q*(163852801)-5 => 2500.19531
MF(15258.78906)(20546.77141);  % 2^29*p+q=2q*(199350055)+1 => 3041.84044
MF(15258.78906)(30447.06313);  % 2^29*p+q=2q*(134528575)+2^14-5 => 2052.74315
MS(15258.78908)(2.00000);      % 2^17*p+q=2q*(5*10^8+1)+0 => 7629.39455
MS(15258.78906)(166.56779);    % 2^17*p+q=2q*(6003562)-1 => 91.60707
MS(15258.78906)(394.96335);    % 2^17*p+q=2q*(2531881)+2 => 38.63344
MS(15258.78906)(42.34901);     % 2^17*p+q=2q*(23613301)-2^14-1 => 360.31036
show oct "77777", hex "400a";
endgroup;

showstats;
path triang, circ;
triang = (-3,-2)--(7,-2)--(0,7)--cycle;
circ = makepath(pencircle shifted (0,.5) scaled 10);
showstats;

picture pic, pic[];
showstats;
begingroup
save sh, dr;
vardef sh expr p =
  show p, stroked p, filled p, textual p, clipped p, bounded p, length p;
  showstats;
enddef;
vardef dr text t =
  pp3:=nullpicture;
  addto pp3 t;
  sh pp3;
enddef;
save pp; picture pp[];
showstats;
pic = pp2 = nullpicture;
sh pic;
clip pic to makepath(pencircle scaled 100);
sh pic; % clipped
setbounds pp2 to triang;
sh pp2; % bounded()
dr doublepath (1,1)--(3,2) withpen pencircle; % stroked
addto pic also pp3;
sh pic; % clipped(), stroked
show ulcorner pic, lrcorner pic;
setbounds pic to triang rotated 90;
sh pic; show llcorner pic, urcorner pic;
sh pic; % bounded(clipped(), stroked)
dr contour triang scaled 12 withcolor (.4,.4,.4); % filled
addto pic also pp3;
sh pic; % bounded(clipped(), stroked), filled
dr also "A56" infont "trapf"; % text
addto pic also pp3 rotated 30 shifted (2,3);
sh pic; % bounded(clipped(), stroked), filled, textual
dr doublepath (4,5)--(8,1) withpen pencircle xscaled 0;
addto pic also pp3;
sh pic; % bounded(clipped(), stroked), filled, textual, stroked
sh "non-picture";
endgroup;

showstats;
for p within pic: show p; endfor

showstats;
pic1 := pic;
clip pic1 to circ;
show length pic1;
showstats;
addto pic1 also pic dashed nullpicture;
show pic1, length pic1;
showstats;
for p within pic1:
  show stroked p, filled p, textual p, clipped p, bounded p;
  show pathpart p, penpart p, dashpart p, fontpart p, textpart p;
  show (redpart p, greenpart p, bluepart p);
  show (xpart p, ypart p), (xxpart p, yxpart p), (xypart p, yypart p);
  showstats;
endfor
pic1 := nullpicture;

for p within nullpicture: show p; endfor
for p within "B34" infont "trapf": show p; endfor
showstats;

shipout pic;
charcode:=1;
prologues:=1;
shipout pic rotated 90;


string s,EOF;
EOF=char 0;
numeric n;
n = 0;
showstats;
forever: n:=n+1;
  s:= readfrom "mtrap.0";
  exitif (s=EOF) or (n>300);
  if substring (0,14) of s<>"%%CreationDate": show s; fi
endfor

numeric n;
n = 0;
showstats;
forever: n:=n+1;
  s := readfrom "mtrap.1";
  if n<=2: show s; fi
  exitif (s=EOF) or (n>300);
endfor
show n;

showstats;
show readfrom "mtrap.17";
for n=1 step 1 until 3:
  s := readfrom "mtrap.0"; show s;
endfor

showstats;
for i=1 step 1 until 10:
  write decimal i & "  " & decimal(i*i) to "writeo";
  write substring(0,i) of "             " & "*" to "writeo.2";
endfor
showstats;

write EOF to "writeo";
s:=readfrom "mtrap.0"; % Ignore creation date
for i=1 step 1 until 3:
  show readfrom "writeo", readfrom "mtrap.0";
endfor
showstats;

write "Clobbered." to "writeo";

tracingstats:=1;
begingroup  % Force string pool compaction
save s; string s[];
s0="a"; s1="b"; s2="c"; s3="d";
randomseed:=4.34294;
for i=1 step 1 until 200:
  s[floor uniformdeviate 4] := substring (0,60) of
    (s[floor uniformdeviate 4] & s[floor uniformdeviate 4]);
endfor
show s1;
endgroup;
showstats;


begingroup
numeric n;
interim warningcheck:=1;
n = 12000; show n;
interim warningcheck:=0;
n:= 40000; show n;
endgroup;

showstats;
for x within triangle:  show x; endfor
showstats;
write triang to "writeo";
write "triangle" "writeo";
write "triangle" to triang;
showstats;

closefrom "mtrap.0";
string s; s=readfrom "mtrap.0";
show readfrom "mtrap.0";   % should be 2nd line, not 5th line

end
