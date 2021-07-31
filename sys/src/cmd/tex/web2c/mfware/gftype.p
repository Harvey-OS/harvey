{3:}program GFtype;const{5:}linelength=79;maxrow=79;maxcol=79;{:5}
type{8:}ASCIIcode=32..126;{:8}{9:}textfile=packed file of char;{:9}{20:}
eightbits=0..255;bytefile=packed file of eightbits;
UNIXfilename=packed array[1..PATHMAX]of char;{:20}{36:}pixel=0..1;{:36}
var{10:}xord:array[char]of ASCIIcode;xchr:array[0..255]of char;{:10}
{21:}gffile:bytefile;{:21}{23:}curloc:integer;{:23}{25:}
wantsmnemonics:boolean;wantspixels:boolean;{:25}{35:}m,n:integer;
paintswitch:pixel;{:35}{37:}
imagearray:packed array[0..maxcol,0..maxrow]of pixel;{:37}{39:}
maxsubrow,maxsubcol:integer;{:39}{41:}
minmstated,maxmstated,minnstated,maxnstated:integer;
maxmobserved,maxnobserved:integer;
minmoverall,maxmoverall,minnoverall,maxnoverall:integer;{:41}{46:}
totalchars:integer;charptr:array[0..255]of integer;gfprevptr:integer;
charactercode:integer;{:46}{54:}badchar:boolean;{:54}{62:}
designsize,checksum:integer;hppp,vppp:integer;postloc:integer;
pixratio:real;{:62}{67:}a:integer;b,c,l,o,p,q,r:integer;{:67}
procedure initialize;var i:integer;begin setpaths(GFFILEPATHBIT);
writeln(stdout,'This is GFtype, C Version 3.1');{11:}
for i:=0 to 31 do xchr[i]:='?';xchr[32]:=' ';xchr[33]:='!';
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
for i:=0 to 127 do xord[chr(i)]:=32;
for i:=32 to 126 do xord[xchr[i]]:=i;{:12}{26:}wantsmnemonics:=false;
wantspixels:=false;{:26}{47:}for i:=0 to 255 do charptr[i]:=-1;
totalchars:=0;{:47}{63:}minmoverall:=536870911;maxmoverall:=-536870911;
minnoverall:=536870911;maxnoverall:=-536870911;{:63}end;{:3}{22:}
procedure opengffile;var j,k:integer;gfname:UNIXfilename;begin k:=1;
if(argc<2)or(argc>4)then begin writeln(stdout,
'Usage: gftype [-m] [-i] <gf file>.');uexit(1);end;argv(1,gfname);
while gfname[1]=xchr[45]do begin k:=k+1;
wantsmnemonics:=wantsmnemonics or(gfname[2]=xchr[109])or(gfname[3]=xchr[
109]);
wantspixels:=wantspixels or(gfname[2]=xchr[105])or(gfname[3]=xchr[105]);
if wantspixels or wantsmnemonics then argv(k,gfname)else begin writeln(
stdout,'Usage: gftype [-m] [-i] <gf file>.');uexit(1);end;end;
if testreadaccess(gfname,GFFILEPATH)then begin reset(gffile,gfname);
curloc:=0;end else begin printpascalstring(gfname);
begin writeln(stdout,': GF file not found');uexit(1);end;end;{34:}
write(stdout,'Options selected: Mnemonic output = ');
if wantsmnemonics then write(stdout,'true')else write(stdout,'false');
write(stdout,'; pixel output = ');
if wantspixels then write(stdout,'true')else write(stdout,'false');
writeln(stdout,'.'){:34};end;{:22}{24:}function getbyte:integer;
var b:eightbits;
begin if eof(gffile)then getbyte:=0 else begin read(gffile,b);
curloc:=curloc+1;getbyte:=b;end;end;function gettwobytes:integer;
var a,b:eightbits;begin read(gffile,a);read(gffile,b);curloc:=curloc+2;
gettwobytes:=a*256+b;end;function getthreebytes:integer;
var a,b,c:eightbits;begin read(gffile,a);read(gffile,b);read(gffile,c);
curloc:=curloc+3;getthreebytes:=(a*256+b)*256+c;end;
function signedquad:integer;var a,b,c,d:eightbits;begin read(gffile,a);
read(gffile,b);read(gffile,c);read(gffile,d);curloc:=curloc+4;
if a<128 then signedquad:=((a*256+b)*256+c)*256+d else signedquad:=(((a
-256)*256+b)*256+c)*256+d;end;{:24}{45:}
procedure printscaled(s:integer);var delta:integer;
begin if s<0 then begin write(stdout,'-');s:=-s;end;
write(stdout,s div 65536:1);s:=10*(s mod 65536)+5;
if s<>5 then begin delta:=10;write(stdout,'.');
repeat if delta>65536 then s:=s+32768-(delta div 2);
write(stdout,xchr[ord('0')+(s div 65536)]);s:=10*(s mod 65536);
delta:=delta*10;until s<=delta;end;end;{:45}{48:}
function firstpar(o:eightbits):integer;
begin case o of 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,
22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,
46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63:firstpar:=o-0;
64,71,245,246,239:firstpar:=getbyte;65,72,240:firstpar:=gettwobytes;
66,73,241:firstpar:=getthreebytes;242,243:firstpar:=signedquad;
67,68,69,70,244,247,248,249,250,251,252,253,254,255:firstpar:=0;
74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,
98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
134,135,136,137:firstpar:=o-74;
138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,
156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,
174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201:firstpar:=o-74;
202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,
238:firstpar:=o-74;end;end;{:48}{49:}function dochar:boolean;
label 9998,9999;var o:eightbits;p,q:integer;aok:boolean;begin aok:=true;
while true do{50:}begin a:=curloc;o:=getbyte;p:=firstpar(o);
if eof(gffile)then begin writeln(stdout,'Bad GF file: ',
'the file ended prematurely','!');uexit(1);end;{51:}if o<=67 then{56:}
begin if wantsmnemonics then write(stdout,' paint ');repeat{57:}
if wantsmnemonics then if paintswitch=0 then write(stdout,'(',p:1,')')
else write(stdout,p:1);m:=m+p;if m>maxmobserved then maxmobserved:=m-1;
if wantspixels then{58:}
if paintswitch=1 then if n<=maxsubrow then begin l:=m-p;r:=m-1;
if r>maxsubcol then r:=maxsubcol;m:=l;
while m<=r do begin imagearray[m,n]:=1;m:=m+1;end;m:=l+p;end{:58};
paintswitch:=1-paintswitch{:57};a:=curloc;o:=getbyte;p:=firstpar(o);
if eof(gffile)then begin writeln(stdout,'Bad GF file: ',
'the file ended prematurely','!');uexit(1);end;until o>67;end{:56};
case o of 70,71,72,73:{60:}
begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','skip',(o-70)mod 4:1,' ',p:1);end;n:=n+p+1;m:=0;
paintswitch:=0;
if wantsmnemonics then write(stdout,' (n=',maxnstated-n:1,')');end{:60};
74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,
98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
134,135,136,137:{59:}begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','newrow ',p:1);end;n:=n+1;m:=p;paintswitch:=1;
if wantsmnemonics then write(stdout,' (n=',maxnstated-n:1,')');end{:59};
138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,
156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,
174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201:{59:}
begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','newrow ',p:1);end;n:=n+1;m:=p;paintswitch:=1;
if wantsmnemonics then write(stdout,' (n=',maxnstated-n:1,')');end{:59};
202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,
238:{59:}begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','newrow ',p:1);end;n:=n+1;m:=p;paintswitch:=1;
if wantsmnemonics then write(stdout,' (n=',maxnstated-n:1,')');end{:59};
{52:}244:if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','no op');end;
247:begin begin write(stdout,a:1,': ','! ',
'preamble command within a character!');writeln(stdout);end;goto 9998;
end;248,249:begin begin write(stdout,a:1,': ','! ',
'postamble command within a character!');writeln(stdout);end;goto 9998;
end;
67,68:begin begin write(stdout,a:1,': ','! ','boc occurred before eoc!')
;writeln(stdout);end;goto 9998;end;
69:begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','eoc');end;writeln(stdout);goto 9999;end;{:52}
239,240,241,242:{53:}begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','xxx ''');end;badchar:=false;b:=16;
if p<0 then begin writeln(stdout);
write(stdout,a:1,': ','! ','string of negative length!');
writeln(stdout);end;while p>0 do begin q:=getbyte;
if(q<32)or(q>126)then badchar:=true;
if wantsmnemonics then begin write(stdout,xchr[q]);
if b<linelength then b:=b+1 else begin writeln(stdout);b:=2;end;end;
p:=p-1;end;if wantsmnemonics then write(stdout,'''');
if badchar then begin writeln(stdout);
write(stdout,a:1,': ','! ','non-ASCII character in xxx command!');
writeln(stdout);end;end{:53};243:{55:}
begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','yyy ',p:1,' (');end;
if wantsmnemonics then begin printscaled(p);write(stdout,')');end;
end{:55};
others:begin write(stdout,a:1,': ','! ','undefined command ',o:1,'!');
writeln(stdout);end end{:51};end{:50};9998:writeln(stdout,'!');
aok:=false;9999:dochar:=aok;end;{:49}{61:}procedure readpostamble;
var k:integer;p,q,m,u,v,w,c:integer;begin postloc:=curloc-1;
write(stdout,'Postamble starts at byte ',postloc:1);
if postloc=gfprevptr then writeln(stdout,'.')else writeln(stdout,
', after special info at byte ',gfprevptr:1,'.');p:=signedquad;
if p<>gfprevptr then begin write(stdout,a:1,': ','! ',
'backpointer in byte ',curloc-4:1,' should be ',gfprevptr:1,' not ',p:1,
'!');writeln(stdout);end;designsize:=signedquad;checksum:=signedquad;
write(stdout,'design size = ',designsize:1,' (');
printscaled(designsize div 16);writeln(stdout,'pt)');
writeln(stdout,'check sum = ',checksum:1);hppp:=signedquad;
vppp:=signedquad;write(stdout,'hppp = ',hppp:1,' (');printscaled(hppp);
writeln(stdout,')');write(stdout,'vppp = ',vppp:1,' (');
printscaled(vppp);writeln(stdout,')');
pixratio:=(designsize/1048576)*(hppp/1048576);minmstated:=signedquad;
maxmstated:=signedquad;minnstated:=signedquad;maxnstated:=signedquad;
writeln(stdout,'min m = ',minmstated:1,', max m = ',maxmstated:1);
if minmstated>minmoverall then begin write(stdout,a:1,': ','! ',
'min m should be <=',minmoverall:1,'!');writeln(stdout);end;
if maxmstated<maxmoverall then begin write(stdout,a:1,': ','! ',
'max m should be >=',maxmoverall:1,'!');writeln(stdout);end;
writeln(stdout,'min n = ',minnstated:1,', max n = ',maxnstated:1);
if minnstated>minnoverall then begin write(stdout,a:1,': ','! ',
'min n should be <=',minnoverall:1,'!');writeln(stdout);end;
if maxnstated<maxnoverall then begin write(stdout,a:1,': ','! ',
'max n should be >=',maxnoverall:1,'!');writeln(stdout);end;{65:}
repeat a:=curloc;k:=getbyte;if(k=245)or(k=246)then begin c:=firstpar(k);
if k=245 then begin u:=signedquad;v:=signedquad;
end else begin u:=getbyte*65536;v:=0;end;w:=signedquad;p:=signedquad;
write(stdout,'Character ',c:1,': dx ',u:1,' (');printscaled(u);
if v<>0 then begin write(stdout,'), dy ',v:1,' (');printscaled(v);end;
write(stdout,'), width ',w:1,' (');w:=round(w*pixratio);printscaled(w);
writeln(stdout,'), loc ',p:1);
if charptr[c]=0 then begin write(stdout,a:1,': ','! ',
'duplicate locator for this character!');writeln(stdout);
end else if p<>charptr[c]then begin write(stdout,a:1,': ','! ',
'character location should be ',charptr[c]:1,'!');writeln(stdout);end;
charptr[c]:=0;k:=244;end;until k<>244{:65};{64:}
if k<>249 then begin write(stdout,a:1,': ','! ','should be postpost!');
writeln(stdout);end;
for k:=0 to 255 do if charptr[k]>0 then begin write(stdout,a:1,': ','! '
,'missing locator for character ',k:1,'!');writeln(stdout);end;
q:=signedquad;if q<>postloc then begin write(stdout,a:1,': ','! ',
'postamble pointer should be ',postloc:1,' not ',q:1,'!');
writeln(stdout);end;m:=getbyte;
if m<>131 then begin write(stdout,a:1,': ','! ',
'identification byte should be ',131:1,', not ',m:1,'!');
writeln(stdout);end;k:=curloc;m:=223;
while(m=223)and not eof(gffile)do m:=getbyte;
if not eof(gffile)then begin writeln(stdout,'Bad GF file: ',
'signature in byte ',curloc-1:1,' should be 223','!');uexit(1);
end else if curloc<k+4 then begin write(stdout,a:1,': ','! ',
'not enough signature bytes at end of file!');writeln(stdout);end{:64};
end;{:61}{66:}begin initialize;{68:}opengffile;o:=getbyte;
if o<>247 then begin writeln(stdout,'Bad GF file: ',
'First byte isn''t start of preamble!','!');uexit(1);end;o:=getbyte;
if o<>131 then begin writeln(stdout,'Bad GF file: ',
'identification byte should be ',131:1,' not ',o:1,'!');uexit(1);end;
o:=getbyte;write(stdout,'''');while o>0 do begin o:=o-1;
write(stdout,xchr[getbyte]);end;writeln(stdout,'''');{:68};{69:}
repeat gfprevptr:=curloc;{70:}repeat a:=curloc;o:=getbyte;
p:=firstpar(o);if eof(gffile)then begin writeln(stdout,'Bad GF file: ',
'the file ended prematurely','!');uexit(1);end;if o=243 then begin{55:}
begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','yyy ',p:1,' (');end;
if wantsmnemonics then begin printscaled(p);write(stdout,')');end;
end{:55};o:=244;end else if(o>=239)and(o<=242)then begin{53:}
begin if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','xxx ''');end;badchar:=false;b:=16;
if p<0 then begin writeln(stdout);
write(stdout,a:1,': ','! ','string of negative length!');
writeln(stdout);end;while p>0 do begin q:=getbyte;
if(q<32)or(q>126)then badchar:=true;
if wantsmnemonics then begin write(stdout,xchr[q]);
if b<linelength then b:=b+1 else begin writeln(stdout);b:=2;end;end;
p:=p-1;end;if wantsmnemonics then write(stdout,'''');
if badchar then begin writeln(stdout);
write(stdout,a:1,': ','! ','non-ASCII character in xxx command!');
writeln(stdout);end;end{:53};o:=244;
end else if o=244 then if wantsmnemonics then begin writeln(stdout);
write(stdout,a:1,': ','no op');end;until o<>244;{:70};
if o<>248 then begin if o<>67 then if o<>68 then begin writeln(stdout,
'Bad GF file: ','byte ',curloc-1:1,' is not boc (',o:1,')','!');
uexit(1);end;writeln(stdout);
write(stdout,curloc-1:1,': beginning of char ');{71:}a:=curloc-1;
totalchars:=totalchars+1;if o=67 then begin charactercode:=signedquad;
p:=signedquad;c:=charactercode mod 256;if c<0 then c:=c+256;
minmstated:=signedquad;maxmstated:=signedquad;minnstated:=signedquad;
maxnstated:=signedquad;end else begin charactercode:=getbyte;p:=-1;
c:=charactercode;q:=getbyte;maxmstated:=getbyte;
minmstated:=maxmstated-q;q:=getbyte;maxnstated:=getbyte;
minnstated:=maxnstated-q;end;write(stdout,c:1);
if charactercode<>c then write(stdout,' with extension ',(charactercode-
c)div 256:1);
if wantsmnemonics then writeln(stdout,': ',minmstated:1,'<=m<=',
maxmstated:1,' ',minnstated:1,'<=n<=',maxnstated:1);maxmobserved:=-1;
if charptr[c]<>p then begin write(stdout,a:1,': ','! ',
'previous character pointer should be ',charptr[c]:1,', not ',p:1,'!');
writeln(stdout);
end else if p>0 then if wantsmnemonics then writeln(stdout,
'(previous character with the same code started at byte ',p:1,')');
charptr[c]:=gfprevptr;
if wantsmnemonics then write(stdout,'(initially n=',maxnstated:1,')');
if wantspixels then{38:}begin maxsubcol:=maxmstated-minmstated-1;
if maxsubcol>maxcol then maxsubcol:=maxcol;
maxsubrow:=maxnstated-minnstated;
if maxsubrow>maxrow then maxsubrow:=maxrow;n:=0;
while n<=maxsubrow do begin m:=0;
while m<=maxsubcol do begin imagearray[m,n]:=0;m:=m+1;end;n:=n+1;end;
end{:38};m:=0;n:=0;paintswitch:=0;{:71};
if not dochar then begin writeln(stdout,'Bad GF file: ',
'char ended unexpectedly','!');uexit(1);end;maxnobserved:=n;
if wantspixels then{40:}begin{42:}
if(maxmobserved>maxcol)or(maxnobserved>maxrow)then writeln(stdout,
'(The character is too large to be displayed in full.)');
if maxsubcol>maxmobserved then maxsubcol:=maxmobserved;
if maxsubrow>maxnobserved then maxsubrow:=maxnobserved;{:42};
if maxsubcol>=0 then{43:}
begin writeln(stdout,'.<--This pixel''s lower left corner is at (',
minmstated:1,',',maxnstated+1:1,') in METAFONT coordinates');n:=0;
while n<=maxsubrow do begin m:=0;b:=0;
while m<=maxsubcol do begin if imagearray[m,n]=0 then b:=b+1 else begin
while b>0 do begin write(stdout,' ');b:=b-1;end;write(stdout,'*');end;
m:=m+1;end;writeln(stdout);n:=n+1;end;
writeln(stdout,'.<--This pixel''s upper left corner is at (',minmstated:
1,',',maxnstated-maxsubrow:1,') in METAFONT coordinates');end{:43}
else writeln(stdout,'(The character is entirely blank.)');end{:40};{72:}
maxmobserved:=minmstated+maxmobserved+1;n:=maxnstated-maxnobserved;
if minmstated<minmoverall then minmoverall:=minmstated;
if maxmobserved>maxmoverall then maxmoverall:=maxmobserved;
if n<minnoverall then minnoverall:=n;
if maxnstated>maxnoverall then maxnoverall:=maxnstated;
if maxmobserved>maxmstated then writeln(stdout,
'The previous character should have had max m >= ',maxmobserved:1,'!');
if n<minnstated then writeln(stdout,
'The previous character should have had min n <= ',n:1,'!'){:72};end;
until o=248;{:69};writeln(stdout);readpostamble;
write(stdout,'The file had ',totalchars:1,' character');
if totalchars<>1 then write(stdout,'s');writeln(stdout,' altogether.');
end.{:66}
