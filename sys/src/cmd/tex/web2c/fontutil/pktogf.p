{4:}program PKtoGF;const{6:}maxcounts=400;{:6}type{9:}ASCIIcode=32..126;
{:9}{10:}textfile=packed file of char;{:10}{38:}eightbits=0..255;
bytefile=packed file of eightbits;{:38}var{11:}
xord:array[char]of ASCIIcode;xchr:array[0..255]of char;{:11}{39:}
gffile,pkfile:bytefile;{:39}{41:}
gfname,pkname:packed array[1..PATHMAX]of char;gfloc,curloc:integer;
gfarg:integer;verbose:boolean;{:41}{48:}i,j:integer;endofpacket:integer;
dynf:integer;car:integer;tfmwidth:integer;xoff,yoff:integer;{:48}{50:}
comment:packed array[1..17]of char;magnification:integer;
designsize:integer;checksum:integer;hppp,vppp:integer;{:50}{55:}
cheight,cwidth:integer;wordwidth:integer;horesc,veresc:integer;
packetlength:integer;lasteoc:integer;{:55}{57:}
minm,maxm,minn,maxn:integer;mminm,mmaxm,mminn,mmaxn:integer;
charpointer,stfmwidth:array[0..255]of integer;
shoresc,sveresc:array[0..255]of integer;thischarptr:integer;{:57}{63:}
inputbyte:eightbits;bitweight:eightbits;
rowcounts:array[0..maxcounts]of integer;rcp:integer;{:63}{67:}
countdown:integer;done:boolean;max:integer;repeatcount:integer;
xtogo,ytogo:integer;turnon,firston:boolean;count:integer;curn:integer;
{:67}{69:}flagbyte:integer;{:69}procedure initialize;var i:integer;
begin setpaths(PKFILEPATHBIT);{12:}for i:=0 to 31 do xchr[i]:='?';
xchr[32]:=' ';xchr[33]:='!';xchr[34]:='"';xchr[35]:='#';xchr[36]:='$';
xchr[37]:='%';xchr[38]:='&';xchr[39]:='''';xchr[40]:='(';xchr[41]:=')';
xchr[42]:='*';xchr[43]:='+';xchr[44]:=',';xchr[45]:='-';xchr[46]:='.';
xchr[47]:='/';xchr[48]:='0';xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';
xchr[52]:='4';xchr[53]:='5';xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';
xchr[57]:='9';xchr[58]:=':';xchr[59]:=';';xchr[60]:='<';xchr[61]:='=';
xchr[62]:='>';xchr[63]:='?';xchr[64]:='@';xchr[65]:='A';xchr[66]:='B';
xchr[67]:='C';xchr[68]:='D';xchr[69]:='E';xchr[70]:='F';xchr[71]:='G';
xchr[72]:='H';xchr[73]:='I';xchr[74]:='J';xchr[75]:='K';xchr[76]:='L';
xchr[77]:='M';xchr[78]:='N';xchr[79]:='O';xchr[80]:='P';xchr[81]:='Q';
xchr[82]:='R';xchr[83]:='S';xchr[84]:='T';xchr[85]:='U';xchr[86]:='V';
xchr[87]:='W';xchr[88]:='X';xchr[89]:='Y';xchr[90]:='Z';xchr[91]:='[';
xchr[92]:='\';xchr[93]:=']';xchr[94]:='^';xchr[95]:='_';xchr[96]:='`';
xchr[97]:='a';xchr[98]:='b';xchr[99]:='c';xchr[100]:='d';xchr[101]:='e';
xchr[102]:='f';xchr[103]:='g';xchr[104]:='h';xchr[105]:='i';
xchr[106]:='j';xchr[107]:='k';xchr[108]:='l';xchr[109]:='m';
xchr[110]:='n';xchr[111]:='o';xchr[112]:='p';xchr[113]:='q';
xchr[114]:='r';xchr[115]:='s';xchr[116]:='t';xchr[117]:='u';
xchr[118]:='v';xchr[119]:='w';xchr[120]:='x';xchr[121]:='y';
xchr[122]:='z';xchr[123]:='{';xchr[124]:='|';xchr[125]:='}';
xchr[126]:='~';for i:=127 to 255 do xchr[i]:='?';{:12}{13:}
for i:=0 to 127 do xord[chr(i)]:=32;
for i:=32 to 126 do xord[xchr[i]]:=i;{:13}{51:}{:51}{58:}mminm:=999999;
mminn:=999999;mmaxm:=-999999;mmaxn:=-999999;
for i:=0 to 255 do charpointer[i]:=-1;{:58}end;{:4}{40:}
procedure openpkfile;var j:integer;begin verbose:=false;gfarg:=3;
if argc<2 then begin verbose:=true;
if verbose then writeln(stdout,'Usage: pktogf [-v] <pk file> [gf file].'
);uexit(1);end;argv(1,pkname);
if pkname[1]=xchr[45]then begin if argc>4 then begin verbose:=true;
if verbose then writeln(stdout,'Usage: pktogf [-v] <pk file> [gf file].'
);uexit(1);end;if pkname[2]=xchr[118]then begin verbose:=true;
argv(2,pkname);gfarg:=gfarg+1 end else begin verbose:=true;
if verbose then writeln(stdout,'Usage: pktogf [-v] <pk file> [gf file].'
);uexit(1);end;end;
if verbose then writeln(stdout,'This is PKtoGF, C Version 1.1 ');
if testreadaccess(pkname,PKFILEPATH)then begin reset(pkfile,pkname)end
else begin printpascalstring(pkname);begin verbose:=true;
if verbose then writeln(stdout,': PK file not found.');uexit(1);end;end;
curloc:=0;end;procedure opengffile;
var dotpos,slashpos,last,gfindex,pkindex:integer;
begin if argc=gfarg then argv(argc-1,gfname)else begin dotpos:=-1;
slashpos:=-1;last:=1;
while(pkname[last]<>' ')and(last<=PATHMAX-5)do begin if pkname[last]='.'
then dotpos:=last;if pkname[last]='/'then slashpos:=last;last:=last+1;
end;if slashpos=-1 then slashpos:=0;
if dotpos<slashpos then dotpos:=last-1;gfindex:=1;
for pkindex:=slashpos+1 to dotpos do begin gfname[gfindex]:=pkname[
pkindex];gfindex:=gfindex+1;end;pkindex:=dotpos+1;
while(pkindex<last)and(pkname[pkindex]<>'p')do begin gfname[gfindex]:=
pkname[pkindex];pkindex:=pkindex+1;gfindex:=gfindex+1;end;
gfname[gfindex]:='g';gfname[gfindex+1]:='f';gfname[gfindex+2]:=' ';end;
if verbose then write(stdout,xchr[xord['[']]);pkindex:=1;
while pkname[pkindex]<>' 'do begin if verbose then write(stdout,xchr[
xord[pkname[pkindex]]]);pkindex:=pkindex+1;end;
if verbose then write(stdout,xchr[xord['-']]);
if verbose then write(stdout,xchr[xord['>']]);gfindex:=1;
while gfname[gfindex]<>' 'do begin if verbose then write(stdout,xchr[
xord[gfname[gfindex]]]);gfindex:=gfindex+1;end;
if verbose then write(stdout,xchr[xord[']']]);
if verbose then writeln(stdout,xchr[xord[' ']]);rewrite(gffile,gfname);
gfloc:=0 end;{:40}{43:}function getbyte:integer;var b:eightbits;
begin if eof(pkfile)then getbyte:=0 else begin read(pkfile,b);
curloc:=curloc+1;getbyte:=b;end;end;function signedbyte:integer;
var b:eightbits;begin read(pkfile,b);curloc:=curloc+1;
if b<128 then signedbyte:=b else signedbyte:=b-256;end;
function gettwobytes:integer;var a,b:eightbits;begin read(pkfile,a);
read(pkfile,b);curloc:=curloc+2;gettwobytes:=a*256+b;end;
function signedpair:integer;var a,b:eightbits;begin read(pkfile,a);
read(pkfile,b);curloc:=curloc+2;
if a<128 then signedpair:=a*256+b else signedpair:=(a-256)*256+b;end;
{function getthreebytes:integer;var a,b,c:eightbits;
begin read(pkfile,a);read(pkfile,b);read(pkfile,c);curloc:=curloc+3;
getthreebytes:=(a*256+b)*256+c;end;function signedtrio:integer;
var a,b,c:eightbits;begin read(pkfile,a);read(pkfile,b);read(pkfile,c);
curloc:=curloc+3;
if a<128 then signedtrio:=(a*256+b)*256+c else signedtrio:=((a-256)*256+
b)*256+c;end;}function signedquad:integer;var a,b,c,d:eightbits;
begin read(pkfile,a);read(pkfile,b);read(pkfile,c);read(pkfile,d);
curloc:=curloc+4;
if a<128 then signedquad:=((a*256+b)*256+c)*256+d else signedquad:=(((a
-256)*256+b)*256+c)*256+d;end;{:43}{46:}procedure gf16(i:integer);
begin begin putbyte(i div 256,gffile);gfloc:=gfloc+1 end;
begin putbyte(i mod 256,gffile);gfloc:=gfloc+1 end;end;
procedure gf24(i:integer);begin begin putbyte(i div 65536,gffile);
gfloc:=gfloc+1 end;gf16(i mod 65536);end;procedure gfquad(i:integer);
begin if i>=0 then begin begin putbyte(i div 16777216,gffile);
gfloc:=gfloc+1 end;end else begin i:=(i+1073741824)+1073741824;
begin putbyte(128+(i div 16777216),gffile);gfloc:=gfloc+1 end;end;
gf24(i mod 16777216);end;{:46}{62:}function getnyb:integer;
var temp:eightbits;begin if bitweight=0 then begin inputbyte:=getbyte;
bitweight:=16;end;temp:=inputbyte div bitweight;
inputbyte:=inputbyte-temp*bitweight;bitweight:=bitweight div 16;
getnyb:=temp;end;function getbit:boolean;var temp:boolean;
begin bitweight:=bitweight div 2;
if bitweight=0 then begin inputbyte:=getbyte;bitweight:=128;end;
temp:=inputbyte>=bitweight;if temp then inputbyte:=inputbyte-bitweight;
getbit:=temp;end;{30:}function pkpackednum:integer;var i,j:integer;
begin i:=getnyb;if i=0 then begin repeat j:=getnyb;i:=i+1;until j<>0;
while i>0 do begin j:=j*16+getnyb;i:=i-1;end;
pkpackednum:=j-15+(13-dynf)*16+dynf;
end else if i<=dynf then pkpackednum:=i else if i<14 then pkpackednum:=(
i-dynf-1)*16+getnyb+dynf+1 else begin if i=14 then repeatcount:=
pkpackednum else repeatcount:=1;pkpackednum:=pkpackednum;end;end;{:30}
{:62}{70:}procedure skipspecials;var i,j,k:integer;
begin thischarptr:=gfloc;repeat flagbyte:=getbyte;
if flagbyte>=240 then case flagbyte of 240,241,242,243:begin i:=0;
begin putbyte(flagbyte-1,gffile);gfloc:=gfloc+1 end;
for j:=240 to flagbyte do begin k:=getbyte;begin putbyte(k,gffile);
gfloc:=gfloc+1 end;i:=256*i+k;end;
for j:=1 to i do begin putbyte(getbyte,gffile);gfloc:=gfloc+1 end;end;
244:begin begin putbyte(243,gffile);gfloc:=gfloc+1 end;
gfquad(signedquad);end;245:begin end;246:begin end;
247,248,249,250,251,252,253,254,255:begin verbose:=true;
if verbose then writeln(stdout,'Unexpected ',flagbyte:1,'!');uexit(1);
end;end;until(flagbyte<240)or(flagbyte=245);end;{:70}{73:}
begin initialize;{44:}openpkfile;opengffile{:44};{49:}
if getbyte<>247 then begin verbose:=true;
if verbose then writeln(stdout,'Bad pk file!  pre command missing.');
uexit(1);end;begin putbyte(247,gffile);gfloc:=gfloc+1 end;
if getbyte<>89 then begin verbose:=true;
if verbose then writeln(stdout,'Wrong version of packed file!.');
uexit(1);end;begin putbyte(131,gffile);gfloc:=gfloc+1 end;j:=getbyte;
begin putbyte(j,gffile);gfloc:=gfloc+1 end;
if verbose then write(stdout,'{');for i:=1 to j do begin hppp:=getbyte;
begin putbyte(hppp,gffile);gfloc:=gfloc+1 end;
if verbose then write(stdout,xchr[xord[hppp]]);end;
if verbose then writeln(stdout,'}');designsize:=signedquad;
checksum:=signedquad;hppp:=signedquad;vppp:=signedquad;
if hppp<>vppp then if verbose then writeln(stdout,
'Warning:  aspect ratio not 1:1!');
magnification:=round(hppp*72.27*5/65536);lasteoc:=gfloc{:49};
skipspecials;while flagbyte<>245 do begin{47:}dynf:=flagbyte div 16;
flagbyte:=flagbyte mod 16;turnon:=flagbyte>=8;
if turnon then flagbyte:=flagbyte-8;if flagbyte=7 then{52:}
begin packetlength:=signedquad;car:=signedquad;
endofpacket:=packetlength+curloc;tfmwidth:=signedquad;
horesc:=signedquad;veresc:=signedquad;cwidth:=signedquad;
cheight:=signedquad;wordwidth:=(cwidth+31)div 32;xoff:=signedquad;
yoff:=signedquad;end{:52}else if flagbyte>3 then{53:}
begin packetlength:=(flagbyte-4)*65536+gettwobytes;car:=getbyte;
endofpacket:=packetlength+curloc;i:=getbyte;
tfmwidth:=i*65536+gettwobytes;horesc:=gettwobytes*65536;veresc:=0;
cwidth:=gettwobytes;cheight:=gettwobytes;wordwidth:=(cwidth+31)div 32;
xoff:=signedpair;yoff:=signedpair;end{:53}else{54:}
begin packetlength:=flagbyte*256+getbyte;car:=getbyte;
endofpacket:=packetlength+curloc;i:=getbyte;
tfmwidth:=i*65536+gettwobytes;horesc:=getbyte*65536;veresc:=0;
cwidth:=getbyte;cheight:=getbyte;wordwidth:=(cwidth+31)div 32;
xoff:=signedbyte;yoff:=signedbyte;end{:54};{56:}
if(cheight=0)or(cwidth=0)then begin cheight:=0;cwidth:=0;xoff:=0;
yoff:=0;end;minm:=-xoff;if minm<mminm then mminm:=minm;
maxm:=cwidth+minm;if maxm>mmaxm then mmaxm:=maxm;minn:=yoff-cheight+1;
maxn:=yoff;if minn>maxn then minn:=maxn;if minn<mminn then mminn:=minn;
if maxn>mmaxn then mmaxn:=maxn{:56};{60:}begin i:=car mod 256;
if(charpointer[i]=-1)then begin sveresc[i]:=veresc;shoresc[i]:=horesc;
stfmwidth[i]:=tfmwidth;
end else begin if(sveresc[i]<>veresc)or(shoresc[i]<>horesc)or(stfmwidth[
i]<>tfmwidth)then if verbose then writeln(stdout,'Two characters mod ',i
:1,' have mismatched parameters');end;end{:60};{59:}
begin if(charpointer[car mod 256]=-1)and(car>=0)and(car<256)and(maxm>=0)
and(maxm<256)and(maxn>=0)and(maxn<256)and(maxm>=minm)and(maxn>=minn)and(
maxm<minm+256)and(maxn<minn+256)then begin charpointer[car mod 256]:=
thischarptr;begin putbyte(68,gffile);gfloc:=gfloc+1 end;
begin putbyte(car,gffile);gfloc:=gfloc+1 end;
begin putbyte(maxm-minm,gffile);gfloc:=gfloc+1 end;
begin putbyte(maxm,gffile);gfloc:=gfloc+1 end;
begin putbyte(maxn-minn,gffile);gfloc:=gfloc+1 end;
begin putbyte(maxn,gffile);gfloc:=gfloc+1 end;
end else begin begin putbyte(67,gffile);gfloc:=gfloc+1 end;gfquad(car);
gfquad(charpointer[car mod 256]);charpointer[car mod 256]:=thischarptr;
gfquad(minm);gfquad(maxm);gfquad(minn);gfquad(maxn);end;end{:59};{65:}
if(cwidth>0)and(cheight>0)then begin bitweight:=0;
countdown:=cheight*cwidth-1;if dynf=14 then turnon:=getbit;
repeatcount:=0;xtogo:=cwidth;ytogo:=cheight;curn:=cheight;count:=0;
firston:=turnon;turnon:=not turnon;rcp:=0;
while ytogo>0 do begin if count=0 then{64:}begin turnon:=not turnon;
if dynf=14 then begin count:=1;done:=false;
while not done do begin if countdown<=0 then done:=true else if(turnon=
getbit)then count:=count+1 else done:=true;countdown:=countdown-1;end;
end else count:=pkpackednum;end{:64};if rcp=0 then firston:=turnon;
while count>=xtogo do begin rowcounts[rcp]:=xtogo;count:=count-xtogo;
for i:=0 to repeatcount do begin{66:}
if(rcp>0)or firston then begin j:=0;max:=rcp;
if not turnon then max:=max-1;
if curn-ytogo=1 then begin if firston then begin putbyte(74,gffile);
gfloc:=gfloc+1 end else if rowcounts[0]<165 then begin begin putbyte(74+
rowcounts[0],gffile);gfloc:=gfloc+1 end;j:=j+1;
end else begin putbyte(70,gffile);gfloc:=gfloc+1 end;
end else if curn>ytogo then begin if curn-ytogo<257 then begin begin
putbyte(71,gffile);gfloc:=gfloc+1 end;
begin putbyte(curn-ytogo-1,gffile);gfloc:=gfloc+1 end;
end else begin begin putbyte(72,gffile);gfloc:=gfloc+1 end;
gf16(curn-ytogo-1);end;if firston then begin putbyte(0,gffile);
gfloc:=gfloc+1 end;end else if firston then begin putbyte(0,gffile);
gfloc:=gfloc+1 end;curn:=ytogo;
while j<=max do begin if rowcounts[j]<64 then begin putbyte(0+rowcounts[
j],gffile);
gfloc:=gfloc+1 end else if rowcounts[j]<256 then begin begin putbyte(64,
gffile);gfloc:=gfloc+1 end;begin putbyte(rowcounts[j],gffile);
gfloc:=gfloc+1 end;end else begin begin putbyte(65,gffile);
gfloc:=gfloc+1 end;gf16(rowcounts[j]);end;j:=j+1;end;end{:66};
ytogo:=ytogo-1;end;repeatcount:=0;xtogo:=cwidth;rcp:=0;
if(count>0)then firston:=turnon;end;
if count>0 then begin rowcounts[rcp]:=count;
if rcp=0 then firston:=turnon;rcp:=rcp+1;
if rcp>maxcounts then begin verbose:=true;
if verbose then writeln(stdout,'A character had too many run counts');
uexit(1);end;xtogo:=xtogo-count;count:=0;end;end;end{:65};
begin putbyte(69,gffile);gfloc:=gfloc+1 end;lasteoc:=gfloc;
if endofpacket<>curloc then begin verbose:=true;
if verbose then writeln(stdout,'Bad pk file!  Bad packet length.');
uexit(1);end{:47};skipspecials;end;while not eof(pkfile)do i:=getbyte;
{68:}j:=gfloc;begin putbyte(248,gffile);gfloc:=gfloc+1 end;
gfquad(lasteoc);gfquad(designsize);gfquad(checksum);gfquad(hppp);
gfquad(vppp);gfquad(mminm);gfquad(mmaxm);gfquad(mminn);gfquad(mmaxn);
{61:}
for i:=0 to 255 do if charpointer[i]<>-1 then begin if(sveresc[i]=0)and(
shoresc[i]>=0)and(shoresc[i]<16777216)and(shoresc[i]mod 65536=0)then
begin begin putbyte(246,gffile);gfloc:=gfloc+1 end;
begin putbyte(i,gffile);gfloc:=gfloc+1 end;
begin putbyte(shoresc[i]div 65536,gffile);gfloc:=gfloc+1 end;
end else begin begin putbyte(245,gffile);gfloc:=gfloc+1 end;
begin putbyte(i,gffile);gfloc:=gfloc+1 end;gfquad(shoresc[i]);
gfquad(sveresc[i]);end;gfquad(stfmwidth[i]);gfquad(charpointer[i]);
end{:61};begin putbyte(249,gffile);gfloc:=gfloc+1 end;gfquad(j);
begin putbyte(131,gffile);gfloc:=gfloc+1 end;
for i:=0 to 3 do begin putbyte(223,gffile);gfloc:=gfloc+1 end;
while gfloc mod 4<>0 do begin putbyte(223,gffile);
gfloc:=gfloc+1 end{:68};
if verbose then writeln(stdout,curloc:1,' bytes unpacked to ',gfloc:1,
' bytes.');end.{:73}
