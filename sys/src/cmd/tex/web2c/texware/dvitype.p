{3:}program DVItype;label{4:}9999,30;{:4}const{5:}maxfonts=100;
maxwidths=25000;linelength=79;terminallinelength=150;stacksize=100;
namesize=1000;{:5}type{8:}ASCIIcode=0..255;{:8}{9:}
textfile=packed file of ASCIIcode;{:9}{21:}eightbits=0..255;
bytefile=packed file of eightbits;{:21}var{10:}
xord:array[ASCIIcode]of ASCIIcode;xchr:array[0..255]of ASCIIcode;{:10}
{22:}dvifile:bytefile;tfmfile:bytefile;{:22}{24:}curloc:integer;
curname:packed array[1..PATHMAX]of char;{:24}{25:}b0,b1,b2,b3:eightbits;
{:25}{30:}fontnum:array[0..maxfonts]of integer;
fontname:array[0..maxfonts]of 1..namesize;
names:array[1..namesize]of ASCIIcode;
fontchecksum:array[0..maxfonts]of integer;
fontscaledsize:array[0..maxfonts]of integer;
fontdesignsize:array[0..maxfonts]of integer;
fontspace:array[0..maxfonts]of integer;
fontbc:array[0..maxfonts]of integer;fontec:array[0..maxfonts]of integer;
widthbase:array[0..maxfonts]of integer;
width:array[0..maxwidths]of integer;nf:0..maxfonts;
widthptr:0..maxwidths;{:30}{33:}inwidth:array[0..255]of integer;
tfmchecksum:integer;tfmdesignsize:integer;tfmconv:real;{:33}{39:}
pixelwidth:array[0..maxwidths]of integer;conv:real;trueconv:real;
numerator,denominator:integer;mag:integer;{:39}{41:}outmode:0..4;
maxpages:integer;resolution:real;newmag:integer;{:41}{42:}
startcount:array[0..9]of integer;startthere:array[0..9]of boolean;
startvals:0..9;count:array[0..9]of integer;{:42}{45:}
buffer:array[0..terminallinelength]of ASCIIcode;{:45}{48:}
bufptr:0..terminallinelength;{:48}{57:}inpostamble:boolean;{:57}{67:}
textptr:0..linelength;textbuf:array[1..linelength]of ASCIIcode;{:67}
{72:}h,v,w,x,y,z,hh,vv:integer;
hstack,vstack,wstack,xstack,ystack,zstack:array[0..stacksize]of integer;
hhstack,vvstack:array[0..stacksize]of integer;{:72}{73:}maxv:integer;
maxh:integer;maxs:integer;maxvsofar,maxhsofar,maxssofar:integer;
totalpages:integer;pagecount:integer;{:73}{78:}s:integer;ss:integer;
curfont:integer;showing:boolean;{:78}{96:}oldbackpointer:integer;
newbackpointer:integer;started:boolean;{:96}{100:}postloc:integer;
firstbackpointer:integer;startloc:integer;{:100}{107:}k,m,n,p,q:integer;
{:107}procedure initialize;var i:integer;begin setpaths(TFMFILEPATHBIT);
{11:}for i:=0 to 31 do xchr[i]:='?';xchr[32]:=' ';xchr[33]:='!';
xchr[34]:='"';xchr[35]:='#';xchr[36]:='$';xchr[37]:='%';xchr[38]:='&';
xchr[39]:='''';xchr[40]:='(';xchr[41]:=')';xchr[42]:='*';xchr[43]:='+';
xchr[44]:=',';xchr[45]:='-';xchr[46]:='.';xchr[47]:='/';xchr[48]:='0';
xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';xchr[52]:='4';xchr[53]:='5';
xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';xchr[57]:='9';xchr[58]:=':';
xchr[59]:=';';xchr[60]:='<';xchr[61]:='=';xchr[62]:='>';xchr[63]:='?';
xchr[64]:='@';xchr[65]:='A';xchr[66]:='B';xchr[67]:='C';xchr[68]:='D';
xchr[69]:='E';xchr[70]:='F';xchr[71]:='G';xchr[72]:='H';xchr[73]:='I';
xchr[74]:='J';xchr[75]:='K';xchr[76]:='L';xchr[77]:='M';xchr[78]:='N';
xchr[79]:='O';xchr[80]:='P';xchr[81]:='Q';xchr[82]:='R';xchr[83]:='S';
xchr[84]:='T';xchr[85]:='U';xchr[86]:='V';xchr[87]:='W';xchr[88]:='X';
xchr[89]:='Y';xchr[90]:='Z';xchr[91]:='[';xchr[92]:='\';xchr[93]:=']';
xchr[94]:='^';xchr[95]:='_';xchr[96]:='`';xchr[97]:='a';xchr[98]:='b';
xchr[99]:='c';xchr[100]:='d';xchr[101]:='e';xchr[102]:='f';
xchr[103]:='g';xchr[104]:='h';xchr[105]:='i';xchr[106]:='j';
xchr[107]:='k';xchr[108]:='l';xchr[109]:='m';xchr[110]:='n';
xchr[111]:='o';xchr[112]:='p';xchr[113]:='q';xchr[114]:='r';
xchr[115]:='s';xchr[116]:='t';xchr[117]:='u';xchr[118]:='v';
xchr[119]:='w';xchr[120]:='x';xchr[121]:='y';xchr[122]:='z';
xchr[123]:='{';xchr[124]:='|';xchr[125]:='}';xchr[126]:='~';
for i:=127 to 255 do xchr[i]:='?';{:11}{12:}
for i:=0 to 255 do xord[chr(i)]:=32;
for i:=32 to 126 do xord[xchr[i]]:=i;{:12}{31:}nf:=0;widthptr:=0;
fontname[0]:=1;fontspace[maxfonts]:=0;fontbc[maxfonts]:=1;
fontec[maxfonts]:=0;{:31}{43:}outmode:=4;maxpages:=1000000;startvals:=0;
startthere[0]:=false;{:43}{58:}inpostamble:=false;{:58}{68:}textptr:=0;
{:68}{74:}maxv:=2147483548;maxh:=2147483548;maxs:=stacksize+1;
maxvsofar:=0;maxhsofar:=0;maxssofar:=0;pagecount:=0;{:74}{97:}
oldbackpointer:=-1;started:=false;{:97}end;{:3}{23:}
procedure opendvifile;var i:integer;begin argv(1,curname);
reset(dvifile,curname);curloc:=0;end;procedure opentfmfile;
var i:integer;
begin if testreadaccess(curname,TFMFILEPATH)then begin reset(tfmfile,
curname);end else begin errprintpascalstring(curname);
begin writeln(stderr,': TFM file not found.');uexit(1);end;end;end;{:23}
{26:}procedure readtfmword;begin read(tfmfile,b0);read(tfmfile,b1);
read(tfmfile,b2);read(tfmfile,b3);end;{:26}{27:}
function getbyte:integer;var b:eightbits;
begin if eof(dvifile)then getbyte:=0 else begin read(dvifile,b);
curloc:=curloc+1;getbyte:=b;end;end;function signedbyte:integer;
var b:eightbits;begin read(dvifile,b);curloc:=curloc+1;
if b<128 then signedbyte:=b else signedbyte:=b-256;end;
function gettwobytes:integer;var a,b:eightbits;begin read(dvifile,a);
read(dvifile,b);curloc:=curloc+2;gettwobytes:=a*toint(256)+b;end;
function signedpair:integer;var a,b:eightbits;begin read(dvifile,a);
read(dvifile,b);curloc:=curloc+2;
if a<128 then signedpair:=a*256+b else signedpair:=(a-256)*256+b;end;
function getthreebytes:integer;var a,b,c:eightbits;
begin read(dvifile,a);read(dvifile,b);read(dvifile,c);curloc:=curloc+3;
getthreebytes:=(a*toint(256)+b)*256+c;end;function signedtrio:integer;
var a,b,c:eightbits;begin read(dvifile,a);read(dvifile,b);
read(dvifile,c);curloc:=curloc+3;
if a<128 then signedtrio:=(a*toint(256)+b)*256+c else signedtrio:=((a-
toint(256))*256+b)*256+c;end;function signedquad:integer;
var a,b,c,d:eightbits;begin read(dvifile,a);read(dvifile,b);
read(dvifile,c);read(dvifile,d);curloc:=curloc+4;
if a<128 then signedquad:=((a*toint(256)+b)*256+c)*256+d else signedquad
:=(((a-256)*toint(256)+b)*256+c)*256+d;end;{:27}{28:}
function dvilength:integer;begin checkedfseek(dvifile,0,2);
curloc:=ftell(dvifile);dvilength:=curloc;end;
procedure movetobyte(n:integer);begin checkedfseek(dvifile,n,0);
curloc:=n;end;{:28}{32:}procedure printfont(f:integer);
var k:0..namesize;
begin if f=maxfonts then write(stdout,'UNDEFINED!')else begin for k:=
fontname[f]to fontname[f+1]-1 do write(stdout,xchr[names[k]]);end;end;
{:32}{34:}function inTFM(z:integer):boolean;label 9997,9998,9999;
var k:integer;lh:integer;nw:integer;wp:0..maxwidths;alpha,beta:integer;
begin{35:}readtfmword;lh:=b2*toint(256)+b3;readtfmword;
fontbc[nf]:=b0*toint(256)+b1;fontec[nf]:=b2*toint(256)+b3;
if fontec[nf]<fontbc[nf]then fontbc[nf]:=fontec[nf]+1;
if widthptr+fontec[nf]-fontbc[nf]+1>maxwidths then begin writeln(stdout,
'---not loaded, DVItype needs larger width table');goto 9998;end;
wp:=widthptr+fontec[nf]-fontbc[nf]+1;readtfmword;nw:=b0*256+b1;
if(nw=0)or(nw>256)then goto 9997;
for k:=1 to 3+lh do begin if eof(tfmfile)then goto 9997;readtfmword;
if k=4 then if b0<128 then tfmchecksum:=((b0*toint(256)+b1)*256+b2)*256+
b3 else tfmchecksum:=(((b0-256)*toint(256)+b1)*256+b2)*256+b3 else if k=
5 then if b0<128 then tfmdesignsize:=round(tfmconv*(((b0*256+b1)*256+b2)
*256+b3))else goto 9997;end;{:35};{36:}
if wp>0 then for k:=widthptr to wp-1 do begin readtfmword;
if b0>nw then goto 9997;width[k]:=b0;end;{:36};{37:}{38:}
begin alpha:=16;while z>=8388608 do begin z:=z div 2;alpha:=alpha+alpha;
end;beta:=256 div alpha;alpha:=alpha*z;end{:38};
for k:=0 to nw-1 do begin readtfmword;
inwidth[k]:=(((((b3*z)div 256)+(b2*z))div 256)+(b1*z))div beta;
if b0>0 then if b0<255 then goto 9997 else inwidth[k]:=inwidth[k]-alpha;
end{:37};{40:}if inwidth[0]<>0 then goto 9997;
widthbase[nf]:=widthptr-fontbc[nf];
if wp>0 then for k:=widthptr to wp-1 do if width[k]=0 then begin width[k
]:=2147483647;pixelwidth[k]:=0;
end else begin width[k]:=inwidth[width[k]];
pixelwidth[k]:=round(conv*(width[k]));end{:40};widthptr:=wp;inTFM:=true;
goto 9999;9997:writeln(stdout,'---not loaded, TFM file is bad');
9998:inTFM:=false;9999:end;{:34}{44:}function startmatch:boolean;
var k:0..9;match:boolean;begin match:=true;
for k:=0 to startvals do if startthere[k]and(startcount[k]<>count[k])
then match:=false;startmatch:=match;end;{:44}{47:}procedure inputln;
var k:0..terminallinelength;begin flush(stderr);
if eoln(stdin)then readln(stdin);k:=0;
while(k<terminallinelength)and not eoln(stdin)do begin buffer[k]:=xord[
getc(stdin)];k:=k+1;end;buffer[k]:=32;end;{:47}{49:}
function getinteger:integer;var x:integer;negative:boolean;
begin if buffer[bufptr]=45 then begin negative:=true;bufptr:=bufptr+1;
end else negative:=false;x:=0;
while(buffer[bufptr]>=48)and(buffer[bufptr]<=57)do begin x:=10*x+buffer[
bufptr]-48;bufptr:=bufptr+1;end;
if negative then getinteger:=-x else getinteger:=x;end;{:49}{50:}
procedure dialog;label 1,2,3,4,5;var k:integer;
begin writeln(stderr,'This is DVItype, C Version 3.4');{51:}
1:write(stderr,'Output level (default=4, ? for help): ');outmode:=4;
inputln;
if buffer[0]<>32 then if(buffer[0]>=48)and(buffer[0]<=52)then outmode:=
buffer[0]-48 else begin write(stderr,'Type 4 for complete listing,');
write(stderr,' 0 for errors only,');
writeln(stderr,' 1 or 2 or 3 for something in between.');goto 1;end{:51}
;{52:}2:write(stderr,'Starting page (default=*): ');startvals:=0;
startthere[0]:=false;inputln;bufptr:=0;k:=0;
if buffer[0]<>32 then repeat if buffer[bufptr]=42 then begin startthere[
k]:=false;bufptr:=bufptr+1;end else begin startthere[k]:=true;
startcount[k]:=getinteger;end;
if(k<9)and(buffer[bufptr]=46)then begin k:=k+1;bufptr:=bufptr+1;
end else if buffer[bufptr]=32 then startvals:=k else begin write(stderr,
'Type, e.g., 1.*.-5 to specify the ');
writeln(stderr,'first page with \count0=1, \count2=-5.');goto 2;end;
until startvals=k{:52};{53:}
3:write(stderr,'Maximum number of pages (default=1000000): ');
maxpages:=1000000;inputln;bufptr:=0;
if buffer[0]<>32 then begin maxpages:=getinteger;
if maxpages<=0 then begin writeln(stderr,
'Please type a positive number.');goto 3;end;end{:53};{54:}
4:write(stderr,'Assumed device resolution');
write(stderr,' in pixels per inch (default=300/1): ');resolution:=300.0;
inputln;bufptr:=0;if buffer[0]<>32 then begin k:=getinteger;
if(k>0)and(buffer[bufptr]=47)and(buffer[bufptr+1]>48)and(buffer[bufptr+1
]<=57)then begin bufptr:=bufptr+1;resolution:=k/getinteger;
end else begin write(stderr,'Type a ratio of positive integers;');
writeln(stderr,' (1 pixel per mm would be 254/10).');goto 4;end;end{:54}
;{55:}
5:write(stderr,'New magnification (default=0 to keep the old one): ');
newmag:=0;inputln;bufptr:=0;
if buffer[0]<>32 then if(buffer[0]>=48)and(buffer[0]<=57)then newmag:=
getinteger else begin write(stderr,
'Type a positive integer to override ');
writeln(stderr,'the magnification in the DVI file.');goto 5;end{:55};
{56:}writeln(stdout,'Options selected:');
write(stdout,'  Starting page = ');
for k:=0 to startvals do begin if startthere[k]then write(stdout,
startcount[k]:1)else write(stdout,'*');
if k<startvals then write(stdout,'.')else writeln(stdout,' ');end;
writeln(stdout,'  Maximum number of pages = ',maxpages:1);
write(stdout,'  Output level = ',outmode:1);
case outmode of 0:writeln(stdout,
' (showing bops, fonts, and error messages only)');
1:writeln(stdout,' (terse)');2:writeln(stdout,' (mnemonics)');
3:writeln(stdout,' (verbose)');
4:if true then writeln(stdout,' (the works)')else begin outmode:=3;
writeln(stdout,' (the works: same as level 3 in this DVItype)');end;end;
write(stdout,'  Resolution = ');printreal(resolution,12,8);
writeln(stdout,' pixels per inch');
if newmag>0 then begin write(stdout,'  New magnification factor = ');
printreal(newmag/1000.0,8,3);writeln(stdout,'')end{:56};end;{:50}{59:}
procedure definefont(e:integer);var f:0..maxfonts;p:integer;n:integer;
c,q,d,m:integer;r:0..PATHMAX;j,k:0..namesize;mismatch:boolean;
begin if nf=maxfonts then begin writeln(stderr,
'DVItype capacity exceeded (max fonts=',maxfonts:1,')!');uexit(1);end;
fontnum[nf]:=e;f:=0;while fontnum[f]<>e do f:=f+1;{61:}c:=signedquad;
fontchecksum[nf]:=c;q:=signedquad;fontscaledsize[nf]:=q;d:=signedquad;
fontdesignsize[nf]:=d;
if(q<=0)or(d<=0)then m:=1000 else m:=round((1000.0*conv*q)/(trueconv*d))
;p:=getbyte;n:=getbyte;
if fontname[nf]+n+p>namesize then begin writeln(stderr,
'DVItype capacity exceeded (name size=',namesize:1,')!');uexit(1);end;
fontname[nf+1]:=fontname[nf]+n+p;
if showing then write(stdout,': ')else write(stdout,'Font ',e:1,': ');
if n+p=0 then write(stdout,'null font name!')else for k:=fontname[nf]to
fontname[nf+1]-1 do names[k]:=getbyte;printfont(nf);
if not showing then if m<>1000 then write(stdout,' scaled ',m:1){:61};
if((outmode=4)and inpostamble)or((outmode<4)and not inpostamble)then
begin if f<nf then writeln(stdout,'---this font was already defined!');
end else begin if f=nf then writeln(stdout,
'---this font wasn''t loaded before!');end;if f=nf then{62:}begin{66:}
for k:=1 to PATHMAX do curname[k]:=' ';r:=0;
for k:=fontname[nf]to fontname[nf+1]-1 do begin r:=r+1;
if r+4>PATHMAX then begin writeln(stderr,
'DVItype capacity exceeded (max font name length=',PATHMAX:1,')!');
uexit(1);end;curname[r]:=xchr[names[k]];end;curname[r+1]:='.';
curname[r+2]:='t';curname[r+3]:='f';curname[r+4]:='m'{:66};opentfmfile;
if eof(tfmfile)then write(stdout,
'---not loaded, TFM file can''t be opened!')else begin if(q<=0)or(q>=
134217728)then write(stdout,'---not loaded, bad scale (',q:1,')!')else
if(d<=0)or(d>=134217728)then write(stdout,
'---not loaded, bad design size (',d:1,')!')else if inTFM(q)then{63:}
begin fontspace[nf]:=q div 6;
if(c<>0)and(tfmchecksum<>0)and(c<>tfmchecksum)then begin writeln(stdout,
'---beware: check sums do not agree!');
writeln(stdout,'   (',c:1,' vs. ',tfmchecksum:1,')');
write(stdout,'   ');end;
if abs(tfmdesignsize-d)>2 then begin writeln(stdout,
'---beware: design sizes do not agree!');
writeln(stdout,'   (',d:1,' vs. ',tfmdesignsize:1,')');
write(stdout,'   ');end;
write(stdout,'---loaded at size ',q:1,' DVI units');
d:=round((100.0*conv*q)/(trueconv*d));
if d<>100 then begin writeln(stdout,' ');
write(stdout,' (this font is magnified ',d:1,'%)');end;nf:=nf+1;end{:63}
;end;if outmode=0 then writeln(stdout,' ');end{:62}else{60:}
begin if fontchecksum[f]<>c then writeln(stdout,
'---check sum doesn''t match previous definition!');
if fontscaledsize[f]<>q then writeln(stdout,
'---scaled size doesn''t match previous definition!');
if fontdesignsize[f]<>d then writeln(stdout,
'---design size doesn''t match previous definition!');j:=fontname[f];
k:=fontname[nf];
if fontname[f+1]-j<>fontname[nf+1]-k then mismatch:=true else begin
mismatch:=false;
while j<fontname[f+1]do begin if names[j]<>names[k]then mismatch:=true;
j:=j+1;k:=k+1;end;end;
if mismatch then writeln(stdout,
'---font name doesn''t match previous definition!');end{:60};end;{:59}
{69:}procedure flushtext;var k:0..linelength;
begin if textptr>0 then begin if outmode>0 then begin write(stdout,'[');
for k:=1 to textptr do write(stdout,xchr[textbuf[k]]);
writeln(stdout,']');end;textptr:=0;end;end;{:69}{70:}
procedure outtext(c:ASCIIcode);
begin if textptr=linelength-2 then flushtext;textptr:=textptr+1;
textbuf[textptr]:=c;end;{:70}{75:}
function firstpar(o:eightbits):integer;
begin case o of 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,
22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,
46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,
70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,
94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
113,114,115,116,117,118,119,120,121,122,123,124,125,126,127:firstpar:=o
-0;128,133,235,239,243:firstpar:=getbyte;
129,134,236,240,244:firstpar:=gettwobytes;
130,135,237,241,245:firstpar:=getthreebytes;
143,148,153,157,162,167:firstpar:=signedbyte;
144,149,154,158,163,168:firstpar:=signedpair;
145,150,155,159,164,169:firstpar:=signedtrio;
131,132,136,137,146,151,156,160,165,170,238,242,246:firstpar:=signedquad
;138,139,140,141,142,247,248,249,250,251,252,253,254,255:firstpar:=0;
147:firstpar:=w;152:firstpar:=x;161:firstpar:=y;166:firstpar:=z;
171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,
189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
225,226,227,228,229,230,231,232,233,234:firstpar:=o-171;end;end;{:75}
{76:}function rulepixels(x:integer):integer;var n:integer;
begin n:=trunc(conv*x);
if n<conv*x then rulepixels:=n+1 else rulepixels:=n;end;{:76}{79:}{82:}
function specialcases(o:eightbits;p,a:integer):boolean;
label 46,44,30,9998;var q:integer;k:integer;badchar:boolean;
pure:boolean;vvv:integer;begin pure:=true;case o of{85:}
157,158,159,160:begin if abs(p)>=5*fontspace[curfont]then vv:=round(conv
*(v+p))else vv:=vv+round(conv*(p));if outmode>0 then begin flushtext;
showing:=true;write(stdout,a:1,': ','down',o-156:1,' ',p:1);end;goto 44;
end;161,162,163,164,165:begin y:=p;
if abs(p)>=5*fontspace[curfont]then vv:=round(conv*(v+p))else vv:=vv+
round(conv*(p));if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','y',o-161:1,' ',p:1);end;goto 44;end;
166,167,168,169,170:begin z:=p;
if abs(p)>=5*fontspace[curfont]then vv:=round(conv*(v+p))else vv:=vv+
round(conv*(p));if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','z',o-166:1,' ',p:1);end;goto 44;end;{:85}{86:}
171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,
189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
225,226,227,228,229,230,231,232,233,234:begin if outmode>0 then begin
flushtext;showing:=true;write(stdout,a:1,': ','fntnum',p:1);end;goto 46;
end;235,236,237,238:begin if outmode>0 then begin flushtext;
showing:=true;write(stdout,a:1,': ','fnt',o-234:1,' ',p:1);end;goto 46;
end;243,244,245,246:begin if outmode>0 then begin flushtext;
showing:=true;write(stdout,a:1,': ','fntdef',o-242:1,' ',p:1);end;
definefont(p);goto 30;end;{:86}239,240,241,242:{87:}
begin if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','xxx ''');end;badchar:=false;
if p<0 then if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','string of negative length!');
end else write(stdout,' ','string of negative length!');
for k:=1 to p do begin q:=getbyte;if(q<32)or(q>126)then badchar:=true;
if showing then write(stdout,xchr[q]);end;
if showing then write(stdout,'''');
if badchar then if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','non-ASCII character in xxx command!');
end else write(stdout,' ','non-ASCII character in xxx command!');
goto 30;end{:87};247:begin if not showing then begin flushtext;
showing:=true;write(stdout,a:1,': ','preamble command within a page!');
end else write(stdout,' ','preamble command within a page!');goto 9998;
end;248,249:begin if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','postamble command within a page!');
end else write(stdout,' ','postamble command within a page!');goto 9998;
end;others:begin if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','undefined command ',o:1,'!');
end else write(stdout,' ','undefined command ',o:1,'!');goto 30;end end;
44:{92:}
if(v>0)and(p>0)then if v>2147483647-p then begin if not showing then
begin flushtext;showing:=true;
write(stdout,a:1,': ','arithmetic overflow! parameter changed from ',p:1
,' to ',2147483647-v:1);
end else write(stdout,' ','arithmetic overflow! parameter changed from '
,p:1,' to ',2147483647-v:1);p:=2147483647-v;end;
if(v<0)and(p<0)then if-v>p+2147483647 then begin if not showing then
begin flushtext;showing:=true;
write(stdout,a:1,': ','arithmetic overflow! parameter changed from ',p:1
,' to ',(-v)-2147483647:1);
end else write(stdout,' ','arithmetic overflow! parameter changed from '
,p:1,' to ',(-v)-2147483647:1);p:=(-v)-2147483647;end;
vvv:=round(conv*(v+p));
if abs(vvv-vv)>2 then if vvv>vv then vv:=vvv-2 else vv:=vvv+2;
if showing then if outmode>2 then begin write(stdout,' v:=',v:1);
if p>=0 then write(stdout,'+');
write(stdout,p:1,'=',v+p:1,', vv:=',vv:1);end;v:=v+p;
if abs(v)>maxvsofar then begin if abs(v)>maxv+99 then begin if not
showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','warning: |v|>',maxv:1,'!');
end else write(stdout,' ','warning: |v|>',maxv:1,'!');maxv:=abs(v);end;
maxvsofar:=abs(v);end;goto 30{:92};46:{94:}fontnum[nf]:=p;curfont:=0;
while fontnum[curfont]<>p do curfont:=curfont+1;
if curfont=nf then begin curfont:=maxfonts;
if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','invalid font selection: font ',p:1,
' was never defined!');
end else write(stdout,' ','invalid font selection: font ',p:1,
' was never defined!');end;
if showing then if outmode>2 then begin write(stdout,' current font is '
);printfont(curfont);end;goto 30{:94};9998:pure:=false;
30:specialcases:=pure;end;{:82}function dopage:boolean;
label 41,42,43,45,30,9998,9999;var o:eightbits;p,q:integer;a:integer;
hhh:integer;begin curfont:=maxfonts;s:=0;h:=0;v:=0;w:=0;x:=0;y:=0;z:=0;
hh:=0;vv:=0;while true do{80:}begin a:=curloc;showing:=false;o:=getbyte;
p:=firstpar(o);
if eof(dvifile)then begin writeln(stderr,'Bad DVI file: ',
'the file ended prematurely','!');uexit(1);end;{81:}if o<128 then{88:}
begin if(o>32)and(o<=126)then begin outtext(p);
if outmode>1 then begin showing:=true;
write(stdout,a:1,': ','setchar',p:1);end;
end else if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','setchar',p:1);end;goto 41;end{:88}
else case o of 128,129,130,131:begin if outmode>0 then begin flushtext;
showing:=true;write(stdout,a:1,': ','set',o-127:1,' ',p:1);end;goto 41;
end;133,134,135,136:begin if outmode>0 then begin flushtext;
showing:=true;write(stdout,a:1,': ','put',o-132:1,' ',p:1);end;goto 41;
end;132:begin if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','setrule');end;goto 42;end;
137:begin if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','putrule');end;goto 42;end;{83:}
138:begin if outmode>1 then begin showing:=true;
write(stdout,a:1,': ','nop');end;goto 30;end;
139:begin if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','bop occurred before eop!');
end else write(stdout,' ','bop occurred before eop!');goto 9998;end;
140:begin if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','eop');end;
if s<>0 then if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','stack not empty at end of page (level ',s:1,')!')
;end else write(stdout,' ','stack not empty at end of page (level ',s:1,
')!');dopage:=true;writeln(stdout,' ');goto 9999;end;
141:begin if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','push');end;
if s=maxssofar then begin maxssofar:=s+1;
if s=maxs then if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','deeper than claimed in postamble!');
end else write(stdout,' ','deeper than claimed in postamble!');
if s=stacksize then begin if not showing then begin flushtext;
showing:=true;
write(stdout,a:1,': ','DVItype capacity exceeded (stack size=',stacksize
:1,')');
end else write(stdout,' ','DVItype capacity exceeded (stack size=',
stacksize:1,')');goto 9998;end;end;hstack[s]:=h;vstack[s]:=v;
wstack[s]:=w;xstack[s]:=x;ystack[s]:=y;zstack[s]:=z;hhstack[s]:=hh;
vvstack[s]:=vv;s:=s+1;ss:=s-1;goto 45;end;
142:begin if outmode>0 then begin flushtext;showing:=true;
write(stdout,a:1,': ','pop');end;
if s=0 then if not showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','(illegal at level zero)!');
end else write(stdout,' ','(illegal at level zero)!')else begin s:=s-1;
hh:=hhstack[s];vv:=vvstack[s];h:=hstack[s];v:=vstack[s];w:=wstack[s];
x:=xstack[s];y:=ystack[s];z:=zstack[s];end;ss:=s;goto 45;end;{:83}{84:}
143,144,145,146:begin if(p>=fontspace[curfont])or(p<=-4*fontspace[
curfont])then begin outtext(32);hh:=round(conv*(h+p));
end else hh:=hh+round(conv*(p));if outmode>1 then begin showing:=true;
write(stdout,a:1,': ','right',o-142:1,' ',p:1);end;q:=p;goto 43;end;
147,148,149,150,151:begin w:=p;
if(p>=fontspace[curfont])or(p<=-4*fontspace[curfont])then begin outtext(
32);hh:=round(conv*(h+p));end else hh:=hh+round(conv*(p));
if outmode>1 then begin showing:=true;
write(stdout,a:1,': ','w',o-147:1,' ',p:1);end;q:=p;goto 43;end;
152,153,154,155,156:begin x:=p;
if(p>=fontspace[curfont])or(p<=-4*fontspace[curfont])then begin outtext(
32);hh:=round(conv*(h+p));end else hh:=hh+round(conv*(p));
if outmode>1 then begin showing:=true;
write(stdout,a:1,': ','x',o-152:1,' ',p:1);end;q:=p;goto 43;end;{:84}
others:if specialcases(o,p,a)then goto 30 else goto 9998 end{:81};
41:{89:}
if p<0 then p:=255-((-1-p)mod 256)else if p>=256 then p:=p mod 256;
if(p<fontbc[curfont])or(p>fontec[curfont])then q:=2147483647 else q:=
width[widthbase[curfont]+p];
if q=2147483647 then begin if not showing then begin flushtext;
showing:=true;
write(stdout,a:1,': ','character ',p:1,' invalid in font ');
end else write(stdout,' ','character ',p:1,' invalid in font ');
printfont(curfont);if curfont<>maxfonts then write(stdout,'!');end;
if o>=133 then goto 30;
if q=2147483647 then q:=0 else hh:=hh+pixelwidth[widthbase[curfont]+p];
goto 43{:89};42:{90:}q:=signedquad;
if showing then begin write(stdout,' height ',p:1,', width ',q:1);
if outmode>2 then if(p<=0)or(q<=0)then write(stdout,' (invisible)')else
write(stdout,' (',rulepixels(p):1,'x',rulepixels(q):1,' pixels)');end;
if o=137 then goto 30;
if showing then if outmode>2 then writeln(stdout,' ');
hh:=hh+rulepixels(q);goto 43{:90};43:{91:}
if(h>0)and(q>0)then if h>2147483647-q then begin if not showing then
begin flushtext;showing:=true;
write(stdout,a:1,': ','arithmetic overflow! parameter changed from ',q:1
,' to ',2147483647-h:1);
end else write(stdout,' ','arithmetic overflow! parameter changed from '
,q:1,' to ',2147483647-h:1);q:=2147483647-h;end;
if(h<0)and(q<0)then if-h>q+2147483647 then begin if not showing then
begin flushtext;showing:=true;
write(stdout,a:1,': ','arithmetic overflow! parameter changed from ',q:1
,' to ',(-h)-2147483647:1);
end else write(stdout,' ','arithmetic overflow! parameter changed from '
,q:1,' to ',(-h)-2147483647:1);q:=(-h)-2147483647;end;
hhh:=round(conv*(h+q));
if abs(hhh-hh)>2 then if hhh>hh then hh:=hhh-2 else hh:=hhh+2;
if showing then if outmode>2 then begin write(stdout,' h:=',h:1);
if q>=0 then write(stdout,'+');
write(stdout,q:1,'=',h+q:1,', hh:=',hh:1);end;h:=h+q;
if abs(h)>maxhsofar then begin if abs(h)>maxh+99 then begin if not
showing then begin flushtext;showing:=true;
write(stdout,a:1,': ','warning: |h|>',maxh:1,'!');
end else write(stdout,' ','warning: |h|>',maxh:1,'!');maxh:=abs(h);end;
maxhsofar:=abs(h);end;goto 30{:91};45:{93:}
if showing then if outmode>2 then begin writeln(stdout,' ');
write(stdout,'level ',ss:1,':(h=',h:1,',v=',v:1,',w=',w:1,',x=',x:1,
',y=',y:1,',z=',z:1,',hh=',hh:1,',vv=',vv:1,')');end;goto 30{:93};
30:if showing then writeln(stdout,' ');end{:80};
9998:writeln(stdout,'!');dopage:=false;9999:end;{:79}{95:}
procedure skippages;label 9999;var p:integer;k:0..255;
downthedrain:integer;begin showing:=false;
while true do begin if eof(dvifile)then begin writeln(stderr,
'Bad DVI file: ','the file ended prematurely','!');uexit(1);end;
k:=getbyte;p:=firstpar(k);case k of 139:begin{98:}
newbackpointer:=curloc-1;pagecount:=pagecount+1;
for k:=0 to 9 do count[k]:=signedquad;
if signedquad<>oldbackpointer then writeln(stdout,'backpointer in byte '
,curloc-4:1,' should be ',oldbackpointer:1,'!');
oldbackpointer:=newbackpointer{:98};
if not started and startmatch then begin started:=true;goto 9999;end;
end;132,137:downthedrain:=signedquad;
243,244,245,246:begin definefont(p);writeln(stdout,' ');end;
239,240,241,242:while p>0 do begin downthedrain:=getbyte;p:=p-1;end;
248:begin inpostamble:=true;goto 9999;end;others:end;end;9999:end;{:95}
{102:}procedure readpostamble;var k:integer;p,q,m:integer;
begin showing:=false;postloc:=curloc-5;
writeln(stdout,'Postamble starts at byte ',postloc:1,'.');
if signedquad<>numerator then writeln(stdout,
'numerator doesn''t match the preamble!');
if signedquad<>denominator then writeln(stdout,
'denominator doesn''t match the preamble!');
if signedquad<>mag then if newmag=0 then writeln(stdout,
'magnification doesn''t match the preamble!');maxv:=signedquad;
maxh:=signedquad;write(stdout,'maxv=',maxv:1,', maxh=',maxh:1);
maxs:=gettwobytes;totalpages:=gettwobytes;
writeln(stdout,', maxstackdepth=',maxs:1,', totalpages=',totalpages:1);
if outmode<4 then{103:}begin if maxv+99<maxvsofar then writeln(stdout,
'warning: observed maxv was ',maxvsofar:1);
if maxh+99<maxhsofar then writeln(stdout,'warning: observed maxh was ',
maxhsofar:1);if maxs<maxssofar then writeln(stdout,
'warning: observed maxstackdepth was ',maxssofar:1);
if pagecount<>totalpages then writeln(stdout,'there are really ',
pagecount:1,' pages, not ',totalpages:1,'!');end{:103};{105:}
repeat k:=getbyte;if(k>=243)and(k<247)then begin p:=firstpar(k);
definefont(p);writeln(stdout,' ');k:=138;end;until k<>138;
if k<>249 then writeln(stdout,'byte ',curloc-1:1,' is not postpost!'){:
105};{104:}q:=signedquad;
if q<>postloc then writeln(stdout,'bad postamble pointer in byte ',
curloc-4:1,'!');m:=getbyte;
if m<>2 then writeln(stdout,'identification in byte ',curloc-1:1,
' should be ',2:1,'!');k:=curloc;m:=223;
while(m=223)and not eof(dvifile)do m:=getbyte;
if not eof(dvifile)then begin writeln(stderr,'Bad DVI file: ',
'signature in byte ',curloc-1:1,' should be 223','!');uexit(1);
end else if curloc<k+4 then writeln(stdout,
'not enough signature bytes at end of file (',curloc-k:1,')');{:104};
end;{:102}{106:}begin initialize;
if argc<>2 then begin writeln('Usage: dvitype <dvi file>.');uexit(1);
end;dialog;{108:}opendvifile;p:=getbyte;
if p<>247 then begin writeln(stderr,'Bad DVI file: ',
'First byte isn''t start of preamble!','!');uexit(1);end;p:=getbyte;
if p<>2 then writeln(stdout,'identification in byte 1 should be ',2:1,
'!');{109:}numerator:=signedquad;denominator:=signedquad;
if numerator<=0 then begin writeln(stderr,'Bad DVI file: ',
'numerator is ',numerator:1,'!');uexit(1);end;
if denominator<=0 then begin writeln(stderr,'Bad DVI file: ',
'denominator is ',denominator:1,'!');uexit(1);end;
writeln(stdout,'numerator/denominator=',numerator:1,'/',denominator:1);
tfmconv:=(25400000.0/numerator)*(denominator/473628672)/16.0;
conv:=(numerator/254000.0)*(resolution/denominator);mag:=signedquad;
if newmag>0 then mag:=newmag else if mag<=0 then begin writeln(stderr,
'Bad DVI file: ','magnification is ',mag:1,'!');uexit(1);end;
trueconv:=conv;conv:=trueconv*(mag/1000.0);
write(stdout,'magnification=',mag:1,'; ');printreal(conv,16,8);
writeln(stdout,' pixels per DVI unit'){:109};p:=getbyte;
write(stdout,'''');while p>0 do begin p:=p-1;
write(stdout,xchr[getbyte]);end;writeln(stdout,''''){:108};
if outmode=4 then begin{99:}n:=dvilength;
if n<53 then begin writeln(stderr,'Bad DVI file: ','only ',n:1,
' bytes long','!');uexit(1);end;m:=n-4;
repeat if m=0 then begin writeln(stderr,'Bad DVI file: ','all 223s','!')
;uexit(1);end;movetobyte(m);k:=getbyte;m:=m-1;until k<>223;
if k<>2 then begin writeln(stderr,'Bad DVI file: ','ID byte is ',k:1,'!'
);uexit(1);end;movetobyte(m-3);q:=signedquad;
if(q<0)or(q>m-33)then begin writeln(stderr,'Bad DVI file: ',
'post pointer ',q:1,' at byte ',m-3:1,'!');uexit(1);end;movetobyte(q);
k:=getbyte;
if k<>248 then begin writeln(stderr,'Bad DVI file: ','byte ',q:1,
' is not post','!');uexit(1);end;postloc:=q;
firstbackpointer:=signedquad{:99};inpostamble:=true;readpostamble;
inpostamble:=false;{101:}q:=postloc;p:=firstbackpointer;startloc:=-1;
if p<0 then inpostamble:=true else begin repeat if p>q-46 then begin
writeln(stderr,'Bad DVI file: ','page link ',p:1,' after byte ',q:1,'!')
;uexit(1);end;q:=p;movetobyte(q);k:=getbyte;
if k=139 then pagecount:=pagecount+1 else begin writeln(stderr,
'Bad DVI file: ','byte ',q:1,' is not bop','!');uexit(1);end;
for k:=0 to 9 do count[k]:=signedquad;if startmatch then startloc:=q;
p:=signedquad;until p<0;if startloc<0 then begin writeln(stderr,
'starting page number could not be found!');uexit(1);end;
movetobyte(startloc+1);oldbackpointer:=startloc;
for k:=0 to 9 do count[k]:=signedquad;p:=signedquad;started:=true;end;
if pagecount<>totalpages then writeln(stdout,'there are really ',
pagecount:1,' pages, not ',totalpages:1,'!'){:101};end else skippages;
if not inpostamble then{110:}
begin while maxpages>0 do begin maxpages:=maxpages-1;
writeln(stdout,' ');write(stdout,curloc-45:1,': beginning of page ');
for k:=0 to startvals do begin write(stdout,count[k]:1);
if k<startvals then write(stdout,'.')else writeln(stdout,' ');end;
if not dopage then begin writeln(stderr,'Bad DVI file: ',
'page ended unexpectedly','!');uexit(1);end;repeat k:=getbyte;
if(k>=243)and(k<247)then begin p:=firstpar(k);definefont(p);k:=138;end;
until k<>138;if k=248 then begin inpostamble:=true;goto 30;end;
if k<>139 then begin writeln(stderr,'Bad DVI file: ','byte ',curloc-1:1,
' is not bop','!');uexit(1);end;{98:}newbackpointer:=curloc-1;
pagecount:=pagecount+1;for k:=0 to 9 do count[k]:=signedquad;
if signedquad<>oldbackpointer then writeln(stdout,'backpointer in byte '
,curloc-4:1,' should be ',oldbackpointer:1,'!');
oldbackpointer:=newbackpointer{:98};end;30:end{:110};
if outmode<4 then begin if not inpostamble then skippages;
if signedquad<>oldbackpointer then writeln(stdout,'backpointer in byte '
,curloc-4:1,' should be ',oldbackpointer:1,'!');readpostamble;end;
writeln(stdout,' ');end.{:106}
