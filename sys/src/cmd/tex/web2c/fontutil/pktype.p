{4:}program PKtype;type{9:}ASCIIcode=32..126;{:9}{10:}
textfile=packed file of char;{:10}{30:}eightbits=0..255;
bytefile=packed file of eightbits;{:30}var{11:}
xord:array[char]of ASCIIcode;xchr:array[0..255]of char;{:11}{31:}
pkfile:bytefile;{:31}{33:}
pkname,typname:packed array[1..PATHMAX]of char;curloc:integer;{:33}{37:}
termpos:integer;{:37}{39:}magnification:integer;designsize:integer;
checksum:integer;hppp,vppp:integer;{:39}{41:}i,j:integer;
flagbyte:integer;endofpacket:integer;width,height:integer;
xoff,yoff:integer;tfmwidth:integer;tfms:array[0..255]of integer;
dx,dy:integer;dxs,dys:array[0..255]of integer;
status:array[0..255]of boolean;dynf:integer;car:integer;
packetlength:integer;{:41}{47:}inputbyte:eightbits;bitweight:eightbits;
nybble:eightbits;{:47}{51:}repeatcount:integer;rowsleft:integer;
turnon:boolean;hbit:integer;count:integer;{:51}procedure initialize;
var i:integer;begin writeln(stdout,'This is PKtype, C Version 2.3');
setpaths(PKFILEPATHBIT);{12:}for i:=0 to 31 do xchr[i]:='?';
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
for i:=32 to 126 do xord[xchr[i]]:=i;{:13}end;{:4}{32:}
procedure openpkfile;var j,k:integer;begin k:=1;
if argc<>2 then begin writeln(stdout,'Usage: pktype <pk file>.');
uexit(1)end;argv(1,pkname);
if testreadaccess(pkname,PKFILEPATH)then begin reset(pkfile,pkname);
curloc:=0;end else begin printpascalstring(pkname);
begin writeln(stdout,': PK file not found.');uexit(1)end;end;end;{:32}
{34:}function getbyte:integer;var b:eightbits;
begin if eof(pkfile)then getbyte:=0 else begin read(pkfile,b);
curloc:=curloc+1;getbyte:=b;end;end;{function signedbyte:integer;
var b:eightbits;begin read(pkfile,b);curloc:=curloc+1;
if b<128 then signedbyte:=b else signedbyte:=b-256;end;}
function gettwobytes:integer;var a,b:eightbits;begin read(pkfile,a);
read(pkfile,b);curloc:=curloc+2;gettwobytes:=a*256+b;end;
{function signedpair:integer;var a,b:eightbits;begin read(pkfile,a);
read(pkfile,b);curloc:=curloc+2;
if a<128 then signedpair:=a*256+b else signedpair:=(a-256)*256+b;end;
function getthreebytes:integer;var a,b,c:eightbits;begin read(pkfile,a);
read(pkfile,b);read(pkfile,c);curloc:=curloc+3;
getthreebytes:=(a*256+b)*256+c;end;function signedtrio:integer;
var a,b,c:eightbits;begin read(pkfile,a);read(pkfile,b);read(pkfile,c);
curloc:=curloc+3;
if a<128 then signedtrio:=(a*256+b)*256+c else signedtrio:=((a-256)*256+
b)*256+c;end;}function signedquad:integer;var a,b,c,d:eightbits;
begin read(pkfile,a);read(pkfile,b);read(pkfile,c);read(pkfile,d);
curloc:=curloc+4;
if a<128 then signedquad:=((a*256+b)*256+c)*256+d else signedquad:=(((a
-256)*256+b)*256+c)*256+d;end;{:34}{45:}function getnyb:integer;
var temp:eightbits;begin if bitweight=0 then begin inputbyte:=getbyte;
bitweight:=16;end;temp:=inputbyte div bitweight;
inputbyte:=inputbyte-temp*bitweight;bitweight:=bitweight div 16;
getnyb:=temp;end;function getbit:boolean;var temp:boolean;
begin bitweight:=bitweight div 2;
if bitweight=0 then begin inputbyte:=getbyte;bitweight:=128;end;
temp:=inputbyte>=bitweight;if temp then inputbyte:=inputbyte-bitweight;
getbit:=temp;end;{:45}{46:}procedure sendout(repeatcount:boolean;
value:integer);var i,len:integer;begin i:=10;len:=1;
while value>=i do begin len:=len+1;i:=i*10;end;
if repeatcount or not turnon then len:=len+2;
if termpos+len>78 then begin termpos:=len+2;writeln(stdout,' ');
write(stdout,'  ');end else termpos:=termpos+len;
if repeatcount then write(stdout,'[',value:1,']')else if turnon then
write(stdout,value:1)else write(stdout,'(',value:1,')');end;{23:}
function pkpackednum:integer;var i,j:integer;begin i:=getnyb;
if i=0 then begin repeat j:=getnyb;i:=i+1;until j<>0;
while i>0 do begin j:=j*16+getnyb;i:=i-1;end;
pkpackednum:=j-15+(13-dynf)*16+dynf;
end else if i<=dynf then pkpackednum:=i else if i<14 then pkpackednum:=(
i-dynf-1)*16+getnyb+dynf+1 else begin if repeatcount<>0 then begin
writeln(stdout,'Second repeat count for this row!');uexit(1)end;
repeatcount:=1;if i=14 then repeatcount:=pkpackednum;
sendout(true,repeatcount);pkpackednum:=pkpackednum;end;end;{:23}{:46}
{52:}procedure skipspecials;var i,j:integer;
begin repeat flagbyte:=getbyte;
if flagbyte>=240 then case flagbyte of 240,241,242,243:begin write(
stdout,(curloc-1):1,':  Special: ''');i:=0;
for j:=240 to flagbyte do i:=256*i+getbyte;
for j:=1 to i do write(stdout,xchr[getbyte]);writeln(stdout,'''');end;
244:writeln(stdout,(curloc-1):1,':  Num special: ',signedquad:1);
245:writeln(stdout,(curloc-1):1,':  Postamble');
246:writeln(stdout,(curloc-1):1,':  No op');
247,248,249,250,251,252,253,254,255:begin writeln(stdout,'Unexpected ',
flagbyte:1,'!');uexit(1)end;end;until(flagbyte<240)or(flagbyte=245);end;
{:52}{55:}begin initialize;openpkfile;{38:}
if getbyte<>247 then begin writeln(stdout,
'Bad PK file:  pre command missing!');uexit(1)end;
if getbyte<>89 then begin writeln(stdout,'Wrong version of PK file!');
uexit(1)end;j:=getbyte;write(stdout,'''');
for i:=1 to j do write(stdout,xchr[getbyte]);writeln(stdout,'''');
designsize:=signedquad;writeln(stdout,'Design size = ',designsize:1);
checksum:=signedquad;writeln(stdout,'Checksum = ',checksum:1);
hppp:=signedquad;vppp:=signedquad;
write(stdout,'Resolution: horizontal = ',hppp:1,'  vertical = ',vppp:1);
magnification:=round(hppp*72.27/65536);
writeln(stdout,'  (',magnification:1,' dpi)');
if hppp<>vppp then writeln(stdout,'Warning:  aspect ratio not 1:1!'){:38
};skipspecials;while flagbyte<>245 do begin{40:}
write(stdout,(curloc-1):1,':  Flag byte = ',flagbyte:1);
dynf:=flagbyte div 16;flagbyte:=flagbyte mod 16;turnon:=flagbyte>=8;
if turnon then flagbyte:=flagbyte-8;if flagbyte=7 then{42:}
begin packetlength:=signedquad;car:=signedquad;
endofpacket:=packetlength+curloc;packetlength:=packetlength+9;
tfmwidth:=signedquad;dx:=signedquad;dy:=signedquad;width:=signedquad;
height:=signedquad;xoff:=signedquad;yoff:=signedquad;end{:42}
else if flagbyte>3 then{43:}
begin packetlength:=(flagbyte-4)*65536+gettwobytes;car:=getbyte;
endofpacket:=packetlength+curloc;packetlength:=packetlength+4;
i:=getbyte;tfmwidth:=i*65536+gettwobytes;dx:=gettwobytes*65536;dy:=0;
width:=gettwobytes;height:=gettwobytes;xoff:=gettwobytes;
yoff:=gettwobytes;if xoff>32767 then xoff:=xoff-65536;
if yoff>32767 then yoff:=yoff-65536;end{:43}else{44:}
begin packetlength:=flagbyte*256+getbyte;car:=getbyte;
endofpacket:=packetlength+curloc;packetlength:=packetlength+3;
i:=getbyte;tfmwidth:=i*65536+gettwobytes;dx:=getbyte*65536;dy:=0;
width:=getbyte;height:=getbyte;xoff:=getbyte;yoff:=getbyte;
if xoff>127 then xoff:=xoff-256;if yoff>127 then yoff:=yoff-256;end{:44}
;
writeln(stdout,'  Character = ',car:1,'  Packet length = ',packetlength:
1);writeln(stdout,'  Dynamic packing variable = ',dynf:1);
write(stdout,'  TFM width = ',tfmwidth:1,'  dx = ',dx:1);
if dy<>0 then writeln(stdout,'  dy = ',dy:1)else writeln(stdout,' ');
writeln(stdout,'  Height = ',height:1,'  Width = ',width:1,
'  X-offset = ',xoff:1,'  Y-offset = ',yoff:1);{48:}bitweight:=0;
if dynf=14 then{49:}
begin for i:=1 to height do begin write(stdout,'  ');
for j:=1 to width do if getbit then write(stdout,'*')else write(stdout,
'.');writeln(stdout,' ');end;end{:49}else{50:}begin termpos:=2;
write(stdout,'  ');rowsleft:=height;hbit:=width;repeatcount:=0;
while rowsleft>0 do begin count:=pkpackednum;sendout(false,count);
if count>=hbit then begin rowsleft:=rowsleft-repeatcount-1;
repeatcount:=0;count:=count-hbit;hbit:=width;
rowsleft:=rowsleft-count div width;count:=count mod width;end;
hbit:=hbit-count;turnon:=not turnon;end;writeln(stdout,' ');
if(rowsleft<>0)or(hbit<>width)then begin writeln(stdout,
'Bad PK file: More bits than required!');uexit(1)end;end{:50}{:48};
if endofpacket<>curloc then begin writeln(stdout,
'Bad PK file: Bad packet length!');uexit(1)end{:40};skipspecials;end;
j:=0;while not eof(pkfile)do begin i:=getbyte;
if i<>246 then begin writeln(stdout,'Bad byte at end of file: ',i:1);
uexit(1)end;writeln(stdout,(curloc-1):1,':  No op');j:=j+1;end;
writeln(stdout,curloc:1,' bytes read from packed file.');end.{:55}
