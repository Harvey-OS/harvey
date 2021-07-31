{4:}program GFtoPK;const{6:}linelength=79;maxrow=16000;{:6}type{9:}
ASCIIcode=32..126;{:9}{10:}textfile=packed file of char;{:10}{37:}
eightbits=0..255;bytefile=packed file of eightbits;
UNIXfilename=packed array[1..PATHMAX]of char;{:37}var{11:}
xord:array[char]of ASCIIcode;xchr:array[0..255]of char;{:11}{38:}
gffile:bytefile;pkfile:bytefile;verbose:boolean;pkarg:integer;
gfname:UNIXfilename;{:38}{40:}pkloc:integer;gfloc:integer;
pkopen:boolean;{:40}{45:}bitweight:integer;outputbyte:integer;{:45}{47:}
gflen:integer;{:47}{48:}tfmwidth:array[0..255]of integer;
dx,dy:array[0..255]of integer;status:array[0..255]of 0..2;
row:array[0..maxrow]of integer;{:48}{55:}gfch:integer;
gfchmod256:integer;predpkloc:integer;maxn,minn:integer;
maxm,minm:integer;rowptr:integer;{:55}{78:}power:array[0..8]of integer;
{:78}{82:}comment:packed array[1..0]of char;{:82}{86:}checksum:integer;
designsize:integer;hmag:integer;i:integer;{:86}procedure initialize;
var i:integer;begin setpaths(GFFILEPATHBIT);{12:}
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
for i:=127 to 255 do xchr[i]:='?';{:12}{13:}
for i:=0 to 127 do xord[chr(i)]:=32;
for i:=32 to 126 do xord[xchr[i]]:=i;{:13}{41:}pkopen:=false;{:41}{49:}
for i:=0 to 255 do status[i]:=0;{:49}{79:}power[0]:=1;
for i:=1 to 8 do power[i]:=power[i-1]+power[i-1];{:79};end;{:4}{30:}
{function pkpackednum:integer;var i,j:integer;begin i:=getnyb;
if i=0 then begin repeat j:=getnyb;i:=i+1;until j<>0;
while i>0 do begin j:=j*16+getnyb;i:=i-1;end;
pkpackednum:=j-15+(13-dynf)*16+dynf;
end else if i<=dynf then pkpackednum:=i else if i<14 then pkpackednum:=(
i-dynf-1)*16+getnyb+dynf+1 else begin if i=14 then repeatcount:=
pkpackednum else repeatcount:=1;pkpackednum:=pkpackednum;end;end;}{:30}
{39:}procedure opengffile;var j:integer;begin verbose:=false;pkarg:=3;
if(argc<2)or(argc>4)then begin verbose:=true;
if verbose then writeln(stdout,'Usage: gftopk [-v] <gf file> [pk file].'
);uexit(1);end;argv(1,gfname);
if gfname[1]=xchr[45]then begin if gfname[2]=xchr[118]then begin verbose
:=true;argv(2,gfname);pkarg:=pkarg+1 end else begin verbose:=true;
if verbose then writeln(stdout,'Usage: gftopk [-v] <gf file> [pk file].'
);uexit(1);end;end;
if verbose then writeln(stdout,'This is GFtoPK, C Version 2.3');
if testreadaccess(gfname,GFFILEPATH)then begin reset(gffile,gfname)end
else begin printpascalstring(gfname);begin verbose:=true;
if verbose then writeln(stdout,': GF file not found.');uexit(1);end;end;
gfloc:=0;end;procedure openpkfile;
var dotpos,slashpos,last,gfindex,pkindex:integer;pkname:UNIXfilename;
begin if argc=pkarg then argv(argc-1,pkname)else begin dotpos:=-1;
slashpos:=-1;last:=1;
while(gfname[last]<>' ')and(last<=PATHMAX-5)do begin if gfname[last]='.'
then dotpos:=last;if gfname[last]='/'then slashpos:=last;last:=last+1;
end;if slashpos=-1 then slashpos:=0;
if dotpos<slashpos then dotpos:=last-1;pkindex:=1;
for gfindex:=slashpos+1 to dotpos do begin pkname[pkindex]:=gfname[
gfindex];pkindex:=pkindex+1;end;gfindex:=dotpos+1;
while(gfindex<last)and(gfname[gfindex]<>'g')do begin pkname[pkindex]:=
gfname[gfindex];gfindex:=gfindex+1;pkindex:=pkindex+1;end;
pkname[pkindex]:='p';pkname[pkindex+1]:='k';pkname[pkindex+2]:=' ';end;
if verbose then write(stdout,xchr[xord['[']]);gfindex:=1;
while gfname[gfindex]<>' 'do begin if verbose then write(stdout,xchr[
xord[gfname[gfindex]]]);gfindex:=gfindex+1;end;
if verbose then write(stdout,xchr[xord['-']]);
if verbose then write(stdout,xchr[xord['>']]);pkindex:=1;
while pkname[pkindex]<>' 'do begin if verbose then write(stdout,xchr[
xord[pkname[pkindex]]]);pkindex:=pkindex+1;end;
if verbose then write(stdout,xchr[xord[']']]);
if verbose then writeln(stdout,xchr[xord[' ']]);rewrite(pkfile,pkname);
pkloc:=0;pkopen:=true end;{:39}{42:}function gfbyte:integer;
var b:eightbits;begin if eof(gffile)then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','Unexpected end of file!'
,'!');uexit(1);end else begin read(gffile,b);gfbyte:=b;end;
gfloc:=gfloc+1;end;function gfsignedquad:integer;var a,b,c,d:eightbits;
begin read(gffile,a);read(gffile,b);read(gffile,c);read(gffile,d);
if a<128 then gfsignedquad:=((a*256+b)*256+c)*256+d else gfsignedquad:=(
((a-256)*256+b)*256+c)*256+d;gfloc:=gfloc+4;end;{:42}{44:}
procedure pkhalfword(a:integer);begin if a<0 then a:=a+65536;
putbyte(a div 256,pkfile);putbyte(a mod 256,pkfile);pkloc:=pkloc+2;end;
procedure pkthreebytes(a:integer);
begin putbyte(a div 65536 mod 256,pkfile);
putbyte(a div 256 mod 256,pkfile);putbyte(a mod 256,pkfile);
pkloc:=pkloc+3;end;procedure pkword(a:integer);var b:integer;
begin if a<0 then begin a:=a+1073741824;a:=a+1073741824;
b:=128+a div 16777216;end else b:=a div 16777216;putbyte(b,pkfile);
putbyte(a div 65536 mod 256,pkfile);putbyte(a div 256 mod 256,pkfile);
putbyte(a mod 256,pkfile);pkloc:=pkloc+4;end;procedure pknyb(a:integer);
begin if bitweight=16 then begin outputbyte:=a*16;bitweight:=1;
end else begin begin putbyte(outputbyte+a,pkfile);pkloc:=pkloc+1 end;
bitweight:=16;end;end;{:44}{46:}function gflength:integer;
begin checkedfseek(gffile,0,2);gflength:=ftell(gffile);end;
procedure movetobyte(n:integer);begin checkedfseek(gffile,n,0);end;{:46}
{51:}{62:}procedure packandsendcharacter;var i,j,k:integer;{65:}
extra:integer;putptr:integer;repeatflag:integer;hbit:integer;
buff:integer;{:65}{70:}dynf:integer;height,width:integer;
xoffset,yoffset:integer;deriv:array[1..13]of integer;bcompsize:integer;
firston:boolean;flagbyte:integer;state:boolean;on:boolean;{:70}{77:}
compsize:integer;count:integer;pbit:integer;ron,son:boolean;
rcount,scount:integer;ri,si:integer;max2:integer;{:77}begin{63:}i:=2;
rowptr:=rowptr-1;while row[i]=(-99999)do i:=i+1;
if row[i]<>(-99998)then begin maxn:=maxn-i+2;
while row[rowptr-2]=(-99999)do begin rowptr:=rowptr-1;
row[rowptr]:=(-99998);end;minn:=maxn+1;extra:=maxm-minm+1;maxm:=0;j:=i;
while row[j]<>(-99998)do begin minn:=minn-1;
if row[j]<>(-99999)then begin k:=row[j];if k<extra then extra:=k;j:=j+1;
while row[j]<>(-99999)do begin k:=k+row[j];j:=j+1;end;
if maxm<k then maxm:=k;end;j:=j+1;end;minm:=minm+extra;
maxm:=minm+maxm-1-extra;height:=maxn-minn+1;width:=maxm-minm+1;
xoffset:=-minm;yoffset:=maxn;;end else begin height:=0;width:=0;
xoffset:=0;yoffset:=0;;end{:63};{64:}putptr:=0;rowptr:=2;repeatflag:=0;
state:=true;buff:=0;while row[rowptr]=(-99999)do rowptr:=rowptr+1;
while row[rowptr]<>(-99998)do begin{66:}i:=rowptr;
if(row[i]<>(-99999))and((row[i]<>extra)or(row[i+1]<>width))then begin j
:=i+1;while row[j-1]<>(-99999)do j:=j+1;
while row[i]=row[j]do begin if row[i]=(-99999)then begin repeatflag:=
repeatflag+1;rowptr:=i+1;end;i:=i+1;j:=j+1;end;end{:66};{67:}
if row[rowptr]<>(-99999)then row[rowptr]:=row[rowptr]-extra;hbit:=0;
while row[rowptr]<>(-99999)do begin hbit:=hbit+row[rowptr];
if state then begin buff:=buff+row[rowptr];state:=false;
end else if row[rowptr]>0 then begin begin row[putptr]:=buff;
putptr:=putptr+1;if repeatflag>0 then begin row[putptr]:=-repeatflag;
repeatflag:=0;putptr:=putptr+1;end;end;buff:=row[rowptr];
end else state:=true;rowptr:=rowptr+1;end;
if hbit<width then if state then buff:=buff+width-hbit else begin begin
row[putptr]:=buff;putptr:=putptr+1;
if repeatflag>0 then begin row[putptr]:=-repeatflag;repeatflag:=0;
putptr:=putptr+1;end;end;buff:=width-hbit;state:=true;
end else state:=false;rowptr:=rowptr+1{:67};end;
if buff>0 then begin row[putptr]:=buff;putptr:=putptr+1;
if repeatflag>0 then begin row[putptr]:=-repeatflag;repeatflag:=0;
putptr:=putptr+1;end;end;begin row[putptr]:=(-99998);putptr:=putptr+1;
if repeatflag>0 then begin row[putptr]:=-repeatflag;repeatflag:=0;
putptr:=putptr+1;end;end{:64};{68:}for i:=1 to 13 do deriv[i]:=0;i:=0;
firston:=row[i]=0;if firston then i:=i+1;compsize:=0;
while row[i]<>(-99998)do{69:}begin j:=row[i];
if j=-1 then compsize:=compsize+1 else begin if j<0 then begin compsize
:=compsize+1;j:=-j;end;
if j<209 then compsize:=compsize+2 else begin k:=j-193;
while k>=16 do begin k:=k div 16;compsize:=compsize+2;end;
compsize:=compsize+1;end;
if j<14 then deriv[j]:=deriv[j]-1 else if j<209 then deriv[(223-j)div 15
]:=deriv[(223-j)div 15]+1 else begin k:=16;while(k*16<j+3)do k:=k*16;
if j-k<=192 then deriv[(207-j+k)div 15]:=deriv[(207-j+k)div 15]+2;end;
end;i:=i+1;end{:69};bcompsize:=compsize;dynf:=0;
for i:=1 to 13 do begin compsize:=compsize+deriv[i];
if compsize<=bcompsize then begin bcompsize:=compsize;dynf:=i;end;end;
compsize:=(bcompsize+1)div 2;
if(compsize>(height*width+7)div 8)or(height*width=0)then begin compsize
:=(height*width+7)div 8;dynf:=14;end;;{71:}flagbyte:=dynf*16;
if firston then flagbyte:=flagbyte+8;
if(gfch<>gfchmod256)or(tfmwidth[gfchmod256]>16777215)or(tfmwidth[
gfchmod256]<0)or(dy[gfchmod256]<>0)or(dx[gfchmod256]<0)or(dx[gfchmod256]
mod 65536<>0)or(compsize>196594)or(width>65535)or(height>65535)or(
xoffset>32767)or(yoffset>32767)or(xoffset<-32768)or(yoffset<-32768)then{
72:}begin flagbyte:=flagbyte+7;begin putbyte(flagbyte,pkfile);
pkloc:=pkloc+1 end;compsize:=compsize+28;pkword(compsize);pkword(gfch);
predpkloc:=pkloc+compsize;pkword(tfmwidth[gfchmod256]);
pkword(dx[gfchmod256]);pkword(dy[gfchmod256]);pkword(width);
pkword(height);pkword(xoffset);pkword(yoffset);end{:72}
else if(dx[gfch]>16777215)or(width>255)or(height>255)or(xoffset>127)or(
yoffset>127)or(xoffset<-128)or(yoffset<-128)or(compsize>1015)then{74:}
begin compsize:=compsize+13;flagbyte:=flagbyte+compsize div 65536+4;
begin putbyte(flagbyte,pkfile);pkloc:=pkloc+1 end;
pkhalfword(compsize mod 65536);begin putbyte(gfch,pkfile);
pkloc:=pkloc+1 end;predpkloc:=pkloc+compsize;
pkthreebytes(tfmwidth[gfchmod256]);pkhalfword(dx[gfchmod256]div 65536);
pkhalfword(width);pkhalfword(height);pkhalfword(xoffset);
pkhalfword(yoffset);end{:74}else{73:}begin compsize:=compsize+8;
flagbyte:=flagbyte+compsize div 256;begin putbyte(flagbyte,pkfile);
pkloc:=pkloc+1 end;begin putbyte(compsize mod 256,pkfile);
pkloc:=pkloc+1 end;begin putbyte(gfch,pkfile);pkloc:=pkloc+1 end;
predpkloc:=pkloc+compsize;pkthreebytes(tfmwidth[gfchmod256]);
begin putbyte(dx[gfchmod256]div 65536,pkfile);pkloc:=pkloc+1 end;
begin putbyte(width,pkfile);pkloc:=pkloc+1 end;
begin putbyte(height,pkfile);pkloc:=pkloc+1 end;
begin putbyte(xoffset,pkfile);pkloc:=pkloc+1 end;
begin putbyte(yoffset,pkfile);pkloc:=pkloc+1 end;end{:73}{:71};
if dynf<>14 then{75:}begin bitweight:=16;max2:=208-15*dynf;i:=0;
if row[i]=0 then i:=i+1;while row[i]<>(-99998)do begin j:=row[i];
if j=-1 then pknyb(15)else begin if j<0 then begin pknyb(14);j:=-j;end;
if j<=dynf then pknyb(j)else if j<=max2 then begin j:=j-dynf-1;
pknyb(j div 16+dynf+1);pknyb(j mod 16);end else begin j:=j-max2+15;
k:=16;while k<=j do begin k:=k*16;pknyb(0);end;
while k>1 do begin k:=k div 16;pknyb(j div k);j:=j mod k;end;end;end;
i:=i+1;end;if bitweight<>16 then begin putbyte(outputbyte,pkfile);
pkloc:=pkloc+1 end;end{:75}else if height>0 then{76:}begin buff:=0;
pbit:=8;i:=1;hbit:=width;on:=false;state:=false;count:=row[0];
repeatflag:=0;
while(row[i]<>(-99998))or state or(count>0)do begin if state then begin
count:=rcount;i:=ri;on:=ron;repeatflag:=repeatflag-1;
end else begin rcount:=count;ri:=i;ron:=on;end;{80:}
repeat if count=0 then begin if row[i]<0 then begin if not state then
repeatflag:=-row[i];i:=i+1;end;count:=row[i];i:=i+1;on:=not on;end;
if(count>=pbit)and(pbit<hbit)then begin if on then buff:=buff+power[pbit
]-1;begin putbyte(buff,pkfile);pkloc:=pkloc+1 end;buff:=0;
hbit:=hbit-pbit;count:=count-pbit;pbit:=8;
end else if(count<pbit)and(count<hbit)then begin if on then buff:=buff+
power[pbit]-power[pbit-count];pbit:=pbit-count;hbit:=hbit-count;
count:=0;
end else begin if on then buff:=buff+power[pbit]-power[pbit-hbit];
count:=count-hbit;pbit:=pbit-hbit;hbit:=width;
if pbit=0 then begin begin putbyte(buff,pkfile);pkloc:=pkloc+1 end;
buff:=0;pbit:=8;end;end;until hbit=width{:80};
if state and(repeatflag=0)then begin count:=scount;i:=si;on:=son;
state:=false;
end else if not state and(repeatflag>0)then begin scount:=count;si:=i;
son:=on;state:=true;end;end;if pbit<>8 then begin putbyte(buff,pkfile);
pkloc:=pkloc+1 end;end{:76}{:68};end{:62};procedure convertgffile;
var i,j,k:integer;gfcom:integer;{58:}dotherows:boolean;on:boolean;
state:boolean;extra:integer;bad:boolean;{:58}{61:}hppp,vppp:integer;
q:integer;postloc:integer;{:61}begin opengffile;
if gfbyte<>247 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ',
'First byte is not preamble','!');uexit(1);end;
if gfbyte<>131 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ',
'Identification byte is incorrect','!');uexit(1);end;{60:}
gflen:=gflength;postloc:=gflen-4;
repeat if postloc=0 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','all 223''s','!');
uexit(1);end;movetobyte(postloc);k:=gfbyte;postloc:=postloc-1;
until k<>223;if k<>131 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','ID byte is ',k:1,'!');
uexit(1);end;movetobyte(postloc-3);q:=gfsignedquad;
if(q<0)or(q>postloc-3)then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','post pointer is ',q:1,
'!');uexit(1);end;movetobyte(q);k:=gfbyte;
if k<>248 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','byte at ',q:1,
' is not post','!');uexit(1);end;i:=gfsignedquad;
designsize:=gfsignedquad;checksum:=gfsignedquad;hppp:=gfsignedquad;
hmag:=round(hppp*72.27/65536);vppp:=gfsignedquad;
if hppp<>vppp then if verbose then writeln(stdout,'Odd aspect ratio!');
i:=gfsignedquad;i:=gfsignedquad;i:=gfsignedquad;i:=gfsignedquad;
repeat gfcom:=gfbyte;case gfcom of 245,246:begin gfch:=gfbyte;
if status[gfch]<>0 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ',
'Locator for this character already found.','!');uexit(1);end;
if gfcom=245 then begin dx[gfch]:=gfsignedquad;dy[gfch]:=gfsignedquad;
end else begin dx[gfch]:=gfbyte*65536;dy[gfch]:=0;end;
tfmwidth[gfch]:=gfsignedquad;i:=gfsignedquad;status[gfch]:=1;end;{53:}
239,240,241,242:begin begin putbyte(gfcom+1,pkfile);pkloc:=pkloc+1 end;
i:=0;for j:=0 to gfcom-239 do begin k:=gfbyte;begin putbyte(k,pkfile);
pkloc:=pkloc+1 end;i:=i*256+k;end;
for j:=1 to i do begin putbyte(gfbyte,pkfile);pkloc:=pkloc+1 end;end;
243:begin begin putbyte(244,pkfile);pkloc:=pkloc+1 end;
pkword(gfsignedquad);end;244:{:53};249:;others:begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','Unexpected ',gfcom:1,
' in postamble','!');uexit(1);end end;until gfcom=249{:60};
movetobyte(2);openpkfile;{81:}begin putbyte(247,pkfile);
pkloc:=pkloc+1 end;begin putbyte(89,pkfile);pkloc:=pkloc+1 end;
i:=gfbyte;repeat if i=0 then j:=46 else j:=gfbyte;i:=i-1;until j<>32;
i:=i+1;if i=0 then k:=-0 else k:=i+0;
if k>255 then begin putbyte(255,pkfile);
pkloc:=pkloc+1 end else begin putbyte(k,pkfile);pkloc:=pkloc+1 end;
for k:=1 to 0 do if(i>0)or(k<=-0)then begin putbyte(xord[comment[k]],
pkfile);pkloc:=pkloc+1 end;if verbose then write(stdout,'''');
for k:=1 to i do begin if k>1 then j:=gfbyte;
if verbose then write(stdout,xchr[j]);
if k<256 then begin putbyte(j,pkfile);pkloc:=pkloc+1 end;end;
if verbose then writeln(stdout,'''');pkword(designsize);
pkword(checksum);pkword(hppp);pkword(vppp){:81};repeat gfcom:=gfbyte;
dotherows:=false;case gfcom of 67,68:{54:}
begin if gfcom=67 then begin gfch:=gfsignedquad;i:=gfsignedquad;
minm:=gfsignedquad;maxm:=gfsignedquad;minn:=gfsignedquad;
maxn:=gfsignedquad;end else begin gfch:=gfbyte;i:=gfbyte;maxm:=gfbyte;
minm:=maxm-i;i:=gfbyte;maxn:=gfbyte;minn:=maxn-i;end;;
if gfch>=0 then gfchmod256:=gfch mod 256 else gfchmod256:=255-((-(1+gfch
))mod 256);if status[gfchmod256]=0 then begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ',
'no character locator for character ',gfch:1,'!');uexit(1);end;{57:}
begin bad:=false;rowptr:=2;on:=false;extra:=0;state:=true;
repeat gfcom:=gfbyte;case gfcom of{59:}0:begin state:=not state;
on:=not on;end;
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
52,53,54,55,56,57,58,59,60,61,62,63,64,65,66:begin if gfcom<64 then i:=
gfcom-0 else begin i:=0;for j:=0 to gfcom-64 do i:=i*256+gfbyte;end;
if state then begin extra:=extra+i;state:=false;
end else begin begin if rowptr>maxrow then bad:=true else begin row[
rowptr]:=extra;rowptr:=rowptr+1;end;end;extra:=i;end;on:=not on;end{:59}
;70,71,72,73:begin i:=0;for j:=1 to gfcom-70 do i:=i*256+gfbyte;
if on=state then begin if rowptr>maxrow then bad:=true else begin row[
rowptr]:=extra;rowptr:=rowptr+1;end;end;
for j:=0 to i do begin if rowptr>maxrow then bad:=true else begin row[
rowptr]:=(-99999);rowptr:=rowptr+1;end;end;on:=false;extra:=0;
state:=true;end;
74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,
98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
134,135,136,137:dotherows:=true;
138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,
156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,
174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201:dotherows:=true;
202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,
238:dotherows:=true;{53:}
239,240,241,242:begin begin putbyte(gfcom+1,pkfile);pkloc:=pkloc+1 end;
i:=0;for j:=0 to gfcom-239 do begin k:=gfbyte;begin putbyte(k,pkfile);
pkloc:=pkloc+1 end;i:=i*256+k;end;
for j:=1 to i do begin putbyte(gfbyte,pkfile);pkloc:=pkloc+1 end;end;
243:begin begin putbyte(244,pkfile);pkloc:=pkloc+1 end;
pkword(gfsignedquad);end;244:{:53};
69:begin if on=state then begin if rowptr>maxrow then bad:=true else
begin row[rowptr]:=extra;rowptr:=rowptr+1;end;end;
if(rowptr>2)and(row[rowptr-1]<>(-99999))then begin if rowptr>maxrow then
bad:=true else begin row[rowptr]:=(-99999);rowptr:=rowptr+1;end;end;
begin if rowptr>maxrow then bad:=true else begin row[rowptr]:=(-99998);
rowptr:=rowptr+1;end;end;if bad then begin verbose:=true;
if verbose then writeln(stdout,
'Ran out of internal memory for row counts!');uexit(1);end;
packandsendcharacter;status[gfchmod256]:=2;
if pkloc<>predpkloc then begin verbose:=true;
if verbose then writeln(stdout,'Internal error while writing character!'
);uexit(1);end;end;others:begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','Unexpected ',gfcom:1,
' character in character definition','!');uexit(1);end;end;
if dotherows then begin dotherows:=false;
if on=state then begin if rowptr>maxrow then bad:=true else begin row[
rowptr]:=extra;rowptr:=rowptr+1;end;end;
begin if rowptr>maxrow then bad:=true else begin row[rowptr]:=(-99999);
rowptr:=rowptr+1;end;end;on:=true;extra:=gfcom-74;state:=false;end;
until gfcom=69;end{:57};end{:54};{53:}
239,240,241,242:begin begin putbyte(gfcom+1,pkfile);pkloc:=pkloc+1 end;
i:=0;for j:=0 to gfcom-239 do begin k:=gfbyte;begin putbyte(k,pkfile);
pkloc:=pkloc+1 end;i:=i*256+k;end;
for j:=1 to i do begin putbyte(gfbyte,pkfile);pkloc:=pkloc+1 end;end;
243:begin begin putbyte(244,pkfile);pkloc:=pkloc+1 end;
pkword(gfsignedquad);end;244:{:53};248:;others:begin verbose:=true;
if verbose then writeln(stdout,'Bad GF file: ','Unexpected ',gfcom:1,
' command between characters','!');uexit(1);end end;until gfcom=248;
{83:}begin putbyte(245,pkfile);pkloc:=pkloc+1 end;
while(pkloc mod 4<>0)do begin putbyte(246,pkfile);
pkloc:=pkloc+1 end{:83};end;{:51}{85:}begin initialize;convertgffile;
{84:}
for i:=0 to 255 do if status[i]=1 then if verbose then writeln(stdout,
'Character ',i:1,' missing raster information!'){:84};
if verbose then writeln(stdout,gflen:1,' bytes packed to ',pkloc:1,
' bytes.');end.{:85}
