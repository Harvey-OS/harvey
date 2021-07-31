{2:}program VFtoVP;label{3:}9999;{:3}const{4:}tfmsize=30000;
vfsize=10000;maxfonts=300;ligsize=5000;hashsize=5003;maxstack=50;{:4}
{142:}charcodeascii=0;charcodeoctal=1;charcodedefault=2;{:142}{146:}
noptions=2;argoptions=1;{:146}type{5:}byte=0..255;{:5}{22:}
index=0..tfmsize;{:22}{141:}
charcodeformattype=charcodeascii..charcodedefault;{:141}var{7:}
vffile:packed file of byte;vfname:packed array[1..PATHMAX]of char;{:7}
{10:}tfmfile:packed file of byte;
tfmname:packed array[1..PATHMAX]of char;{:10}{12:}
lf,lh,bc,ec,nw,nh,nd,ni,nl,nk,ne,np:0..32767;{:12}{20:}vplfile:text;
vplname:packed array[1..PATHMAX]of char;{:20}{23:}
tfm:array[-1000..tfmsize]of byte;{:23}{26:}
charbase,widthbase,heightbase,depthbase,italicbase,ligkernbase,kernbase,
extenbase,parambase:integer;{:26}{29:}fonttype:0..2;{:29}{30:}
vf:array[0..vfsize]of byte;fontnumber:array[0..maxfonts]of integer;
fontstart,fontchars:array[0..maxfonts]of 0..vfsize;fontptr:0..maxfonts;
packetstart,packetend:array[byte]of 0..vfsize;packetfound:boolean;
tempbyte:byte;count:integer;realdsize:real;pl:integer;vfptr:0..vfsize;
vfcount:integer;{:30}{37:}a:integer;l:integer;
curname:packed array[1..PATHMAX]of char;b0,b1,b2,b3:byte;
fontlh:0..32767;fontbc,fontec:0..32767;{:37}{42:}
defaultdirectory:packed array[1..9]of char;{:42}{49:}
ASCII04,ASCII10,ASCII14:ccharpointer;xchr:packed array[0..255]of char;
MBLstring,RIstring,RCEstring:ccharpointer;{:49}{51:}
dig:array[0..11]of 0..9;{:51}{54:}level:0..5;{:54}{67:}charsonline:0..8;
perfect:boolean;{:67}{69:}i:0..32767;c:0..256;d:0..3;k:index;r:0..65535;
{:69}{85:}labeltable:array[0..258]of record cc:0..256;rr:0..ligsize;end;
labelptr:0..257;sortptr:0..257;boundarychar:0..256;bcharlabel:0..32767;
{:85}{87:}activity:array[0..ligsize]of 0..2;ai,acti:0..ligsize;{:87}
{111:}hash:array[0..hashsize]of 0..66048;
class:array[0..hashsize]of 0..4;ligz:array[0..hashsize]of 0..257;
hashptr:0..hashsize;hashlist:array[0..hashsize]of 0..hashsize;
h,hh:0..hashsize;xligcycle,yligcycle:0..256;{:111}{123:}top:0..maxstack;
wstack,xstack,ystack,zstack:array[0..maxstack]of integer;
vflimit:0..vfsize;o:byte;{:123}{138:}verbose:integer;{:138}{143:}
charcodeformat:charcodeformattype;{:143}procedure initialize;
var k:integer;{136:}longoptions:array[0..noptions]of getoptstruct;
getoptreturnval:integer;optionindex:integer;currentoption:0..noptions;
{:136}begin if(argc<3)or(argc>noptions+argoptions+4)then begin write(
'Usage: vftovp ');write('[-verbose] ');
writeln('[-charcode-format=<format>] ');
writeln('  <vfm file> <tfm file> [<vpl file>].');uexit(1);end;{139:}
verbose:=false;{:139}{144:}charcodeformat:=charcodedefault;{:144};{135:}
begin{137:}currentoption:=0;longoptions[0].name:='verbose';
longoptions[0].hasarg:=0;longoptions[0].flag:=addressofint(verbose);
longoptions[0].val:=1;currentoption:=currentoption+1;{:137}{140:}
longoptions[currentoption].name:='charcode-format';
longoptions[currentoption].hasarg:=1;longoptions[currentoption].flag:=0;
longoptions[currentoption].val:=0;currentoption:=currentoption+1;{:140}
{145:}longoptions[currentoption].name:=0;
longoptions[currentoption].hasarg:=0;longoptions[currentoption].flag:=0;
longoptions[currentoption].val:=0;{:145};
repeat getoptreturnval:=getoptlongonly(argc,gargv,'',longoptions,
addressofint(optionindex));
if getoptreturnval<>-1 then begin if getoptreturnval=63 then uexit(1);
if(strcmp(longoptions[optionindex].name,'charcode-format')=0)then begin
if strcmp(optarg,'ascii')=0 then charcodeformat:=charcodeascii else if
strcmp(optarg,'octal')=0 then charcodeformat:=charcodeoctal else write(
'Bad character code format',optarg,'.');end else;end;
until getoptreturnval=-1;end{:135};{11:}
setpaths(TFMFILEPATHBIT+VFFILEPATHBIT);argv(optind,vfname);
if testreadaccess(vfname,VFFILEPATH)then reset(vffile,vfname)else begin
printpascalstring(vfname);writeln(': VF file not found.');uexit(1);end;
argv(optind+1,tfmname);
if testreadaccess(tfmname,TFMFILEPATH)then reset(tfmfile,tfmname)else
begin printpascalstring(tfmname);writeln(': TFM file not found.');
uexit(1);end;if verbose then writeln('This is VFtoVP, C Version 1.2');
{:11}{21:}
if optind+2=argc then vplfile:=stdout else begin argv(optind+2,vplname);
rewrite(vplfile,vplname);end;{:21}{50:}
ASCII04:='  !"#$%&''()*+,-./0123456789:;<=>?';
ASCII10:=' @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';
ASCII14:=' `abcdefghijklmnopqrstuvwxyz{|}~?';
for k:=0 to 255 do xchr[k]:='?';
for k:=0 to 31 do begin xchr[k+32]:=ASCII04[k+1];
xchr[k+64]:=ASCII10[k+1];xchr[k+96]:=ASCII14[k+1];end;MBLstring:=' MBL';
RIstring:=' RI ';RCEstring:=' RCE';{:50}{55:}level:=0;{:55}{68:}
charsonline:=0;perfect:=true;{:68}{86:}boundarychar:=256;
bcharlabel:=32767;labelptr:=0;labeltable[0].rr:=0;{:86}end;{:2}{38:}
procedure readtfmword;
begin if eof(tfmfile)then b0:=0 else read(tfmfile,b0);
if eof(tfmfile)then b1:=0 else read(tfmfile,b1);
if eof(tfmfile)then b2:=0 else read(tfmfile,b2);
if eof(tfmfile)then b3:=0 else read(tfmfile,b3);end;{:38}{45:}
function vfread(k:integer):integer;var b:byte;a:integer;
begin vfcount:=vfcount+k;if eof(vffile)then b:=0 else read(vffile,b);
a:=b;if k=4 then if b>=128 then a:=a-256;
while k>1 do begin if eof(vffile)then b:=0 else read(vffile,b);
a:=256*a+b;k:=k-1;end;vfread:=a;end;{:45}{47:}
function tfmwidth(c:byte):integer;var a:integer;k:index;
begin k:=4*(widthbase+tfm[4*(charbase+c)]);a:=tfm[k];
if a>=128 then a:=a-256;
tfmwidth:=((256*a+tfm[k+1])*256+tfm[k+2])*256+tfm[k+3];end;{:47}{52:}
procedure outdigs(j:integer);begin repeat j:=j-1;
write(vplfile,dig[j]:1);until j=0;end;procedure printdigs(j:integer);
begin repeat j:=j-1;write(dig[j]:1);until j=0;end;{:52}{53:}
procedure printoctal(c:byte);var j:0..2;begin write('''');
for j:=0 to 2 do begin dig[j]:=c mod 8;c:=c div 8;end;printdigs(3);end;
{:53}{56:}procedure outln;var l:0..5;begin writeln(vplfile);
for l:=1 to level do write(vplfile,'   ');end;procedure left;
begin level:=level+1;write(vplfile,'(');end;procedure right;
begin level:=level-1;write(vplfile,')');outln;end;{:56}{57:}
procedure outBCPL(k:index);var l:0..39;begin write(vplfile,' ');
l:=tfm[k];while l>0 do begin k:=k+1;l:=l-1;write(vplfile,xchr[tfm[k]]);
end;end;{:57}{58:}procedure outoctal(k,l:index);var a:0..1023;b:0..32;
j:0..11;begin write(vplfile,' O ');a:=0;b:=0;j:=0;while l>0 do{59:}
begin l:=l-1;
if tfm[k+l]<>0 then begin while b>2 do begin dig[j]:=a mod 8;a:=a div 8;
b:=b-3;j:=j+1;end;case b of 0:a:=tfm[k+l];1:a:=a+2*tfm[k+l];
2:a:=a+4*tfm[k+l];end;end;b:=b+8;end{:59};
while(a>0)or(j=0)do begin dig[j]:=a mod 8;a:=a div 8;j:=j+1;end;
outdigs(j);end;{:58}{60:}procedure outchar(c:byte);
begin if(fonttype>0)or(charcodeformat=charcodeoctal)then begin tfm[0]:=c
;outoctal(0,1)end else if(charcodeformat=charcodeascii)and(c>32)and(c<=
126)and(c<>40)and(c<>41)then write(vplfile,' C ',xchr[c-31])else if((c>=
48)and(c<=57))or((c>=65)and(c<=90))or((c>=97)and(c<=122))then write(
vplfile,' C ',xchr[c])else begin tfm[0]:=c;outoctal(0,1);end;end;{:60}
{61:}procedure outface(k:index);var s:0..1;b:0..8;
begin if tfm[k]>=18 then outoctal(k,1)else begin write(vplfile,' F ');
s:=tfm[k]mod 2;b:=tfm[k]div 2;putbyte(MBLstring[1+(b mod 3)],vplfile);
putbyte(RIstring[1+s],vplfile);putbyte(RCEstring[1+(b div 3)],vplfile);
end;end;{:61}{62:}procedure outfix(k:index);var a:0..4095;f:integer;
j:0..12;delta:integer;begin write(vplfile,' R ');
a:=(tfm[k]*16)+(tfm[k+1]div 16);
f:=((tfm[k+1]mod 16)*toint(256)+tfm[k+2])*256+tfm[k+3];
if a>2047 then{65:}begin write(vplfile,'-');a:=4096-a;
if f>0 then begin f:=1048576-f;a:=a-1;end;end{:65};{63:}begin j:=0;
repeat dig[j]:=a mod 10;a:=a div 10;j:=j+1;until a=0;outdigs(j);end{:63}
;{64:}begin write(vplfile,'.');f:=10*f+5;delta:=10;
repeat if delta>1048576 then f:=f+524288-(delta div 2);
write(vplfile,f div 1048576:1);f:=10*(f mod 1048576);delta:=delta*10;
until f<=delta;end;{:64};end;{:62}{74:}procedure checkBCPL(k,l:index);
var j:index;c:byte;begin if tfm[k]>=l then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'String is too long; I''ve shortened it drastically.');end;tfm[k]:=1;
end;for j:=k+1 to k+tfm[k]do begin c:=tfm[j];
if(c=40)or(c=41)then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'Parenthesis in string has been changed to slash.');end;tfm[j]:=47;
end else if(c<32)or(c>126)then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Nonstandard ASCII code has been blotted out.')
;end;tfm[j]:=63;end else if(c>=97)and(c<=122)then tfm[j]:=c-32;end;end;
{:74}{114:}procedure hashinput;label 10;var cc:0..3;zz:0..255;y:0..255;
key:integer;t:integer;begin if hashptr=hashsize then goto 10;{115:}
k:=4*(ligkernbase+(i));y:=tfm[k+1];t:=tfm[k+2];cc:=0;zz:=tfm[k+3];
if t>=128 then zz:=y else begin case t of 0,6:;5,11:zz:=y;1,7:cc:=1;
2:cc:=2;3:cc:=3;end;end{:115};key:=256*c+y+1;h:=(1009*key)mod hashsize;
while hash[h]>0 do begin if hash[h]<=key then begin if hash[h]=key then
goto 10;t:=hash[h];hash[h]:=key;key:=t;t:=class[h];class[h]:=cc;cc:=t;
t:=ligz[h];ligz[h]:=zz;zz:=t;end;if h>0 then h:=h-1 else h:=hashsize;
end;hash[h]:=key;class[h]:=cc;ligz[h]:=zz;hashptr:=hashptr+1;
hashlist[hashptr]:=h;10:end;{:114}{116:}
ifdef('notdef')function ligf(h,x,y:index):index;begin end;
endif('notdef')function eval(x,y:index):index;var key:integer;
begin key:=256*x+y+1;h:=(1009*key)mod hashsize;
while hash[h]>key do if h>0 then h:=h-1 else h:=hashsize;
if hash[h]<key then eval:=y else eval:=ligf(h,x,y);end;{:116}{117:}
function ligf(h,x,y:index):index;begin case class[h]of 0:;
1:begin class[h]:=4;ligz[h]:=eval(ligz[h],y);class[h]:=0;end;
2:begin class[h]:=4;ligz[h]:=eval(x,ligz[h]);class[h]:=0;end;
3:begin class[h]:=4;ligz[h]:=eval(eval(x,ligz[h]),y);class[h]:=0;end;
4:begin xligcycle:=x;yligcycle:=y;ligz[h]:=257;class[h]:=0;end;end;
ligf:=ligz[h];end;{:117}{118:}
function stringbalance(k,l:integer):boolean;label 45,10;
var j,bal:integer;begin if l>0 then if vf[k]=32 then goto 45;bal:=0;
for j:=k to k+l-1 do begin if(vf[j]<32)or(vf[j]>=127)then goto 45;
if vf[j]=40 then bal:=bal+1 else if vf[j]=41 then if bal=0 then goto 45
else bal:=bal-1;end;if bal>0 then goto 45;stringbalance:=true;goto 10;
45:stringbalance:=false;10:end;{:118}{120:}
procedure outasfix(x:integer);var k:1..3;
begin if abs(x)>=16777216 then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Oversize dimension has been reset to zero.');
end;if x>=0 then tfm[28]:=0 else begin tfm[28]:=255;x:=x+16777216;end;
for k:=3 downto 1 do begin tfm[28+k]:=x mod 256;x:=x div 256;end;
outfix(28);end;{:120}{125:}function getbytes(k:integer;
issigned:boolean):integer;var a:integer;
begin if vfptr+k>vflimit then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Packet ended prematurely');end;
k:=vflimit-vfptr;end;a:=vf[vfptr];
if(k=4)or issigned then if a>=128 then a:=a-256;vfptr:=vfptr+1;
while k>1 do begin a:=a*256+vf[vfptr];vfptr:=vfptr+1;k:=k-1;end;
getbytes:=a;end;{:125}{131:}function vfinput:boolean;label 9999,10;
var vfptr:0..vfsize;k:integer;c:integer;begin{31:}read(vffile,tempbyte);
if tempbyte<>247 then begin writeln('The first byte isn''t `pre''!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;{32:}
if eof(vffile)then begin writeln('The input file is only one byte long!'
);writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,tempbyte);if tempbyte<>202 then begin writeln(
'Wrong VF version number in second byte!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;if eof(vffile)then begin writeln(
'The input file is only two bytes long!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,tempbyte);vfcount:=11;vfptr:=0;
if vfptr+tempbyte>=vfsize then begin writeln(
'The file is bigger than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;
for k:=vfptr to vfptr+tempbyte-1 do begin if eof(vffile)then begin
writeln('The file ended prematurely!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,vf[k]);end;vfcount:=vfcount+tempbyte;
vfptr:=vfptr+tempbyte;
if verbose then begin for k:=0 to vfptr-1 do write(xchr[vf[k]]);
writeln(' ');end;count:=0;
for k:=0 to 7 do begin if eof(vffile)then begin writeln(
'The file ended prematurely!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,tempbyte);
if tempbyte=tfm[24+k]then count:=count+1;end;
realdsize:=(((tfm[28]*256+tfm[29])*256+tfm[30])*256+tfm[31])/1048576;
if count<>8 then begin writeln('Check sum and/or design size mismatch.')
;writeln('Data from TFM file will be assumed correct.');end{:32};{33:}
for k:=0 to 255 do packetstart[k]:=vfsize;fontptr:=0;packetfound:=false;
fontstart[0]:=vfptr;repeat if eof(vffile)then begin writeln(
'File ended without a postamble!');tempbyte:=248;
end else begin read(vffile,tempbyte);vfcount:=vfcount+1;
if tempbyte<>248 then if tempbyte>242 then{35:}
begin if packetfound or(tempbyte>=247)then begin writeln('Illegal byte '
,tempbyte:1,' at beginning of character packet!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;fontnumber[fontptr]:=vfread(tempbyte-242);
if fontptr=maxfonts then begin writeln(
'I can''t handle that many fonts!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;if vfptr+14>=vfsize then begin writeln(
'The file is bigger than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;
for k:=vfptr to vfptr+13 do begin if eof(vffile)then begin writeln(
'The file ended prematurely!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,vf[k]);end;vfcount:=vfcount+14;vfptr:=vfptr+14;
if vf[vfptr-10]>0 then begin writeln('Mapped font size is too big!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;a:=vf[vfptr-2];l:=vf[vfptr-1];
if vfptr+a+l>=vfsize then begin writeln(
'The file is bigger than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;
for k:=vfptr to vfptr+a+l-1 do begin if eof(vffile)then begin writeln(
'The file ended prematurely!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,vf[k]);end;vfcount:=vfcount+a+l;
vfptr:=vfptr+a+l;if verbose then begin{36:}
write('MAPFONT ',fontptr:1,': ');
for k:=fontstart[fontptr]+14 to vfptr-1 do write(xchr[vf[k]]);
k:=fontstart[fontptr]+5;write(' at ');
printreal((((vf[k]*256+vf[k+1])*256+vf[k+2])/1048576)*realdsize,2,2);
writeln('pt'){:36};end;{39:}fontchars[fontptr]:=vfptr;{44:}
for k:=1 to PATHMAX do curname[k]:=' ';r:=0;
for k:=fontstart[fontptr]+14 to vfptr-1 do begin r:=r+1;
if r+4>PATHMAX then begin writeln('Font name too long for me!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;curname[r]:=xchr[vf[k]];end;curname[r+1]:='.';
curname[r+2]:='t';curname[r+3]:='f';curname[r+4]:='m'{:44};
if not testreadaccess(curname,TFMFILEPATH)then writeln(
'---not loaded, TFM file can''t be opened!')else begin reset(tfmfile,
curname);fontbc:=0;fontec:=256;readtfmword;
if b2<128 then begin fontlh:=b2*256+b3;readtfmword;
if(b0<128)and(b2<128)then begin fontbc:=b0*256+b1;fontec:=b2*256+b3;end;
end;if fontbc<=fontec then if fontec>255 then writeln(
'---not loaded, bad TFM file!')else begin for k:=0 to 3+fontlh do begin
readtfmword;if k=4 then{40:}
if b0+b1+b2+b3>0 then if(b0<>vf[fontstart[fontptr]])or(b1<>vf[fontstart[
fontptr]+1])or(b2<>vf[fontstart[fontptr]+2])or(b3<>vf[fontstart[fontptr]
+3])then begin if verbose then writeln(
'Check sum in VF file being replaced by TFM check sum');
vf[fontstart[fontptr]]:=b0;vf[fontstart[fontptr]+1]:=b1;
vf[fontstart[fontptr]+2]:=b2;vf[fontstart[fontptr]+3]:=b3;end{:40};
if k=5 then{41:}
if(b0<>vf[fontstart[fontptr]+8])or(b1<>vf[fontstart[fontptr]+9])or(b2<>
vf[fontstart[fontptr]+10])or(b3<>vf[fontstart[fontptr]+11])then begin
writeln('Design size in VF file being replaced by TFM design size');
vf[fontstart[fontptr]+8]:=b0;vf[fontstart[fontptr]+9]:=b1;
vf[fontstart[fontptr]+10]:=b2;vf[fontstart[fontptr]+11]:=b3;end{:41};
end;for k:=fontbc to fontec do begin readtfmword;
if b0>0 then begin vf[vfptr]:=k;vfptr:=vfptr+1;
if vfptr=vfsize then begin writeln('I''m out of VF memory!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;end;end;end;if eof(tfmfile)then writeln(
'---trouble is brewing, TFM file ended too soon!');end;
vfptr:=vfptr+1{:39};fontptr:=fontptr+1;fontstart[fontptr]:=vfptr;
end{:35}else{46:}begin if tempbyte=242 then begin pl:=vfread(4);
c:=vfread(4);count:=vfread(4);end else begin pl:=tempbyte;c:=vfread(1);
count:=vfread(3);end;
if((c<bc)or(c>ec)or(tfm[4*(charbase+c)]=0))then begin writeln(
'Character ',c:1,' does not exist!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;if packetstart[c]<vfsize then writeln(
'Discarding earlier packet for character ',c:1);
if count<>tfmwidth(c)then writeln('Incorrect TFM width for character ',c
:1,' in VF file');if pl<0 then begin writeln('Negative packet length!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;packetstart[c]:=vfptr;
if vfptr+pl>=vfsize then begin writeln(
'The file is bigger than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;
for k:=vfptr to vfptr+pl-1 do begin if eof(vffile)then begin writeln(
'The file ended prematurely!');
writeln('Sorry, but I can''t go on; are you sure this is a VF?');
uexit(1);end;read(vffile,vf[k]);end;vfcount:=vfcount+pl;vfptr:=vfptr+pl;
packetend[c]:=vfptr-1;packetfound:=true;end{:46};end;
until tempbyte=248{:33};{34:}
while(tempbyte=248)and not eof(vffile)do begin read(vffile,tempbyte);
vfcount:=vfcount+1;end;if not eof(vffile)then begin writeln(
'There''s some extra junk at the end of the VF file.');
writeln('I''ll proceed as if it weren''t there.');end;
if vfcount mod 4<>0 then writeln('VF data not a multiple of 4 bytes'){:
34}{:31};vfinput:=true;goto 10;9999:vfinput:=false;10:end;
function organize:boolean;label 9999,10;var tfmptr:index;begin{24:}
read(tfmfile,tfm[0]);if tfm[0]>127 then begin writeln(
'The first byte of the input file exceeds 127!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;if eof(tfmfile)then begin writeln(
'The input file is only one byte long!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;read(tfmfile,tfm[1]);lf:=tfm[0]*256+tfm[1];
if lf=0 then begin writeln(
'The file claims to have length zero, but that''s impossible!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;if 4*lf-1>tfmsize then begin writeln(
'The file is bigger than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;
for tfmptr:=2 to 4*lf-1 do begin if eof(tfmfile)then begin writeln(
'The file has fewer bytes than it claims!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;read(tfmfile,tfm[tfmptr]);end;
if not eof(tfmfile)then begin writeln(
'There''s some extra junk at the end of the TFM file,');
writeln('but I''ll proceed as if it weren''t there.');end{:24};{25:}
begin tfmptr:=2;begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;lh:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;bc:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;ec:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;nw:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;nh:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;nd:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;ni:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;nl:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;nk:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;ne:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;np:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
if lh<2 then begin writeln('The header length is only ',lh:1,'!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;if nl>4*ligsize then begin writeln(
'The lig/kern program is longer than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;
if(bc>ec+1)or(ec>255)then begin writeln('The character code range ',bc:1
,'..',ec:1,'is illegal!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;if(nw=0)or(nh=0)or(nd=0)or(ni=0)then begin writeln(
'Incomplete subfiles for character dimensions!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;
if ne>256 then begin writeln('There are ',ne:1,' extensible recipes!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;
if lf<>6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+nk+ne+np then begin writeln(
'Subfile sizes don''t add up to the stated total!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
uexit(1);end;end{:25};{27:}begin charbase:=6+lh-bc;
widthbase:=charbase+ec+1;heightbase:=widthbase+nw;
depthbase:=heightbase+nh;italicbase:=depthbase+nd;
ligkernbase:=italicbase+ni;kernbase:=ligkernbase+nl;
extenbase:=kernbase+nk;parambase:=extenbase+ne-1;end{:27};
organize:=vfinput;goto 10;9999:organize:=false;10:end;{:131}{132:}
procedure dosimplethings;var i:0..32767;f:0..vfsize;k:integer;
begin{119:}if stringbalance(0,fontstart[0])then begin left;
write(vplfile,'VTITLE ');
for k:=0 to fontstart[0]-1 do write(vplfile,xchr[vf[k]]);right;
end else begin perfect:=false;if charsonline>0 then writeln(' ');
charsonline:=0;
writeln('Bad VF file: ','Title is not a balanced ASCII string');
end{:119};{70:}begin fonttype:=0;if lh>=12 then begin{75:}
begin checkBCPL(32,40);
if(tfm[32]>=11)and(tfm[33]=84)and(tfm[34]=69)and(tfm[35]=88)and(tfm[36]=
32)and(tfm[37]=77)and(tfm[38]=65)and(tfm[39]=84)and(tfm[40]=72)and(tfm[
41]=32)then begin if(tfm[42]=83)and(tfm[43]=89)then fonttype:=1 else if(
tfm[42]=69)and(tfm[43]=88)then fonttype:=2;end;end{:75};
if lh>=17 then begin{77:}left;write(vplfile,'FAMILY');checkBCPL(72,20);
outBCPL(72);right{:77};if lh>=18 then{78:}begin left;
write(vplfile,'FACE');outface(95);right;for i:=18 to lh-1 do begin left;
write(vplfile,'HEADER D ',i:1);outoctal(24+4*i,4);right;end;end{:78};
end;{76:}left;write(vplfile,'CODINGSCHEME');outBCPL(32);right{:76};end;
{73:}left;write(vplfile,'DESIGNSIZE');
if tfm[28]>127 then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Design size ','negative','!');end;
writeln('I''ve set it to 10 points.');write(vplfile,' D 10');
end else if(tfm[28]=0)and(tfm[29]<16)then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Design size ','too small','!');end;
writeln('I''ve set it to 10 points.');write(vplfile,' D 10');
end else outfix(28);right;
write(vplfile,'(COMMENT DESIGNSIZE IS IN POINTS)');outln;
write(vplfile,'(COMMENT OTHER SIZES ARE MULTIPLES OF DESIGNSIZE)');
outln{:73};{71:}left;write(vplfile,'CHECKSUM');outoctal(24,4);right{:71}
;{79:}if(lh>17)and(tfm[92]>127)then begin left;
write(vplfile,'SEVENBITSAFEFLAG TRUE');right;end{:79};end{:70};{80:}
if np>0 then begin left;write(vplfile,'FONTDIMEN');outln;
for i:=1 to np do{82:}begin left;
if i=1 then write(vplfile,'SLANT')else begin if(tfm[4*(parambase+i)]>0)
and(tfm[4*(parambase+i)]<255)then begin tfm[4*(parambase+i)]:=0;
tfm[(4*(parambase+i))+1]:=0;tfm[(4*(parambase+i))+2]:=0;
tfm[(4*(parambase+i))+3]:=0;begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Parameter',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;{83:}
if i<=7 then case i of 2:write(vplfile,'SPACE');
3:write(vplfile,'STRETCH');4:write(vplfile,'SHRINK');
5:write(vplfile,'XHEIGHT');6:write(vplfile,'QUAD');
7:write(vplfile,'EXTRASPACE')end else if(i<=22)and(fonttype=1)then case
i of 8:write(vplfile,'NUM1');9:write(vplfile,'NUM2');
10:write(vplfile,'NUM3');11:write(vplfile,'DENOM1');
12:write(vplfile,'DENOM2');13:write(vplfile,'SUP1');
14:write(vplfile,'SUP2');15:write(vplfile,'SUP3');
16:write(vplfile,'SUB1');17:write(vplfile,'SUB2');
18:write(vplfile,'SUPDROP');19:write(vplfile,'SUBDROP');
20:write(vplfile,'DELIM1');21:write(vplfile,'DELIM2');
22:write(vplfile,'AXISHEIGHT')end else if(i<=13)and(fonttype=2)then if i
=8 then write(vplfile,'DEFAULTRULETHICKNESS')else write(vplfile,
'BIGOPSPACING',i-8:1)else write(vplfile,'PARAMETER D ',i:1){:83};end;
outfix(4*(parambase+i));right;end{:82};right;end;{81:}
if(fonttype=1)and(np<>22)then writeln(
'Unusual number of fontdimen parameters for a math symbols font (',np:1,
' not 22).')else if(fonttype=2)and(np<>13)then writeln(
'Unusual number of fontdimen parameters for an extension font (',np:1,
' not 13).'){:81};{:80};{121:}for f:=0 to fontptr-1 do begin left;
write(vplfile,'MAPFONT D ',f:1);outln;{122:}a:=vf[fontstart[f]+12];
l:=vf[fontstart[f]+13];
if a>0 then if not stringbalance(fontstart[f]+14,a)then begin perfect:=
false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Improper font area will be ignored');
end else begin left;write(vplfile,'FONTAREA ');
for k:=fontstart[f]+14 to fontstart[f]+a+13 do write(vplfile,xchr[vf[k]]
);right;end;
if(l=0)or not stringbalance(fontstart[f]+14+a,l)then begin perfect:=
false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Improper font name will be ignored');
end else begin left;write(vplfile,'FONTNAME ');
for k:=fontstart[f]+14+a to fontstart[f]+a+l+13 do write(vplfile,xchr[vf
[k]]);right;end{:122};for k:=0 to 11 do tfm[k]:=vf[fontstart[f]+k];
if tfm[0]+tfm[1]+tfm[2]+tfm[3]>0 then begin left;
write(vplfile,'FONTCHECKSUM');outoctal(0,4);right;end;left;
write(vplfile,'FONTAT');outfix(4);right;left;write(vplfile,'FONTDSIZE');
outfix(8);right;right;end{:121};{84:}
if(tfm[4*widthbase]>0)or(tfm[4*widthbase+1]>0)or(tfm[4*widthbase+2]>0)or
(tfm[4*widthbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','width[0] should be zero.');end;
if(tfm[4*heightbase]>0)or(tfm[4*heightbase+1]>0)or(tfm[4*heightbase+2]>0
)or(tfm[4*heightbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','height[0] should be zero.');end;
if(tfm[4*depthbase]>0)or(tfm[4*depthbase+1]>0)or(tfm[4*depthbase+2]>0)or
(tfm[4*depthbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','depth[0] should be zero.');end;
if(tfm[4*italicbase]>0)or(tfm[4*italicbase+1]>0)or(tfm[4*italicbase+2]>0
)or(tfm[4*italicbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','italic[0] should be zero.');end;
for i:=0 to nw-1 do if(tfm[4*(widthbase+i)]>0)and(tfm[4*(widthbase+i)]<
255)then begin tfm[4*(widthbase+i)]:=0;tfm[(4*(widthbase+i))+1]:=0;
tfm[(4*(widthbase+i))+2]:=0;tfm[(4*(widthbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Width',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;
for i:=0 to nh-1 do if(tfm[4*(heightbase+i)]>0)and(tfm[4*(heightbase+i)]
<255)then begin tfm[4*(heightbase+i)]:=0;tfm[(4*(heightbase+i))+1]:=0;
tfm[(4*(heightbase+i))+2]:=0;tfm[(4*(heightbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Height',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;
for i:=0 to nd-1 do if(tfm[4*(depthbase+i)]>0)and(tfm[4*(depthbase+i)]<
255)then begin tfm[4*(depthbase+i)]:=0;tfm[(4*(depthbase+i))+1]:=0;
tfm[(4*(depthbase+i))+2]:=0;tfm[(4*(depthbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Depth',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;
for i:=0 to ni-1 do if(tfm[4*(italicbase+i)]>0)and(tfm[4*(italicbase+i)]
<255)then begin tfm[4*(italicbase+i)]:=0;tfm[(4*(italicbase+i))+1]:=0;
tfm[(4*(italicbase+i))+2]:=0;tfm[(4*(italicbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Italic correction',' ',i:1,' is too big;');
end;writeln('I have set it to zero.');end;
if nk>0 then for i:=0 to nk-1 do if(tfm[4*(kernbase+i)]>0)and(tfm[4*(
kernbase+i)]<255)then begin tfm[4*(kernbase+i)]:=0;
tfm[(4*(kernbase+i))+1]:=0;tfm[(4*(kernbase+i))+2]:=0;
tfm[(4*(kernbase+i))+3]:=0;begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Kern',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;{:84};end;{:132}{133:}
function domap(c:byte):boolean;label 9999,10;var k:integer;f:0..vfsize;
begin{124:}if packetstart[c]=vfsize then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Missing packet for character ',c:1);
end else begin left;write(vplfile,'MAP');outln;top:=0;wstack[0]:=0;
xstack[0]:=0;ystack[0]:=0;zstack[0]:=0;vfptr:=packetstart[c];
vflimit:=packetend[c]+1;f:=0;while vfptr<vflimit do begin o:=vf[vfptr];
vfptr:=vfptr+1;
if((o>=0)and(o<=127))or((o>=128)and(o<=131))or((o>=133)and(o<=136))then
begin if o>=128 then if o>=133 then c:=getbytes(o-132,false)else c:=
getbytes(o-127,false)else c:=o;if f=fontptr then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Character ',c:1,
' in undeclared font will be ignored');
end else begin vf[fontstart[f+1]-1]:=c;k:=fontchars[f];
while vf[k]<>c do k:=k+1;
if k=fontstart[f+1]-1 then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Character ',c:1,' in font ',f:1,
' will be ignored');
end else begin if o>=133 then write(vplfile,'(PUSH)');left;
write(vplfile,'SETCHAR');outchar(c);
if o>=133 then write(vplfile,')(POP');right;end;end;
end else case o of{126:}138:;
141:begin if top=maxstack then begin writeln('Stack overflow!');
uexit(1);end;top:=top+1;wstack[top]:=wstack[top-1];
xstack[top]:=xstack[top-1];ystack[top]:=ystack[top-1];
zstack[top]:=zstack[top-1];write(vplfile,'(PUSH)');outln;end;
142:if top=0 then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','More pops than pushes!');
end else begin top:=top-1;write(vplfile,'(POP)');outln;end;
132,137:begin if o=137 then write(vplfile,'(PUSH)');left;
write(vplfile,'SETRULE');outasfix(getbytes(4,true));
outasfix(getbytes(4,true));if o=137 then write(vplfile,')(POP');right;
end;{:126}{127:}143,144,145,146:begin write(vplfile,'(MOVERIGHT');
outasfix(getbytes(o-142,true));write(vplfile,')');outln;end;
147,148,149,150,151:begin if o<>147 then wstack[top]:=getbytes(o-147,
true);write(vplfile,'(MOVERIGHT');outasfix(wstack[top]);
write(vplfile,')');outln;end;
152,153,154,155,156:begin if o<>152 then xstack[top]:=getbytes(o-152,
true);write(vplfile,'(MOVERIGHT');outasfix(xstack[top]);
write(vplfile,')');outln;end;
157,158,159,160:begin write(vplfile,'(MOVEDOWN');
outasfix(getbytes(o-156,true));write(vplfile,')');outln;end;
161,162,163,164,165:begin if o<>161 then ystack[top]:=getbytes(o-161,
true);write(vplfile,'(MOVEDOWN');outasfix(ystack[top]);
write(vplfile,')');outln;end;
166,167,168,169,170:begin if o<>166 then zstack[top]:=getbytes(o-166,
true);write(vplfile,'(MOVEDOWN');outasfix(zstack[top]);
write(vplfile,')');outln;end;{:127}{128:}
171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,
189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
225,226,227,228,229,230,231,232,233,234,235,236,237,238:begin f:=0;
if o>=235 then fontnumber[fontptr]:=getbytes(o-234,false)else fontnumber
[fontptr]:=o-171;while fontnumber[f]<>fontnumber[fontptr]do f:=f+1;
if f=fontptr then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Undeclared font selected');
end else begin write(vplfile,'(SELECTFONT D ',f:1,')');outln;end;end;
{:128}{130:}239,240,241,242:begin k:=getbytes(o-238,false);
if k<0 then begin perfect:=false;if charsonline>0 then writeln(' ');
charsonline:=0;writeln('Bad VF file: ','String of negative length!');
end else begin left;if k+vfptr>vflimit then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Special command truncated to packet length');
end;k:=vflimit-vfptr;end;
if(k>64)or not stringbalance(vfptr,k)then begin write(vplfile,
'SPECIALHEX ');
while k>0 do begin if k mod 32=0 then outln else if k mod 4=0 then write
(vplfile,' ');begin a:=vf[vfptr]div 16;
if a<10 then write(vplfile,a:1)else write(vplfile,xchr[a+55]);end;
begin a:=vf[vfptr]mod 16;
if a<10 then write(vplfile,a:1)else write(vplfile,xchr[a+55]);end;
vfptr:=vfptr+1;k:=k-1;end;end else begin write(vplfile,'SPECIAL ');
while k>0 do begin write(vplfile,xchr[vf[vfptr]]);vfptr:=vfptr+1;k:=k-1;
end;end;right;end;end;{:130}
139,140,243,244,245,246,247,248,249,250,251,252,253,254,255:begin
perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','Illegal DVI code ',o:1,' will be ignored');end;
end;end;if top>0 then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad VF file: ','More pushes than pops!');end;
repeat write(vplfile,'(POP)');top:=top-1;until top=0;end;right;end{:124}
;domap:=true;goto 10;9999:domap:=false;10:end;
function docharacters:boolean;label 9999,10;var c:byte;k:index;
ai:0..ligsize;begin{100:}sortptr:=0;
for c:=bc to ec do if tfm[4*(charbase+c)]>0 then begin if charsonline=8
then begin writeln(' ');charsonline:=1;
end else begin if charsonline>0 then write(' ');
if verbose then charsonline:=charsonline+1;end;
if verbose then printoctal(c);left;write(vplfile,'CHARACTER');
outchar(c);outln;{101:}begin left;write(vplfile,'CHARWD');
if tfm[4*(charbase+c)]>=nw then begin perfect:=false;writeln(' ');
write('Width',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');
end else outfix(4*(widthbase+tfm[4*(charbase+c)]));right;end{:101};
if(tfm[4*(charbase+c)+1]div 16)>0 then{102:}
if(tfm[4*(charbase+c)+1]div 16)>=nh then begin perfect:=false;
writeln(' ');write('Height',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');
end else begin left;write(vplfile,'CHARHT');
outfix(4*(heightbase+(tfm[4*(charbase+c)+1]div 16)));right;end{:102};
if(tfm[4*(charbase+c)+1]mod 16)>0 then{103:}
if(tfm[4*(charbase+c)+1]mod 16)>=nd then begin perfect:=false;
writeln(' ');write('Depth',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');
end else begin left;write(vplfile,'CHARDP');
outfix(4*(depthbase+(tfm[4*(charbase+c)+1]mod 16)));right;end{:103};
if(tfm[4*(charbase+c)+2]div 4)>0 then{104:}
if(tfm[4*(charbase+c)+2]div 4)>=ni then begin perfect:=false;
writeln(' ');write('Italic correction',' index for character ');
printoctal(c);writeln(' is too large;');
writeln('so I reset it to zero.');end else begin left;
write(vplfile,'CHARIC');
outfix(4*(italicbase+(tfm[4*(charbase+c)+2]div 4)));right;end{:104};
case(tfm[4*(charbase+c)+2]mod 4)of 0:;1:{105:}begin left;
write(vplfile,'COMMENT');outln;i:=tfm[4*(charbase+c)+3];
r:=4*(ligkernbase+(i));if tfm[r]>128 then i:=256*tfm[r+2]+tfm[r+3];
repeat{96:}begin k:=4*(ligkernbase+(i));
if tfm[k]>128 then begin if 256*tfm[k+2]+tfm[k+3]>=nl then begin perfect
:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'Ligature unconditional stop command address is too big.');end;
end else if tfm[k+2]>=128 then{98:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Kern step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;left;
write(vplfile,'KRN');outchar(tfm[k+1]);r:=256*(tfm[k+2]-128)+tfm[k+3];
if r>=nk then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Kern index too large.');end;
write(vplfile,' R 0.0');end else outfix(4*(kernbase+r));right;end{:98}
else{99:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;
if((tfm[k+3]<bc)or(tfm[k+3]>ec)or(tfm[4*(charbase+tfm[k+3])]=0))then
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step produces the',
' nonexistent character ');printoctal(tfm[k+3]);writeln('.');
tfm[k+3]:=bc;end;left;r:=tfm[k+2];
if(r=4)or((r>7)and(r<>11))then begin writeln(
'Ligature step with nonstandard code changed to LIG');r:=0;tfm[k+2]:=0;
end;if r mod 4>1 then write(vplfile,'/');write(vplfile,'LIG');
if odd(r)then write(vplfile,'/');while r>3 do begin write(vplfile,'>');
r:=r-4;end;outchar(tfm[k+1]);outchar(tfm[k+3]);right;end{:99};
if tfm[k]>0 then if level=1 then{97:}
begin if tfm[k]>=128 then write(vplfile,'(STOP)')else begin count:=0;
for ai:=i+1 to i+tfm[k]do if activity[ai]=2 then count:=count+1;
write(vplfile,'(SKIP D ',count:1,')');end;outln;end{:97};end{:96};
if tfm[k]>=128 then i:=nl else i:=i+1+tfm[k];until i>=nl;right;end{:105}
;2:{106:}begin r:=tfm[4*(charbase+c)+3];
if((r<bc)or(r>ec)or(tfm[4*(charbase+r)]=0))then begin begin perfect:=
false;if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Character list link to',
' nonexistent character ');printoctal(r);writeln('.');end;
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;
end else begin while(r<c)and((tfm[4*(charbase+r)+2]mod 4)=2)do r:=tfm[4*
(charbase+r)+3];if r=c then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Cycle in a character list!');end;
write('Character ');printoctal(c);writeln(' now ends the list.');
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;
end else begin left;write(vplfile,'NEXTLARGER');
outchar(tfm[4*(charbase+c)+3]);right;end;end;end{:106};3:{107:}
if tfm[4*(charbase+c)+3]>=ne then begin begin perfect:=false;
writeln(' ');write('Extensible',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');end;
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;
end else begin left;write(vplfile,'VARCHAR');outln;{108:}
for k:=0 to 3 do if(k=3)or(tfm[4*(extenbase+tfm[4*(charbase+c)+3])+k]>0)
then begin left;case k of 0:write(vplfile,'TOP');1:write(vplfile,'MID');
2:write(vplfile,'BOT');3:write(vplfile,'REP')end;
if((tfm[4*(extenbase+tfm[4*(charbase+c)+3])+k]<bc)or(tfm[4*(extenbase+
tfm[4*(charbase+c)+3])+k]>ec)or(tfm[4*(charbase+tfm[4*(extenbase+tfm[4*(
charbase+c)+3])+k])]=0))then outchar(c)else outchar(tfm[4*(extenbase+tfm
[4*(charbase+c)+3])+k]);right;end{:108};right;end{:107};end;
if not domap(c)then goto 9999;right;end{:100};docharacters:=true;
goto 10;9999:docharacters:=false;10:end;{:133}{134:}begin initialize;
if not organize then goto 9999;dosimplethings;{88:}
if nl>0 then begin for ai:=0 to nl-1 do activity[ai]:=0;{91:}
if tfm[4*(ligkernbase+(0))]=255 then begin left;
write(vplfile,'BOUNDARYCHAR');boundarychar:=tfm[4*(ligkernbase+(0))+1];
outchar(boundarychar);right;activity[0]:=1;end;
if tfm[4*(ligkernbase+(nl-1))]=255 then begin r:=256*tfm[4*(ligkernbase+
(nl-1))+2]+tfm[4*(ligkernbase+(nl-1))+3];
if r>=nl then begin perfect:=false;writeln(' ');
write('Ligature/kern starting index for boundarychar is too large;');
writeln('so I removed it.');end else begin labelptr:=1;
labeltable[1].cc:=256;labeltable[1].rr:=r;bcharlabel:=r;activity[r]:=2;
end;activity[nl-1]:=1;end{:91};end;{89:}
for c:=bc to ec do if(tfm[4*(charbase+c)+2]mod 4)=1 then begin r:=tfm[4*
(charbase+c)+3];
if r<nl then begin if tfm[4*(ligkernbase+(r))]>128 then begin r:=256*tfm
[4*(ligkernbase+(r))+2]+tfm[4*(ligkernbase+(r))+3];
if r<nl then if activity[tfm[4*(charbase+c)+3]]=0 then activity[tfm[4*(
charbase+c)+3]]:=1;end;end;if r>=nl then begin perfect:=false;
writeln(' ');write('Ligature/kern starting index for character ');
printoctal(c);writeln(' is too large;');writeln('so I removed it.');
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;end else{90:}
begin sortptr:=labelptr;
while labeltable[sortptr].rr>r do begin labeltable[sortptr+1]:=
labeltable[sortptr];sortptr:=sortptr-1;end;labeltable[sortptr+1].cc:=c;
labeltable[sortptr+1].rr:=r;labelptr:=labelptr+1;activity[r]:=2;end{:90}
;end;labeltable[labelptr+1].rr:=ligsize;{:89};if nl>0 then begin left;
write(vplfile,'LIGTABLE');outln;{92:}
for ai:=0 to nl-1 do if activity[ai]=2 then begin r:=tfm[4*(ligkernbase+
(ai))];if r<128 then begin r:=r+ai+1;
if r>=nl then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Ligature/kern step ',ai:1,' skips too far;');
end;writeln('I made it stop.');tfm[4*(ligkernbase+(ai))]:=128;
end else activity[r]:=2;end;end{:92};{93:}sortptr:=1;
for acti:=0 to nl-1 do if activity[acti]<>1 then begin i:=acti;{95:}
if activity[i]=0 then begin if level=1 then begin left;
write(vplfile,'COMMENT THIS PART OF THE PROGRAM IS NEVER USED!');outln;
end end else if level=2 then right{:95};{94:}
while i=labeltable[sortptr].rr do begin left;write(vplfile,'LABEL');
if labeltable[sortptr].cc=256 then write(vplfile,' BOUNDARYCHAR')else
outchar(labeltable[sortptr].cc);right;sortptr:=sortptr+1;end{:94};{96:}
begin k:=4*(ligkernbase+(i));
if tfm[k]>128 then begin if 256*tfm[k+2]+tfm[k+3]>=nl then begin perfect
:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'Ligature unconditional stop command address is too big.');end;
end else if tfm[k+2]>=128 then{98:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Kern step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;left;
write(vplfile,'KRN');outchar(tfm[k+1]);r:=256*(tfm[k+2]-128)+tfm[k+3];
if r>=nk then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Kern index too large.');end;
write(vplfile,' R 0.0');end else outfix(4*(kernbase+r));right;end{:98}
else{99:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;
if((tfm[k+3]<bc)or(tfm[k+3]>ec)or(tfm[4*(charbase+tfm[k+3])]=0))then
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step produces the',
' nonexistent character ');printoctal(tfm[k+3]);writeln('.');
tfm[k+3]:=bc;end;left;r:=tfm[k+2];
if(r=4)or((r>7)and(r<>11))then begin writeln(
'Ligature step with nonstandard code changed to LIG');r:=0;tfm[k+2]:=0;
end;if r mod 4>1 then write(vplfile,'/');write(vplfile,'LIG');
if odd(r)then write(vplfile,'/');while r>3 do begin write(vplfile,'>');
r:=r-4;end;outchar(tfm[k+1]);outchar(tfm[k+3]);right;end{:99};
if tfm[k]>0 then if level=1 then{97:}
begin if tfm[k]>=128 then write(vplfile,'(STOP)')else begin count:=0;
for ai:=i+1 to i+tfm[k]do if activity[ai]=2 then count:=count+1;
write(vplfile,'(SKIP D ',count:1,')');end;outln;end{:97};end{:96};end;
if level=2 then right{:93};right;{112:}hashptr:=0;yligcycle:=256;
for hh:=0 to hashsize do hash[hh]:=0;
for c:=bc to ec do if(tfm[4*(charbase+c)+2]mod 4)=1 then begin i:=tfm[4*
(charbase+c)+3];
if tfm[4*(ligkernbase+(i))]>128 then i:=256*tfm[4*(ligkernbase+(i))+2]+
tfm[4*(ligkernbase+(i))+3];{113:}repeat hashinput;
k:=tfm[4*(ligkernbase+(i))];if k>=128 then i:=nl else i:=i+1+k;
until i>=nl{:113};end;if bcharlabel<nl then begin c:=256;i:=bcharlabel;
{113:}repeat hashinput;k:=tfm[4*(ligkernbase+(i))];
if k>=128 then i:=nl else i:=i+1+k;until i>=nl{:113};end;
if hashptr=hashsize then begin writeln(
'Sorry, I haven''t room for so many ligature/kern pairs!');uexit(1);end;
for hh:=1 to hashptr do begin r:=hashlist[hh];
if class[r]>0 then r:=ligf(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);end;
if yligcycle<256 then begin write(
'Infinite ligature loop starting with ');
if xligcycle=256 then write('boundary')else printoctal(xligcycle);
write(' and ');printoctal(yligcycle);writeln('!');
write(vplfile,'(INFINITE LIGATURE LOOP MUST BE BROKEN!)');uexit(1);
end{:112};end{:88};{109:}
if ne>0 then for c:=0 to ne-1 do for d:=0 to 3 do begin k:=4*(extenbase+
c)+d;if(tfm[k]>0)or(d=3)then begin if((tfm[k]<bc)or(tfm[k]>ec)or(tfm[4*(
charbase+tfm[k])]=0))then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Extensible recipe involves the',
' nonexistent character ');printoctal(tfm[k]);writeln('.');end;
if d<3 then tfm[k]:=0;end;end;end{:109};
if not docharacters then goto 9999;if verbose then writeln('.');
if level<>0 then writeln('This program isn''t working!');
if not perfect then begin write(vplfile,
'(COMMENT THE TFM AND/OR VF FILE WAS BAD, ');
write(vplfile,'SO THE DATA HAS BEEN CHANGED!)');end;9999:end.{:134}
