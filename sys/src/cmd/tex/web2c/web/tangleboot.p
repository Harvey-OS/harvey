{2:}program TANGLE;label 9999;const{8:}bufsize=100;maxbytes=45000;
maxtoks=50000;maxnames=4000;maxtexts=2000;hashsize=353;longestname=400;
linelength=72;outbufsize=144;stacksize=100;maxidlength=50;
unambiglength=20;{:8}type{11:}ASCIIcode=0..255;{:11}{12:}
textfile=packed file of ASCIIcode;{:12}{37:}eightbits=0..255;
sixteenbits=0..65535;{:37}{39:}namepointer=0..maxnames;{:39}{43:}
textpointer=0..maxtexts;{:43}{78:}
outputstate=record endfield:sixteenbits;bytefield:sixteenbits;
namefield:namepointer;replfield:textpointer;modfield:0..12287;end;{:78}
var{9:}history:0..3;{:9}{13:}xord:array[ASCIIcode]of ASCIIcode;
xchr:array[ASCIIcode]of ASCIIcode;{:13}{23:}webfile:textfile;
changefile:textfile;{:23}{25:}Pascalfile:textfile;pool:textfile;{:25}
{27:}buffer:array[0..bufsize]of ASCIIcode;{:27}{29:}phaseone:boolean;
{:29}{38:}bytemem:packed array[0..2,0..maxbytes]of ASCIIcode;
tokmem:packed array[0..3,0..maxtoks]of eightbits;
bytestart:array[0..maxnames]of sixteenbits;
tokstart:array[0..maxtexts]of sixteenbits;
link:array[0..maxnames]of sixteenbits;
ilk:array[0..maxnames]of sixteenbits;
equiv:array[0..maxnames]of sixteenbits;
textlink:array[0..maxtexts]of sixteenbits;{:38}{40:}nameptr:namepointer;
stringptr:namepointer;byteptr:array[0..2]of 0..maxbytes;
poolchecksum:integer;{:40}{44:}textptr:textpointer;
tokptr:array[0..3]of 0..maxtoks;z:0..3;
{maxtokptr:array[0..3]of 0..maxtoks;}{:44}{50:}idfirst:0..bufsize;
idloc:0..bufsize;doublechars:0..bufsize;
hash,chophash:array[0..hashsize]of sixteenbits;
choppedid:array[0..unambiglength]of ASCIIcode;{:50}{65:}
modtext:array[0..longestname]of ASCIIcode;{:65}{70:}
lastunnamed:textpointer;{:70}{79:}curstate:outputstate;
stack:array[1..stacksize]of outputstate;stackptr:0..stacksize;{:79}{80:}
zo:0..3;{:80}{82:}bracelevel:eightbits;{:82}{86:}curval:integer;{:86}
{94:}outbuf:array[0..outbufsize]of ASCIIcode;outptr:0..outbufsize;
breakptr:0..outbufsize;semiptr:0..outbufsize;{:94}{95:}
outstate:eightbits;outval,outapp:integer;outsign:ASCIIcode;
lastsign:-1..+1;{:95}{100:}outcontrib:array[1..linelength]of ASCIIcode;
{:100}{124:}ii:integer;line:integer;otherline:integer;templine:integer;
limit:0..bufsize;loc:0..bufsize;inputhasended:boolean;changing:boolean;
{:124}{126:}changebuffer:array[0..bufsize]of ASCIIcode;
changelimit:0..bufsize;{:126}{143:}curmodule:namepointer;
scanninghex:boolean;{:143}{156:}nextcontrol:eightbits;{:156}{164:}
currepltext:textpointer;{:164}{171:}modulecount:0..12287;{:171}{179:}
{troubleshooting:boolean;ddt:integer;dd:integer;debugcycle:integer;
debugskipped:integer;}{:179}{185:}{wo:0..2;}{:185}{189:}
webname,chgname,pascalfilename,poolfilename:array[1..PATHMAX]of char;
{:189}{30:}{procedure debughelp;forward;}{:30}{31:}procedure error;
var j:0..outbufsize;k,l:0..bufsize;begin if phaseone then{32:}
begin if changing then write(stdout,'. (change file ')else write(stdout,
'. (');writeln(stdout,'l.',line:1,')');
if loc>=limit then l:=limit else l:=loc;
for k:=1 to l do if buffer[k-1]=9 then write(stdout,' ')else write(
stdout,xchr[buffer[k-1]]);writeln(stdout);
for k:=1 to l do write(stdout,' ');
for k:=l+1 to limit do write(stdout,xchr[buffer[k-1]]);
write(stdout,' ');end{:32}else{33:}
begin writeln(stdout,'. (l.',line:1,')');
for j:=1 to outptr do write(stdout,xchr[outbuf[j-1]]);
write(stdout,'... ');end{:33};flush(stdout);history:=2;{debughelp;}end;
{:31}{190:}procedure scanargs;var dotpos,slashpos,i,a:integer;c:char;
fname:array[1..PATHMAX]of char;foundweb,foundchange:boolean;
begin foundweb:=false;foundchange:=false;
for a:=1 to argc-1 do begin argv(a,fname);
if fname[1]<>'-'then begin if not foundweb then{191:}begin dotpos:=-1;
slashpos:=-1;i:=1;
while(fname[i]<>' ')and(i<=PATHMAX-5)do begin webname[i]:=fname[i];
if fname[i]='.'then dotpos:=i;if fname[i]='/'then slashpos:=i;i:=i+1;
end;webname[i]:=' ';
if(dotpos=-1)or(dotpos<slashpos)then begin dotpos:=i;
webname[dotpos]:='.';webname[dotpos+1]:='w';webname[dotpos+2]:='e';
webname[dotpos+3]:='b';webname[dotpos+4]:=' ';end;
for i:=1 to dotpos do begin c:=webname[i];pascalfilename[i]:=c;
poolfilename[i]:=c;end;pascalfilename[dotpos+1]:='p';
pascalfilename[dotpos+2]:=' ';poolfilename[dotpos+1]:='p';
poolfilename[dotpos+2]:='o';poolfilename[dotpos+3]:='o';
poolfilename[dotpos+4]:='l';poolfilename[dotpos+5]:=' ';foundweb:=true;
end{:191}else if not foundchange then{192:}begin dotpos:=-1;
slashpos:=-1;i:=1;
while(fname[i]<>' ')and(i<=PATHMAX-5)do begin chgname[i]:=fname[i];
if fname[i]='.'then dotpos:=i;if fname[i]='/'then slashpos:=i;i:=i+1;
end;chgname[i]:=' ';
if(dotpos=-1)or(dotpos<slashpos)then begin dotpos:=i;
chgname[dotpos]:='.';chgname[dotpos+1]:='c';chgname[dotpos+2]:='h';
chgname[dotpos+3]:=' ';end;foundchange:=true;end{:192}else{195:}
begin writeln(stdout,'Usage: tangle webfile[.web] [changefile[.ch]].');
uexit(1);end{:195};end else{194:}begin{195:}
begin writeln(stdout,'Usage: tangle webfile[.web] [changefile[.ch]].');
uexit(1);end{:195};end{:194};end;if not foundweb then{195:}
begin writeln(stdout,'Usage: tangle webfile[.web] [changefile[.ch]].');
uexit(1);end{:195};if not foundchange then{193:}begin chgname[1]:='/';
chgname[2]:='d';chgname[3]:='e';chgname[4]:='v';chgname[5]:='/';
chgname[6]:='n';chgname[7]:='u';chgname[8]:='l';chgname[9]:='l';
chgname[10]:=' ';end{:193};end;{:190}procedure initialize;var{16:}
i:0..255;{:16}{41:}wi:0..2;{:41}{45:}zi:0..3;{:45}{51:}h:0..hashsize;
{:51}begin{10:}history:=0;{:10}{14:}xchr[32]:=' ';xchr[33]:='!';
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
xchr[0]:=' ';xchr[127]:=' ';{:14}{17:}for i:=1 to 31 do xchr[i]:=chr(i);
for i:=128 to 255 do xchr[i]:=chr(i);{:17}{18:}
for i:=0 to 255 do xord[chr(i)]:=32;for i:=1 to 255 do xord[xchr[i]]:=i;
xord[' ']:=32;{:18}{21:}{:21}{26:}scanargs;
rewrite(Pascalfile,pascalfilename);{:26}{42:}
for wi:=0 to 2 do begin bytestart[wi]:=0;byteptr[wi]:=0;end;
bytestart[3]:=0;nameptr:=1;stringptr:=256;poolchecksum:=271828;{:42}
{46:}for zi:=0 to 3 do begin tokstart[zi]:=0;tokptr[zi]:=0;end;
tokstart[4]:=0;textptr:=1;z:=1 mod 4;{:46}{48:}ilk[0]:=0;equiv[0]:=0;
{:48}{52:}for h:=0 to hashsize-1 do begin hash[h]:=0;chophash[h]:=0;end;
{:52}{71:}lastunnamed:=0;textlink[0]:=0;{:71}{144:}scanninghex:=false;
{:144}{152:}modtext[0]:=32;{:152}{180:}{troubleshooting:=true;
debugcycle:=1;debugskipped:=0;troubleshooting:=false;debugcycle:=99999;}
{:180}end;{:2}{24:}procedure openinput;begin reset(webfile,webname);
reset(changefile,chgname);end;{:24}{28:}
function inputln(var f:textfile):boolean;var finallimit:0..bufsize;
begin limit:=0;finallimit:=0;
if eof(f)then inputln:=false else begin while not eoln(f)do begin buffer
[limit]:=xord[getc(f)];limit:=limit+1;
if buffer[limit-1]<>32 then finallimit:=limit;
if limit=bufsize then begin while not eoln(f)do vgetc(f);limit:=limit-1;
if finallimit>limit then finallimit:=limit;begin writeln(stdout);
write(stdout,'! Input line too long');end;loc:=0;error;end;end;
readln(f);limit:=finallimit;inputln:=true;end;end;{:28}{49:}
procedure printid(p:namepointer);var k:0..maxbytes;w:0..2;
begin if p>=nameptr then write(stdout,'IMPOSSIBLE')else begin w:=p mod 3
;
for k:=bytestart[p]to bytestart[p+3]-1 do write(stdout,xchr[bytemem[w,k]
]);end;end;{:49}{53:}function idlookup(t:eightbits):namepointer;
label 31,32;var c:eightbits;i:0..bufsize;h:0..hashsize;k:0..maxbytes;
w:0..2;l:0..bufsize;p,q:namepointer;s:0..unambiglength;
begin l:=idloc-idfirst;{54:}h:=buffer[idfirst];i:=idfirst+1;
while i<idloc do begin h:=(h+h+buffer[i])mod hashsize;i:=i+1;end{:54};
{55:}p:=hash[h];
while p<>0 do begin if bytestart[p+3]-bytestart[p]=l then{56:}
begin i:=idfirst;k:=bytestart[p];w:=p mod 3;
while(i<idloc)and(buffer[i]=bytemem[w,k])do begin i:=i+1;k:=k+1;end;
if i=idloc then goto 31;end{:56};p:=link[p];end;p:=nameptr;
link[p]:=hash[h];hash[h]:=p;31:{:55};if(p=nameptr)or(t<>0)then{57:}
begin if((p<>nameptr)and(t<>0)and(ilk[p]=0))or((p=nameptr)and(t=0)and(
buffer[idfirst]<>34))then{58:}begin i:=idfirst;s:=0;h:=0;
while(i<idloc)and(s<unambiglength)do begin if buffer[i]<>95 then begin
if buffer[i]>=97 then choppedid[s]:=buffer[i]-32 else choppedid[s]:=
buffer[i];h:=(h+h+choppedid[s])mod hashsize;s:=s+1;end;i:=i+1;end;
choppedid[s]:=0;end{:58};if p<>nameptr then{59:}
begin if ilk[p]=0 then begin if t=1 then begin writeln(stdout);
write(stdout,'! This identifier has already appeared');error;end;{60:}
q:=chophash[h];
if q=p then chophash[h]:=equiv[p]else begin while equiv[q]<>p do q:=
equiv[q];equiv[q]:=equiv[p];end{:60};end else begin writeln(stdout);
write(stdout,'! This identifier was defined before');error;end;
ilk[p]:=t;end{:59}else{61:}
begin if(t=0)and(buffer[idfirst]<>34)then{62:}begin q:=chophash[h];
while q<>0 do begin{63:}begin k:=bytestart[q];s:=0;w:=q mod 3;
while(k<bytestart[q+3])and(s<unambiglength)do begin c:=bytemem[w,k];
if c<>95 then begin if choppedid[s]<>c then goto 32;s:=s+1;end;k:=k+1;
end;if(k=bytestart[q+3])and(choppedid[s]<>0)then goto 32;
begin writeln(stdout);write(stdout,'! Identifier conflict with ');end;
for k:=bytestart[q]to bytestart[q+3]-1 do write(stdout,xchr[bytemem[w,k]
]);error;q:=0;32:end{:63};q:=equiv[q];end;equiv[p]:=chophash[h];
chophash[h]:=p;end{:62};w:=nameptr mod 3;k:=byteptr[w];
if k+l>maxbytes then begin writeln(stdout);
write(stdout,'! Sorry, ','byte memory',' capacity exceeded');error;
history:=3;uexit(1);end;
if nameptr>maxnames-3 then begin writeln(stdout);
write(stdout,'! Sorry, ','name',' capacity exceeded');error;history:=3;
uexit(1);end;i:=idfirst;while i<idloc do begin bytemem[w,k]:=buffer[i];
k:=k+1;i:=i+1;end;byteptr[w]:=k;bytestart[nameptr+3]:=k;
nameptr:=nameptr+1;if buffer[idfirst]<>34 then ilk[p]:=t else{64:}
begin ilk[p]:=1;
if l-doublechars=2 then equiv[p]:=buffer[idfirst+1]+32768 else begin if
stringptr=256 then rewrite(pool,poolfilename);equiv[p]:=stringptr+32768;
l:=l-doublechars-1;if l>99 then begin writeln(stdout);
write(stdout,'! Preprocessed string is too long');error;end;
stringptr:=stringptr+1;write(pool,xchr[48+l div 10],xchr[48+l mod 10]);
poolchecksum:=poolchecksum+poolchecksum+l;
while poolchecksum>536870839 do poolchecksum:=poolchecksum-536870839;
i:=idfirst+1;while i<idloc do begin write(pool,xchr[buffer[i]]);
poolchecksum:=poolchecksum+poolchecksum+buffer[i];
while poolchecksum>536870839 do poolchecksum:=poolchecksum-536870839;
if(buffer[i]=34)or(buffer[i]=64)then i:=i+2 else i:=i+1;end;
writeln(pool);end;end{:64};end{:61};end{:57};idlookup:=p;end;{:53}{66:}
function modlookup(l:sixteenbits):namepointer;label 31;var c:0..4;
j:0..longestname;k:0..maxbytes;w:0..2;p:namepointer;q:namepointer;
begin c:=2;q:=0;p:=ilk[0];while p<>0 do begin{68:}begin k:=bytestart[p];
w:=p mod 3;c:=1;j:=1;
while(k<bytestart[p+3])and(j<=l)and(modtext[j]=bytemem[w,k])do begin k:=
k+1;j:=j+1;end;
if k=bytestart[p+3]then if j>l then c:=1 else c:=4 else if j>l then c:=3
else if modtext[j]<bytemem[w,k]then c:=0 else c:=2;end{:68};q:=p;
if c=0 then p:=link[q]else if c=2 then p:=ilk[q]else goto 31;end;{67:}
w:=nameptr mod 3;k:=byteptr[w];
if k+l>maxbytes then begin writeln(stdout);
write(stdout,'! Sorry, ','byte memory',' capacity exceeded');error;
history:=3;uexit(1);end;
if nameptr>maxnames-3 then begin writeln(stdout);
write(stdout,'! Sorry, ','name',' capacity exceeded');error;history:=3;
uexit(1);end;p:=nameptr;if c=0 then link[q]:=p else ilk[q]:=p;
link[p]:=0;ilk[p]:=0;c:=1;equiv[p]:=0;
for j:=1 to l do bytemem[w,k+j-1]:=modtext[j];byteptr[w]:=k+l;
bytestart[nameptr+3]:=k+l;nameptr:=nameptr+1;{:67};
31:if c<>1 then begin begin writeln(stdout);
write(stdout,'! Incompatible section names');error;end;p:=0;end;
modlookup:=p;end;{:66}{69:}
function prefixlookup(l:sixteenbits):namepointer;var c:0..4;
count:0..maxnames;j:0..longestname;k:0..maxbytes;w:0..2;p:namepointer;
q:namepointer;r:namepointer;begin q:=0;p:=ilk[0];count:=0;r:=0;
while p<>0 do begin{68:}begin k:=bytestart[p];w:=p mod 3;c:=1;j:=1;
while(k<bytestart[p+3])and(j<=l)and(modtext[j]=bytemem[w,k])do begin k:=
k+1;j:=j+1;end;
if k=bytestart[p+3]then if j>l then c:=1 else c:=4 else if j>l then c:=3
else if modtext[j]<bytemem[w,k]then c:=0 else c:=2;end{:68};
if c=0 then p:=link[p]else if c=2 then p:=ilk[p]else begin r:=p;
count:=count+1;q:=ilk[p];p:=link[p];end;if p=0 then begin p:=q;q:=0;end;
end;if count<>1 then if count=0 then begin writeln(stdout);
write(stdout,'! Name does not match');error;
end else begin writeln(stdout);write(stdout,'! Ambiguous prefix');error;
end;prefixlookup:=r;end;{:69}{73:}
procedure storetwobytes(x:sixteenbits);
begin if tokptr[z]+2>maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=x div 256;
tokmem[z,tokptr[z]+1]:=x mod 256;tokptr[z]:=tokptr[z]+2;end;{:73}{74:}
{procedure printrepl(p:textpointer);var k:0..maxtoks;a:sixteenbits;
zp:0..3;
begin if p>=textptr then write(stdout,'BAD')else begin k:=tokstart[p];
zp:=p mod 4;while k<tokstart[p+4]do begin a:=tokmem[zp,k];
if a>=128 then[75:]begin k:=k+1;
if a<168 then begin a:=(a-128)*256+tokmem[zp,k];printid(a);
if bytemem[a mod 3,bytestart[a]]=34 then write(stdout,'"')else write(
stdout,' ');end else if a<208 then begin write(stdout,'@<');
printid((a-168)*256+tokmem[zp,k]);write(stdout,'@>');
end else begin a:=(a-208)*256+tokmem[zp,k];
write(stdout,'@',xchr[123],a:1,'@',xchr[125]);end;
end[:75]else[76:]case a of 9:write(stdout,'@',xchr[123]);
10:write(stdout,'@',xchr[125]);12:write(stdout,'@''');
13:write(stdout,'@"');125:write(stdout,'@$');0:write(stdout,'#');
64:write(stdout,'@@');2:write(stdout,'@=');3:write(stdout,'@\');
others:write(stdout,xchr[a])end[:76];k:=k+1;end;end;end;}{:74}{84:}
procedure pushlevel(p:namepointer);
begin if stackptr=stacksize then begin writeln(stdout);
write(stdout,'! Sorry, ','stack',' capacity exceeded');error;history:=3;
uexit(1);end else begin stack[stackptr]:=curstate;stackptr:=stackptr+1;
curstate.namefield:=p;curstate.replfield:=equiv[p];
zo:=curstate.replfield mod 4;
curstate.bytefield:=tokstart[curstate.replfield];
curstate.endfield:=tokstart[curstate.replfield+4];curstate.modfield:=0;
end;end;{:84}{85:}procedure poplevel;label 10;
begin if textlink[curstate.replfield]=0 then begin if ilk[curstate.
namefield]=3 then{91:}begin nameptr:=nameptr-1;textptr:=textptr-1;
z:=textptr mod 4;{if tokptr[z]>maxtokptr[z]then maxtokptr[z]:=tokptr[z];
}tokptr[z]:=tokstart[textptr];
{byteptr[nameptr mod 3]:=byteptr[nameptr mod 3]-1;}end{:91};
end else if textlink[curstate.replfield]<maxtexts then begin curstate.
replfield:=textlink[curstate.replfield];zo:=curstate.replfield mod 4;
curstate.bytefield:=tokstart[curstate.replfield];
curstate.endfield:=tokstart[curstate.replfield+4];goto 10;end;
stackptr:=stackptr-1;if stackptr>0 then begin curstate:=stack[stackptr];
zo:=curstate.replfield mod 4;end;10:end;{:85}{87:}
function getoutput:sixteenbits;label 20,30,31;var a:sixteenbits;
b:eightbits;bal:sixteenbits;k:0..maxbytes;w:0..2;
begin 20:if stackptr=0 then begin a:=0;goto 31;end;
if curstate.bytefield=curstate.endfield then begin curval:=-curstate.
modfield;poplevel;if curval=0 then goto 20;a:=129;goto 31;end;
a:=tokmem[zo,curstate.bytefield];
curstate.bytefield:=curstate.bytefield+1;if a<128 then if a=0 then{92:}
begin pushlevel(nameptr-1);goto 20;end{:92}else goto 31;
a:=(a-128)*256+tokmem[zo,curstate.bytefield];
curstate.bytefield:=curstate.bytefield+1;if a<10240 then{89:}
begin case ilk[a]of 0:begin curval:=a;a:=130;end;
1:begin curval:=equiv[a]-32768;a:=128;end;2:begin pushlevel(a);goto 20;
end;3:begin{90:}
while(curstate.bytefield=curstate.endfield)and(stackptr>0)do poplevel;
if(stackptr=0)or(tokmem[zo,curstate.bytefield]<>40)then begin begin
writeln(stdout);write(stdout,'! No parameter given for ');end;
printid(a);error;goto 20;end;{93:}bal:=1;
curstate.bytefield:=curstate.bytefield+1;
while true do begin b:=tokmem[zo,curstate.bytefield];
curstate.bytefield:=curstate.bytefield+1;
if b=0 then storetwobytes(nameptr+32767)else begin if b>=128 then begin
begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=b;tokptr[z]:=tokptr[z]+1;end;
b:=tokmem[zo,curstate.bytefield];
curstate.bytefield:=curstate.bytefield+1;
end else case b of 40:bal:=bal+1;41:begin bal:=bal-1;
if bal=0 then goto 30;end;
39:repeat begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=b;tokptr[z]:=tokptr[z]+1;end;
b:=tokmem[zo,curstate.bytefield];
curstate.bytefield:=curstate.bytefield+1;until b=39;others:end;
begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=b;tokptr[z]:=tokptr[z]+1;end;end;end;
30:{:93};equiv[nameptr]:=textptr;ilk[nameptr]:=2;w:=nameptr mod 3;
k:=byteptr[w];{if k=maxbytes then begin writeln(stdout);
write(stdout,'! Sorry, ','byte memory',' capacity exceeded');error;
history:=3;uexit(1);end;bytemem[w,k]:=35;k:=k+1;byteptr[w]:=k;}
if nameptr>maxnames-3 then begin writeln(stdout);
write(stdout,'! Sorry, ','name',' capacity exceeded');error;history:=3;
uexit(1);end;bytestart[nameptr+3]:=k;nameptr:=nameptr+1;
if textptr>maxtexts-4 then begin writeln(stdout);
write(stdout,'! Sorry, ','text',' capacity exceeded');error;history:=3;
uexit(1);end;textlink[textptr]:=0;tokstart[textptr+4]:=tokptr[z];
textptr:=textptr+1;z:=textptr mod 4{:90};pushlevel(a);goto 20;end;
others:begin writeln(stdout);
write(stdout,'! This can''t happen (','output',')');error;history:=3;
uexit(1);end end;goto 31;end{:89};if a<20480 then{88:}begin a:=a-10240;
if equiv[a]<>0 then pushlevel(a)else if a<>0 then begin begin writeln(
stdout);write(stdout,'! Not present: <');end;printid(a);
write(stdout,'>');error;end;goto 20;end{:88};curval:=a-20480;a:=129;
curstate.modfield:=curval;31:{if troubleshooting then debughelp;}
getoutput:=a;end;{:87}{97:}procedure flushbuffer;var k:0..outbufsize;
b:0..outbufsize;begin b:=breakptr;
if(semiptr<>0)and(outptr-semiptr<=linelength)then breakptr:=semiptr;
for k:=1 to breakptr do write(Pascalfile,xchr[outbuf[k-1]]);
writeln(Pascalfile);line:=line+1;
if line mod 100=0 then begin write(stdout,'.');
if line mod 500=0 then write(stdout,line:1);flush(stdout);end;
if breakptr<outptr then begin if outbuf[breakptr]=32 then begin breakptr
:=breakptr+1;if breakptr>b then b:=breakptr;end;
for k:=breakptr to outptr-1 do outbuf[k-breakptr]:=outbuf[k];end;
outptr:=outptr-breakptr;breakptr:=b-breakptr;semiptr:=0;
if outptr>linelength then begin begin writeln(stdout);
write(stdout,'! Long line must be truncated');error;end;
outptr:=linelength;end;end;{:97}{99:}procedure appval(v:integer);
var k:0..outbufsize;begin k:=outbufsize;repeat outbuf[k]:=v mod 10;
v:=v div 10;k:=k-1;until v=0;repeat k:=k+1;
begin outbuf[outptr]:=outbuf[k]+48;outptr:=outptr+1;end;
until k=outbufsize;end;{:99}{101:}procedure sendout(t:eightbits;
v:sixteenbits);label 20;var k:0..linelength;begin{102:}
20:case outstate of 1:if t<>3 then begin breakptr:=outptr;
if t=2 then begin outbuf[outptr]:=32;outptr:=outptr+1;end;end;
2:begin begin outbuf[outptr]:=44-outapp;outptr:=outptr+1;end;
if outptr>linelength then flushbuffer;breakptr:=outptr;end;
3,4:begin{103:}
if(outval<0)or((outval=0)and(lastsign<0))then begin outbuf[outptr]:=45;
outptr:=outptr+1;
end else if outsign>0 then begin outbuf[outptr]:=outsign;
outptr:=outptr+1;end;appval(abs(outval));
if outptr>linelength then flushbuffer;{:103};outstate:=outstate-2;
goto 20;end;5:{104:}begin if(t=3)or({105:}
((t=2)and(v=3)and(((outcontrib[1]=68)and(outcontrib[2]=73)and(outcontrib
[3]=86))or((outcontrib[1]=100)and(outcontrib[2]=105)and(outcontrib[3]=
118))or((outcontrib[1]=77)and(outcontrib[2]=79)and(outcontrib[3]=68))or(
(outcontrib[1]=109)and(outcontrib[2]=111)and(outcontrib[3]=100))))or((t=
0)and((v=42)or(v=47))){:105})then begin{103:}
if(outval<0)or((outval=0)and(lastsign<0))then begin outbuf[outptr]:=45;
outptr:=outptr+1;
end else if outsign>0 then begin outbuf[outptr]:=outsign;
outptr:=outptr+1;end;appval(abs(outval));
if outptr>linelength then flushbuffer;{:103};outsign:=43;outval:=outapp;
end else outval:=outval+outapp;outstate:=3;goto 20;end{:104};
0:if t<>3 then breakptr:=outptr;others:end{:102};
if t<>0 then for k:=1 to v do begin outbuf[outptr]:=outcontrib[k];
outptr:=outptr+1;end else begin outbuf[outptr]:=v;outptr:=outptr+1;end;
if outptr>linelength then flushbuffer;
if(t=0)and((v=59)or(v=125))then begin semiptr:=outptr;breakptr:=outptr;
end;if t>=2 then outstate:=1 else outstate:=0 end;{:101}{106:}
procedure sendsign(v:integer);
begin case outstate of 2,4:outapp:=outapp*v;3:begin outapp:=v;
outstate:=4;end;5:begin outval:=outval+outapp;outapp:=v;outstate:=4;end;
others:begin breakptr:=outptr;outapp:=v;outstate:=2;end end;
lastsign:=outapp;end;{:106}{107:}procedure sendval(v:integer);
label 666,10;begin case outstate of 1:begin{110:}
if(outptr=breakptr+3)or((outptr=breakptr+4)and(outbuf[breakptr]=32))then
if((outbuf[outptr-3]=68)and(outbuf[outptr-2]=73)and(outbuf[outptr-1]=86)
)or((outbuf[outptr-3]=100)and(outbuf[outptr-2]=105)and(outbuf[outptr-1]=
118))or((outbuf[outptr-3]=77)and(outbuf[outptr-2]=79)and(outbuf[outptr-1
]=68))or((outbuf[outptr-3]=109)and(outbuf[outptr-2]=111)and(outbuf[
outptr-1]=100))then goto 666{:110};outsign:=32;outstate:=3;outval:=v;
breakptr:=outptr;lastsign:=+1;end;0:begin{109:}
if(outptr=breakptr+1)and((outbuf[breakptr]=42)or(outbuf[breakptr]=47))
then goto 666{:109};outsign:=0;outstate:=3;outval:=v;breakptr:=outptr;
lastsign:=+1;end;{108:}2:begin outsign:=43;outstate:=3;outval:=outapp*v;
end;3:begin outstate:=5;outapp:=v;begin writeln(stdout);
write(stdout,'! Two numbers occurred without a sign between them');
error;end;end;4:begin outstate:=5;outapp:=outapp*v;end;
5:begin outval:=outval+outapp;outapp:=v;begin writeln(stdout);
write(stdout,'! Two numbers occurred without a sign between them');
error;end;end;{:108}others:goto 666 end;goto 10;666:{111:}
if v>=0 then begin if outstate=1 then begin breakptr:=outptr;
begin outbuf[outptr]:=32;outptr:=outptr+1;end;end;appval(v);
if outptr>linelength then flushbuffer;outstate:=1;
end else begin begin outbuf[outptr]:=40;outptr:=outptr+1;end;
begin outbuf[outptr]:=45;outptr:=outptr+1;end;appval(-v);
begin outbuf[outptr]:=41;outptr:=outptr+1;end;
if outptr>linelength then flushbuffer;outstate:=0;end{:111};10:end;
{:107}{113:}procedure sendtheoutput;label 2,21,22;var curchar:eightbits;
k:0..linelength;j:0..maxbytes;w:0..2;n:integer;
begin while stackptr>0 do begin curchar:=getoutput;
21:case curchar of 0:;{116:}
65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,
89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,
114,115,116,117,118,119,120,121,122:begin outcontrib[1]:=curchar;
sendout(2,1);end;130:begin k:=0;j:=bytestart[curval];w:=curval mod 3;
while(k<maxidlength)and(j<bytestart[curval+3])do begin k:=k+1;
outcontrib[k]:=bytemem[w,j];j:=j+1;if outcontrib[k]=95 then k:=k-1;end;
sendout(2,k);end;{:116}{119:}48,49,50,51,52,53,54,55,56,57:begin n:=0;
repeat curchar:=curchar-48;if n>=214748364 then begin writeln(stdout);
write(stdout,'! Constant too big');error;end else n:=10*n+curchar;
curchar:=getoutput;until(curchar>57)or(curchar<48);sendval(n);k:=0;
if curchar=101 then curchar:=69;if curchar=69 then goto 2 else goto 21;
end;125:sendval(poolchecksum);12:begin n:=0;curchar:=48;
repeat curchar:=curchar-48;if n>=268435456 then begin writeln(stdout);
write(stdout,'! Constant too big');error;end else n:=8*n+curchar;
curchar:=getoutput;until(curchar>55)or(curchar<48);sendval(n);goto 21;
end;13:begin n:=0;curchar:=48;
repeat if curchar>=65 then curchar:=curchar-55 else curchar:=curchar-48;
if n>=134217728 then begin writeln(stdout);
write(stdout,'! Constant too big');error;end else n:=16*n+curchar;
curchar:=getoutput;
until(curchar>70)or(curchar<48)or((curchar>57)and(curchar<65));
sendval(n);goto 21;end;128:sendval(curval);46:begin k:=1;
outcontrib[1]:=46;curchar:=getoutput;
if curchar=46 then begin outcontrib[2]:=46;sendout(1,2);
end else if(curchar>=48)and(curchar<=57)then goto 2 else begin sendout(0
,46);goto 21;end;end;{:119}43,45:sendsign(44-curchar);{114:}
4:begin outcontrib[1]:=97;outcontrib[2]:=110;outcontrib[3]:=100;
sendout(2,3);end;5:begin outcontrib[1]:=110;outcontrib[2]:=111;
outcontrib[3]:=116;sendout(2,3);end;6:begin outcontrib[1]:=105;
outcontrib[2]:=110;sendout(2,2);end;31:begin outcontrib[1]:=111;
outcontrib[2]:=114;sendout(2,2);end;24:begin outcontrib[1]:=58;
outcontrib[2]:=61;sendout(1,2);end;26:begin outcontrib[1]:=60;
outcontrib[2]:=62;sendout(1,2);end;28:begin outcontrib[1]:=60;
outcontrib[2]:=61;sendout(1,2);end;29:begin outcontrib[1]:=62;
outcontrib[2]:=61;sendout(1,2);end;30:begin outcontrib[1]:=61;
outcontrib[2]:=61;sendout(1,2);end;32:begin outcontrib[1]:=46;
outcontrib[2]:=46;sendout(1,2);end;{:114}39:{117:}begin k:=1;
outcontrib[1]:=39;repeat if k<linelength then k:=k+1;
outcontrib[k]:=getoutput;until(outcontrib[k]=39)or(stackptr=0);
if k=linelength then begin writeln(stdout);
write(stdout,'! String too long');error;end;sendout(1,k);
curchar:=getoutput;if curchar=39 then outstate:=6;goto 21;end{:117};
{115:}
33,34,35,36,37,38,40,41,42,44,47,58,59,60,61,62,63,64,91,92,93,94,95,96,
123,124{:115}:sendout(0,curchar);{121:}
9:begin if bracelevel=0 then sendout(0,123)else sendout(0,91);
bracelevel:=bracelevel+1;end;
10:if bracelevel>0 then begin bracelevel:=bracelevel-1;
if bracelevel=0 then sendout(0,125)else sendout(0,93);
end else begin writeln(stdout);write(stdout,'! Extra @}');error;end;
129:begin if bracelevel=0 then sendout(0,123)else sendout(0,91);
if curval<0 then begin sendout(0,58);sendval(-curval);
end else begin sendval(curval);sendout(0,58);end;
if bracelevel=0 then sendout(0,125)else sendout(0,93);end;{:121}
127:begin sendout(3,0);outstate:=6;end;2:{118:}begin k:=0;
repeat if k<linelength then k:=k+1;outcontrib[k]:=getoutput;
until(outcontrib[k]=2)or(stackptr=0);
if k=linelength then begin writeln(stdout);
write(stdout,'! Verbatim string too long');error;end;sendout(1,k-1);
end{:118};3:{122:}begin sendout(1,0);
while outptr>0 do begin if outptr<=linelength then breakptr:=outptr;
flushbuffer;end;outstate:=0;end{:122};others:begin writeln(stdout);
write(stdout,'! Can''t output ASCII code ',curchar:1);error;end end;
goto 22;2:{120:}repeat if k<linelength then k:=k+1;
outcontrib[k]:=curchar;curchar:=getoutput;
if(outcontrib[k]=69)and((curchar=43)or(curchar=45))then begin if k<
linelength then k:=k+1;outcontrib[k]:=curchar;curchar:=getoutput;
end else if curchar=101 then curchar:=69;
until(curchar<>69)and((curchar<48)or(curchar>57));
if k=linelength then begin writeln(stdout);
write(stdout,'! Fraction too long');error;end;sendout(3,k);goto 21{:120}
;22:end;end;{:113}{127:}function linesdontmatch:boolean;label 10;
var k:0..bufsize;begin linesdontmatch:=true;
if changelimit<>limit then goto 10;
if limit>0 then for k:=0 to limit-1 do if changebuffer[k]<>buffer[k]then
goto 10;linesdontmatch:=false;10:end;{:127}{128:}
procedure primethechangebuffer;label 22,30,10;var k:0..bufsize;
begin changelimit:=0;{129:}while true do begin line:=line+1;
if not inputln(changefile)then goto 10;if limit<2 then goto 22;
if buffer[0]<>64 then goto 22;
if(buffer[1]>=88)and(buffer[1]<=90)then buffer[1]:=buffer[1]+32;
if buffer[1]=120 then goto 30;
if(buffer[1]=121)or(buffer[1]=122)then begin loc:=2;
begin writeln(stdout);write(stdout,'! Where is the matching @x?');error;
end;end;22:end;30:{:129};{130:}repeat line:=line+1;
if not inputln(changefile)then begin begin writeln(stdout);
write(stdout,'! Change file ended after @x');error;end;goto 10;end;
until limit>0;{:130};{131:}begin changelimit:=limit;
if limit>0 then for k:=0 to limit-1 do changebuffer[k]:=buffer[k];
end{:131};10:end;{:128}{132:}procedure checkchange;label 10;
var n:integer;k:0..bufsize;begin if linesdontmatch then goto 10;n:=0;
while true do begin changing:=not changing;templine:=otherline;
otherline:=line;line:=templine;line:=line+1;
if not inputln(changefile)then begin begin writeln(stdout);
write(stdout,'! Change file ended before @y');error;end;changelimit:=0;
changing:=not changing;templine:=otherline;otherline:=line;
line:=templine;goto 10;end;{133:}
if limit>1 then if buffer[0]=64 then begin if(buffer[1]>=88)and(buffer[1
]<=90)then buffer[1]:=buffer[1]+32;
if(buffer[1]=120)or(buffer[1]=122)then begin loc:=2;
begin writeln(stdout);write(stdout,'! Where is the matching @y?');error;
end;end else if buffer[1]=121 then begin if n>0 then begin loc:=2;
begin writeln(stdout);
write(stdout,'! Hmm... ',n:1,' of the preceding lines failed to match');
error;end;end;goto 10;end;end{:133};{131:}begin changelimit:=limit;
if limit>0 then for k:=0 to limit-1 do changebuffer[k]:=buffer[k];
end{:131};changing:=not changing;templine:=otherline;otherline:=line;
line:=templine;line:=line+1;
if not inputln(webfile)then begin begin writeln(stdout);
write(stdout,'! WEB file ended during a change');error;end;
inputhasended:=true;goto 10;end;if linesdontmatch then n:=n+1;end;
10:end;{:132}{135:}procedure getline;label 20;
begin 20:if changing then{137:}begin line:=line+1;
if not inputln(changefile)then begin begin writeln(stdout);
write(stdout,'! Change file ended without @z');error;end;buffer[0]:=64;
buffer[1]:=122;limit:=2;end;
if limit>1 then if buffer[0]=64 then begin if(buffer[1]>=88)and(buffer[1
]<=90)then buffer[1]:=buffer[1]+32;
if(buffer[1]=120)or(buffer[1]=121)then begin loc:=2;
begin writeln(stdout);write(stdout,'! Where is the matching @z?');error;
end;end else if buffer[1]=122 then begin primethechangebuffer;
changing:=not changing;templine:=otherline;otherline:=line;
line:=templine;end;end;end{:137};if not changing then begin{136:}
begin line:=line+1;
if not inputln(webfile)then inputhasended:=true else if limit=
changelimit then if buffer[0]=changebuffer[0]then if changelimit>0 then
checkchange;end{:136};if changing then goto 20;end;loc:=0;
buffer[limit]:=32;end;{:135}{139:}
function controlcode(c:ASCIIcode):eightbits;
begin case c of 64:controlcode:=64;39:controlcode:=12;
34:controlcode:=13;36:controlcode:=125;32,9:controlcode:=136;
42:begin write(stdout,'*',modulecount+1:1);flush(stdout);
controlcode:=136;end;68,100:controlcode:=133;70,102:controlcode:=132;
123:controlcode:=9;125:controlcode:=10;80,112:controlcode:=134;
84,116,94,46,58:controlcode:=131;38:controlcode:=127;
60:controlcode:=135;61:controlcode:=2;92:controlcode:=3;
others:controlcode:=0 end;end;{:139}{140:}function skipahead:eightbits;
label 30;var c:eightbits;
begin while true do begin if loc>limit then begin getline;
if inputhasended then begin c:=136;goto 30;end;end;buffer[limit+1]:=64;
while buffer[loc]<>64 do loc:=loc+1;if loc<=limit then begin loc:=loc+2;
c:=controlcode(buffer[loc-1]);if(c<>0)or(buffer[loc-1]=62)then goto 30;
end;end;30:skipahead:=c;end;{:140}{141:}procedure skipcomment;label 10;
var bal:eightbits;c:ASCIIcode;begin bal:=0;
while true do begin if loc>limit then begin getline;
if inputhasended then begin begin writeln(stdout);
write(stdout,'! Input ended in mid-comment');error;end;goto 10;end;end;
c:=buffer[loc];loc:=loc+1;{142:}if c=64 then begin c:=buffer[loc];
if(c<>32)and(c<>9)and(c<>42)and(c<>122)and(c<>90)then loc:=loc+1 else
begin begin writeln(stdout);
write(stdout,'! Section ended in mid-comment');error;end;loc:=loc-1;
goto 10;
end end else if(c=92)and(buffer[loc]<>64)then loc:=loc+1 else if c=123
then bal:=bal+1 else if c=125 then begin if bal=0 then goto 10;
bal:=bal-1;end{:142};end;10:end;{:141}{145:}function getnext:eightbits;
label 20,30,31;var c:eightbits;d:eightbits;j,k:0..longestname;
begin 20:if loc>limit then begin getline;
if inputhasended then begin c:=136;goto 31;end;end;c:=buffer[loc];
loc:=loc+1;if scanninghex then{146:}
if((c>=48)and(c<=57))or((c>=65)and(c<=70))then goto 31 else scanninghex
:=false{:146};
case c of 65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85
,86,87,88,89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111
,112,113,114,115,116,117,118,119,120,121,122:{148:}
begin if((c=101)or(c=69))and(loc>1)then if(buffer[loc-2]<=57)and(buffer[
loc-2]>=48)then c:=0;if c<>0 then begin loc:=loc-1;idfirst:=loc;
repeat loc:=loc+1;d:=buffer[loc];
until((d<48)or((d>57)and(d<65))or((d>90)and(d<97))or(d>122))and(d<>95);
if loc>idfirst+1 then begin c:=130;idloc:=loc;end;end else c:=69;
end{:148};34:{149:}begin doublechars:=0;idfirst:=loc-1;
repeat d:=buffer[loc];loc:=loc+1;
if(d=34)or(d=64)then if buffer[loc]=d then begin loc:=loc+1;d:=0;
doublechars:=doublechars+1;
end else begin if d=64 then begin writeln(stdout);
write(stdout,'! Double @ sign missing');error;
end end else if loc>limit then begin begin writeln(stdout);
write(stdout,'! String constant didn''t end');error;end;d:=34;end;
until d=34;idloc:=loc-1;c:=130;end{:149};64:{150:}
begin c:=controlcode(buffer[loc]);loc:=loc+1;
if c=0 then goto 20 else if c=13 then scanninghex:=true else if c=135
then{151:}begin{153:}k:=0;
while true do begin if loc>limit then begin getline;
if inputhasended then begin begin writeln(stdout);
write(stdout,'! Input ended in section name');error;end;goto 30;end;end;
d:=buffer[loc];{154:}if d=64 then begin d:=buffer[loc+1];
if d=62 then begin loc:=loc+2;goto 30;end;
if(d=32)or(d=9)or(d=42)then begin begin writeln(stdout);
write(stdout,'! Section name didn''t end');error;end;goto 30;end;k:=k+1;
modtext[k]:=64;loc:=loc+1;end{:154};loc:=loc+1;
if k<longestname-1 then k:=k+1;if(d=32)or(d=9)then begin d:=32;
if modtext[k-1]=32 then k:=k-1;end;modtext[k]:=d;end;30:{155:}
if k>=longestname-2 then begin begin writeln(stdout);
write(stdout,'! Section name too long: ');end;
for j:=1 to 25 do write(stdout,xchr[modtext[j]]);write(stdout,'...');
if history=0 then history:=1;end{:155};
if(modtext[k]=32)and(k>0)then k:=k-1;{:153};
if k>3 then begin if(modtext[k]=46)and(modtext[k-1]=46)and(modtext[k-2]=
46)then curmodule:=prefixlookup(k-3)else curmodule:=modlookup(k);
end else curmodule:=modlookup(k);end{:151}
else if c=131 then begin repeat c:=skipahead;until c<>64;
if buffer[loc-1]<>62 then begin writeln(stdout);
write(stdout,'! Improper @ within control text');error;end;goto 20;end;
end{:150};{147:}
46:if buffer[loc]=46 then begin if loc<=limit then begin c:=32;
loc:=loc+1;end;
end else if buffer[loc]=41 then begin if loc<=limit then begin c:=93;
loc:=loc+1;end;end;
58:if buffer[loc]=61 then begin if loc<=limit then begin c:=24;
loc:=loc+1;end;end;
61:if buffer[loc]=61 then begin if loc<=limit then begin c:=30;
loc:=loc+1;end;end;
62:if buffer[loc]=61 then begin if loc<=limit then begin c:=29;
loc:=loc+1;end;end;
60:if buffer[loc]=61 then begin if loc<=limit then begin c:=28;
loc:=loc+1;end;
end else if buffer[loc]=62 then begin if loc<=limit then begin c:=26;
loc:=loc+1;end;end;
40:if buffer[loc]=42 then begin if loc<=limit then begin c:=9;
loc:=loc+1;end;
end else if buffer[loc]=46 then begin if loc<=limit then begin c:=91;
loc:=loc+1;end;end;
42:if buffer[loc]=41 then begin if loc<=limit then begin c:=10;
loc:=loc+1;end;end;{:147}32,9:goto 20;123:begin skipcomment;goto 20;end;
125:begin begin writeln(stdout);write(stdout,'! Extra }');error;end;
goto 20;end;others:if c>=128 then goto 20 else end;
31:{if troubleshooting then debughelp;}getnext:=c;end;{:145}{157:}
procedure scannumeric(p:namepointer);label 21,30;
var accumulator:integer;nextsign:-1..+1;q:namepointer;val:integer;
begin{158:}accumulator:=0;nextsign:=+1;
while true do begin nextcontrol:=getnext;
21:case nextcontrol of 48,49,50,51,52,53,54,55,56,57:begin{160:}val:=0;
repeat val:=10*val+nextcontrol-48;nextcontrol:=getnext;
until(nextcontrol>57)or(nextcontrol<48){:160};
begin accumulator:=accumulator+nextsign*toint(val);nextsign:=+1;end;
goto 21;end;12:begin{161:}val:=0;nextcontrol:=48;
repeat val:=8*val+nextcontrol-48;nextcontrol:=getnext;
until(nextcontrol>55)or(nextcontrol<48){:161};
begin accumulator:=accumulator+nextsign*toint(val);nextsign:=+1;end;
goto 21;end;13:begin{162:}val:=0;nextcontrol:=48;
repeat if nextcontrol>=65 then nextcontrol:=nextcontrol-7;
val:=16*val+nextcontrol-48;nextcontrol:=getnext;
until(nextcontrol>70)or(nextcontrol<48)or((nextcontrol>57)and(
nextcontrol<65)){:162};
begin accumulator:=accumulator+nextsign*toint(val);nextsign:=+1;end;
goto 21;end;130:begin q:=idlookup(0);
if ilk[q]<>1 then begin nextcontrol:=42;goto 21;end;
begin accumulator:=accumulator+nextsign*toint(equiv[q]-32768);
nextsign:=+1;end;end;43:;45:nextsign:=-nextsign;
132,133,135,134,136:goto 30;59:begin writeln(stdout);
write(stdout,'! Omit semicolon in numeric definition');error;end;
others:{159:}begin begin writeln(stdout);
write(stdout,'! Improper numeric definition will be flushed');error;end;
repeat nextcontrol:=skipahead until(nextcontrol>=132);
if nextcontrol=135 then begin loc:=loc-2;nextcontrol:=getnext;end;
accumulator:=0;goto 30;end{:159}end;end;30:{:158};
if abs(accumulator)>=32768 then begin begin writeln(stdout);
write(stdout,'! Value too big: ',accumulator:1);error;end;
accumulator:=0;end;equiv[p]:=accumulator+32768;end;{:157}{165:}
procedure scanrepl(t:eightbits);label 22,30,31,21;var a:sixteenbits;
b:ASCIIcode;bal:eightbits;begin bal:=0;
while true do begin 22:a:=getnext;case a of 40:bal:=bal+1;
41:if bal=0 then begin writeln(stdout);write(stdout,'! Extra )');error;
end else bal:=bal-1;39:{168:}begin b:=39;
while true do begin begin if tokptr[z]=maxtoks then begin writeln(stdout
);write(stdout,'! Sorry, ','token',' capacity exceeded');error;
history:=3;uexit(1);end;tokmem[z,tokptr[z]]:=b;tokptr[z]:=tokptr[z]+1;
end;
if b=64 then if buffer[loc]=64 then loc:=loc+1 else begin writeln(stdout
);write(stdout,'! You should double @ signs in strings');error;end;
if loc=limit then begin begin writeln(stdout);
write(stdout,'! String didn''t end');error;end;buffer[loc]:=39;
buffer[loc+1]:=0;end;b:=buffer[loc];loc:=loc+1;
if b=39 then begin if buffer[loc]<>39 then goto 31 else begin loc:=loc+1
;begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=39;tokptr[z]:=tokptr[z]+1;end;end;end;
end;31:end{:168};35:if t=3 then a:=0;{167:}130:begin a:=idlookup(0);
begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=(a div 256)+128;
tokptr[z]:=tokptr[z]+1;end;a:=a mod 256;end;
135:if t<>135 then goto 30 else begin begin if tokptr[z]=maxtoks then
begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=(curmodule div 256)+168;
tokptr[z]:=tokptr[z]+1;end;a:=curmodule mod 256;end;2:{169:}
begin begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=2;tokptr[z]:=tokptr[z]+1;end;
buffer[limit+1]:=64;
21:if buffer[loc]=64 then begin if loc<limit then if buffer[loc+1]=64
then begin begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=64;tokptr[z]:=tokptr[z]+1;end;
loc:=loc+2;goto 21;end;
end else begin begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=buffer[loc];tokptr[z]:=tokptr[z]+1;
end;loc:=loc+1;goto 21;end;if loc>=limit then begin writeln(stdout);
write(stdout,'! Verbatim string didn''t end');error;
end else if buffer[loc+1]<>62 then begin writeln(stdout);
write(stdout,'! You should double @ signs in verbatim strings');error;
end;loc:=loc+2;end{:169};
133,132,134:if t<>135 then goto 30 else begin begin writeln(stdout);
write(stdout,'! @',xchr[buffer[loc-1]],' is ignored in Pascal text');
error;end;goto 22;end;136:goto 30;{:167}others:end;
begin if tokptr[z]=maxtoks then begin writeln(stdout);
write(stdout,'! Sorry, ','token',' capacity exceeded');error;history:=3;
uexit(1);end;tokmem[z,tokptr[z]]:=a;tokptr[z]:=tokptr[z]+1;end;end;
30:nextcontrol:=a;{166:}
if bal>0 then begin if bal=1 then begin writeln(stdout);
write(stdout,'! Missing )');error;end else begin writeln(stdout);
write(stdout,'! Missing ',bal:1,' )''s');error;end;
while bal>0 do begin begin if tokptr[z]=maxtoks then begin writeln(
stdout);write(stdout,'! Sorry, ','token',' capacity exceeded');error;
history:=3;uexit(1);end;tokmem[z,tokptr[z]]:=41;tokptr[z]:=tokptr[z]+1;
end;bal:=bal-1;end;end{:166};
if textptr>maxtexts-4 then begin writeln(stdout);
write(stdout,'! Sorry, ','text',' capacity exceeded');error;history:=3;
uexit(1);end;currepltext:=textptr;tokstart[textptr+4]:=tokptr[z];
textptr:=textptr+1;if z=3 then z:=0 else z:=z+1;end;{:165}{170:}
procedure definemacro(t:eightbits);var p:namepointer;
begin p:=idlookup(t);scanrepl(t);equiv[p]:=currepltext;
textlink[currepltext]:=0;end;{:170}{172:}procedure scanmodule;
label 22,30,10;var p:namepointer;begin modulecount:=modulecount+1;{173:}
nextcontrol:=0;
while true do begin 22:while nextcontrol<=132 do begin nextcontrol:=
skipahead;if nextcontrol=135 then begin loc:=loc-2;nextcontrol:=getnext;
end;end;if nextcontrol<>133 then goto 30;nextcontrol:=getnext;
if nextcontrol<>130 then begin begin writeln(stdout);
write(stdout,'! Definition flushed, must start with ',
'identifier of length > 1');error;end;goto 22;end;nextcontrol:=getnext;
if nextcontrol=61 then begin scannumeric(idlookup(1));goto 22;
end else if nextcontrol=30 then begin definemacro(2);goto 22;
end else{174:}if nextcontrol=40 then begin nextcontrol:=getnext;
if nextcontrol=35 then begin nextcontrol:=getnext;
if nextcontrol=41 then begin nextcontrol:=getnext;
if nextcontrol=61 then begin begin writeln(stdout);
write(stdout,'! Use == for macros');error;end;nextcontrol:=30;end;
if nextcontrol=30 then begin definemacro(3);goto 22;end;end;end;end;
{:174};begin writeln(stdout);
write(stdout,'! Definition flushed since it starts badly');error;end;
end;30:{:173};{175:}case nextcontrol of 134:p:=0;135:begin p:=curmodule;
{176:}repeat nextcontrol:=getnext;until nextcontrol<>43;
if(nextcontrol<>61)and(nextcontrol<>30)then begin begin writeln(stdout);
write(stdout,'! Pascal text flushed, = sign is missing');error;end;
repeat nextcontrol:=skipahead;until nextcontrol=136;goto 10;end{:176};
end;others:goto 10 end;{177:}storetwobytes(53248+modulecount);{:177};
scanrepl(135);{178:}
if p=0 then begin textlink[lastunnamed]:=currepltext;
lastunnamed:=currepltext;
end else if equiv[p]=0 then equiv[p]:=currepltext else begin p:=equiv[p]
;while textlink[p]<maxtexts do p:=textlink[p];textlink[p]:=currepltext;
end;textlink[currepltext]:=maxtexts;{:178};{:175};10:end;{:172}{181:}
{procedure debughelp;label 888,10;var k:integer;
begin debugskipped:=debugskipped+1;
if debugskipped<debugcycle then goto 10;debugskipped:=0;
while true do begin write(stdout,'#');flush(stdout);read(stdin,ddt);
if ddt<0 then goto 10 else if ddt=0 then begin goto 888;
888:ddt:=0;
end else begin read(stdin,dd);case ddt of 1:printid(dd);2:printrepl(dd);
3:for k:=1 to dd do write(stdout,xchr[buffer[k]]);
4:for k:=1 to dd do write(stdout,xchr[modtext[k]]);
5:for k:=1 to outptr do write(stdout,xchr[outbuf[k]]);
6:for k:=1 to dd do write(stdout,xchr[outcontrib[k]]);
others:write(stdout,'?')end;end;end;10:end;}{:181}{182:}
begin initialize;{134:}openinput;line:=0;otherline:=0;changing:=true;
primethechangebuffer;changing:=not changing;templine:=otherline;
otherline:=line;line:=templine;limit:=0;loc:=1;buffer[0]:=32;
inputhasended:=false;{:134};
writeln(stdout,'This is TANGLE, C Version 4.3');{183:}phaseone:=true;
modulecount:=0;repeat nextcontrol:=skipahead;until nextcontrol=136;
while not inputhasended do scanmodule;{138:}
if changelimit<>0 then begin for ii:=0 to changelimit do buffer[ii]:=
changebuffer[ii];limit:=changelimit;changing:=true;line:=otherline;
loc:=changelimit;begin writeln(stdout);
write(stdout,'! Change file entry did not match');error;end;end{:138};
phaseone:=false;{:183};{for ii:=0 to 3 do maxtokptr[ii]:=tokptr[ii];}
{112:}if textlink[0]=0 then begin begin writeln(stdout);
write(stdout,'! No output was specified.');end;
if history=0 then history:=1;end else begin begin writeln(stdout);
write(stdout,'Writing the output file');end;flush(stdout);{83:}
stackptr:=1;bracelevel:=0;curstate.namefield:=0;
curstate.replfield:=textlink[0];zo:=curstate.replfield mod 4;
curstate.bytefield:=tokstart[curstate.replfield];
curstate.endfield:=tokstart[curstate.replfield+4];curstate.modfield:=0;
{:83};{96:}outstate:=0;outptr:=0;breakptr:=0;semiptr:=0;outbuf[0]:=0;
line:=1;{:96};sendtheoutput;{98:}breakptr:=outptr;semiptr:=0;
flushbuffer;if bracelevel<>0 then begin writeln(stdout);
write(stdout,'! Program ended at brace level ',bracelevel:1);error;end;
{:98};begin writeln(stdout);write(stdout,'Done.');end;end{:112};
9999:if stringptr>256 then{184:}begin begin writeln(stdout);
write(stdout,stringptr-256:1,' strings written to string pool file.');
end;write(pool,'*');
for ii:=1 to 9 do begin outbuf[ii]:=poolchecksum mod 10;
poolchecksum:=poolchecksum div 10;end;
for ii:=9 downto 1 do write(pool,xchr[48+outbuf[ii]]);writeln(pool);
end{:184};{[186:]begin writeln(stdout);
write(stdout,'Memory usage statistics:');end;begin writeln(stdout);
write(stdout,nameptr:1,' names, ',textptr:1,' replacement texts;');end;
begin writeln(stdout);write(stdout,byteptr[0]:1);end;
for wo:=1 to 2 do write(stdout,'+',byteptr[wo]:1);
if phaseone then for ii:=0 to 3 do maxtokptr[ii]:=tokptr[ii];
write(stdout,' bytes, ',maxtokptr[0]:1);
for ii:=1 to 3 do write(stdout,'+',maxtokptr[ii]:1);
write(stdout,' tokens.');[:186];}{187:}
case history of 0:begin writeln(stdout);
write(stdout,'(No errors were found.)');end;1:begin writeln(stdout);
write(stdout,'(Did you see the warning message above?)');end;
2:begin writeln(stdout);
write(stdout,'(Pardon me, but I think I spotted something wrong.)');end;
3:begin writeln(stdout);
write(stdout,'(That was a fatal error, my friend.)');end;end{:187};
writeln(stdout);if(history<>0)and(history<>1)then uexit(1)else uexit(0);
end.{:182}
