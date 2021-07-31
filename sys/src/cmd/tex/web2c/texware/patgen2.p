{3:}{11:}{:11}program PATGEN;label 9999;const{27:}triesize=55000;
triecsize=26000;maxops=4080;maxval=10;maxdot=15;maxlen=50;maxbuflen=80;
{:27}type{12:}ASCIIcode=0..255;textchar=ASCIIcode;textfile=text;{:12}
{13:}packedASCIIcode=0..255;{:13}{20:}internalcode=ASCIIcode;
packedinternalcode=packedASCIIcode;{:20}{22:}classtype=0..5;digit=0..9;
hyftype=0..3;{:22}{29:}qindex=1..255;valtype=0..maxval;
dottype=0..maxdot;optype=0..maxops;wordindex=0..maxlen;
triepointer=0..triesize;triecpointer=0..triecsize;
opword=packed record dot:dottype;val:valtype;op:optype end;{:29}var{4:}
patstart,patfinish:dottype;hyphstart,hyphfinish:valtype;
goodwt,badwt,thresh:integer;{:4}{16:}xord:array[textchar]of ASCIIcode;
xchr:array[ASCIIcode]of textchar;{:16}{23:}
xclass:array[textchar]of classtype;xint:array[textchar]of internalcode;
xdig:array[0..9]of textchar;xext:array[internalcode]of textchar;
xhyf:array[1..3]of textchar;{:23}{25:}cmax:internalcode;{:25}{30:}
triec:packed array[triepointer]of packedinternalcode;
triel,trier:packed array[triepointer]of triepointer;
trietaken:packed array[triepointer]of boolean;
triecc:packed array[triecpointer]of packedinternalcode;
triecl,triecr:packed array[triecpointer]of triecpointer;
triectaken:packed array[triecpointer]of boolean;
ops:array[optype]of opword;{:30}{31:}
trieqc:array[qindex]of internalcode;
trieql,trieqr:array[qindex]of triepointer;qmax:qindex;qmaxthresh:qindex;
{:31}{33:}triemax:triepointer;triebmax:triepointer;
triecount:triepointer;opcount:optype;{:33}{40:}
pat:array[dottype]of internalcode;patlen:dottype;{:40}{43:}
triecmax,triecbmax,trieccount:triecpointer;trieckmax:triecpointer;
patcount:integer;{:43}{51:}
dictionary,patterns,translate,patout,pattmp:textfile;
fname:packed array[1..PATHMAX]of char;{:51}{52:}
buf:array[1..maxbuflen]of textchar;bufptr:0..maxbuflen;{:52}{55:}
imax:internalcode;lefthyphenmin,righthyphenmin:dottype;{:55}{66:}
goodpatcount,badpatcount:integer;goodcount,badcount,misscount:integer;
levelpatterncount:integer;moretocome:boolean;{:66}{74:}
word:array[wordindex]of internalcode;dots:array[wordindex]of hyftype;
dotw:array[wordindex]of digit;hval:array[wordindex]of valtype;
nomore:array[wordindex]of boolean;wlen:wordindex;wordwt:digit;
wtchg:boolean;{:74}{78:}hyfmin,hyfmax,hyflen:wordindex;{:78}{84:}
gooddot,baddot:hyftype;dotmin,dotmax,dotlen:wordindex;{:84}{87:}
procesp,hyphp:boolean;patdot:dottype;hyphlevel:valtype;
filnam:packed array[1..8]of char;{:87}{91:}maxpat:valtype;{:91}{95:}
n1,n2:integer;i:valtype;j:dottype;k:dottype;dot1:dottype;
morethislevel:array[dottype]of boolean;{:95}procedure initialize;
var{15:}bad:integer;i:textchar;j:ASCIIcode;{:15}begin{98:}
if argc<4 then begin begin writeln(stdout,
'Usage: patgen <dictionary file> <pattern file> <output file>',
' <translate file>');uexit(1);end;end;{:98}
writeln(stdout,'This is PATGEN, C Version 2.0');{14:}bad:=0;
if 255<127 then bad:=1;if(0<>0)or(0<>0)then bad:=2;{28:}
if(triecsize<4096)or(triesize<triecsize)then bad:=3;
if maxops>triesize then bad:=4;if maxval>10 then bad:=5;
if maxbuflen<maxlen then bad:=6;{:28}
if bad>0 then begin writeln(stdout,'Bad constants---case ',bad:1);
uexit(1);end;;{:14}{17:}for j:=0 to 255 do xchr[j]:=' ';xchr[46]:='.';
xchr[48]:='0';xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';xchr[52]:='4';
xchr[53]:='5';xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';xchr[57]:='9';
xchr[65]:='A';xchr[66]:='B';xchr[67]:='C';xchr[68]:='D';xchr[69]:='E';
xchr[70]:='F';xchr[71]:='G';xchr[72]:='H';xchr[73]:='I';xchr[74]:='J';
xchr[75]:='K';xchr[76]:='L';xchr[77]:='M';xchr[78]:='N';xchr[79]:='O';
xchr[80]:='P';xchr[81]:='Q';xchr[82]:='R';xchr[83]:='S';xchr[84]:='T';
xchr[85]:='U';xchr[86]:='V';xchr[87]:='W';xchr[88]:='X';xchr[89]:='Y';
xchr[90]:='Z';xchr[97]:='a';xchr[98]:='b';xchr[99]:='c';xchr[100]:='d';
xchr[101]:='e';xchr[102]:='f';xchr[103]:='g';xchr[104]:='h';
xchr[105]:='i';xchr[106]:='j';xchr[107]:='k';xchr[108]:='l';
xchr[109]:='m';xchr[110]:='n';xchr[111]:='o';xchr[112]:='p';
xchr[113]:='q';xchr[114]:='r';xchr[115]:='s';xchr[116]:='t';
xchr[117]:='u';xchr[118]:='v';xchr[119]:='w';xchr[120]:='x';
xchr[121]:='y';xchr[122]:='z';{:17}{18:}
for i:=chr(0)to chr(255)do xord[i]:=0;
for j:=0 to 255 do xord[xchr[j]]:=j;xord[' ']:=32;xord[chr(9)]:=32;{:18}
{24:}for i:=chr(0)to chr(255)do begin xclass[i]:=5;xint[i]:=0;end;
xclass[' ']:=0;for j:=0 to 255 do xext[j]:=' ';xext[1]:='.';
for j:=0 to 9 do begin xdig[j]:=xchr[j+48];xclass[xdig[j]]:=1;
xint[xdig[j]]:=j;end;xhyf[1]:='.';xhyf[2]:='-';xhyf[3]:='*';{:24}end;
{:3}{19:}function getASCII(c:textchar):ASCIIcode;label 40;
var i:ASCIIcode;begin i:=xord[c];
if i=0 then begin while i<255 do begin i:=i+1;
if(xchr[i]=' ')and(i<>32)then goto 40;end;
begin writeln(stdout,'PATGEN capacity exceeded, sorry [',256:1,
' characters','].');uexit(1);end;;40:xord[c]:=i;xchr[i]:=c;end;
getASCII:=i;end;{:19}{34:}procedure initpatterntrie;var c:internalcode;
h:optype;begin for c:=0 to 255 do begin triec[1+c]:=c;triel[1+c]:=0;
trier[1+c]:=0;trietaken[1+c]:=false;end;trietaken[1]:=true;triebmax:=1;
triemax:=256;triecount:=256;qmaxthresh:=5;triel[0]:=triemax+1;
trier[triemax+1]:=0;for h:=1 to maxops do ops[h].val:=0;opcount:=0;end;
{:34}{35:}function firstfit:triepointer;label 40,41;var s,t:triepointer;
q:qindex;begin{36:}if qmax>qmaxthresh then t:=trier[triemax+1]else t:=0;
while true do begin t:=triel[t];s:=t-trieqc[1];{37:}
if s>triesize-256 then begin writeln(stdout,
'PATGEN capacity exceeded, sorry [',triesize:1,' pattern trie nodes',
'].');uexit(1);end;;while triebmax<s do begin triebmax:=triebmax+1;
trietaken[triebmax]:=false;triec[triebmax+255]:=0;
triel[triebmax+255]:=triebmax+256;trier[triebmax+256]:=triebmax+255;
end{:37};if trietaken[s]then goto 41;
for q:=qmax downto 2 do if triec[s+trieqc[q]]<>0 then goto 41;goto 40;
41:end;40:{:36};for q:=1 to qmax do begin t:=s+trieqc[q];
triel[trier[t]]:=triel[t];trier[triel[t]]:=trier[t];triec[t]:=trieqc[q];
triel[t]:=trieql[q];trier[t]:=trieqr[q];if t>triemax then triemax:=t;
end;trietaken[s]:=true;firstfit:=s end;{:35}{38:}
procedure unpack(s:triepointer);var c:internalcode;t:triepointer;
begin qmax:=1;for c:=1 to cmax do begin t:=s+c;
if triec[t]=c then begin trieqc[qmax]:=c;trieql[qmax]:=triel[t];
trieqr[qmax]:=trier[t];qmax:=qmax+1;trier[triel[0]]:=t;
triel[t]:=triel[0];triel[0]:=t;trier[t]:=0;triec[t]:=0;end;end;
trietaken[s]:=false;end;{:38}{39:}function newtrieop(v:valtype;
d:dottype;n:optype):optype;label 10;var h:optype;
begin h:=((n+313*d+361*v)mod maxops)+1;
while true do begin if ops[h].val=0 then begin opcount:=opcount+1;
if opcount=maxops then begin writeln(stdout,
'PATGEN capacity exceeded, sorry [',maxops:1,' outputs','].');uexit(1);
end;;ops[h].val:=v;ops[h].dot:=d;ops[h].op:=n;newtrieop:=h;goto 10;end;
if(ops[h].val=v)and(ops[h].dot=d)and(ops[h].op=n)then begin newtrieop:=h
;goto 10;end;if h>1 then h:=h-1 else h:=maxops;end;10:end;{:39}{41:}
procedure insertpattern(val:valtype;dot:dottype);var i:dottype;
s,t:triepointer;begin i:=1;s:=1+pat[i];t:=triel[s];
while(t>0)and(i<patlen)do begin i:=i+1;t:=t+pat[i];
if triec[t]<>pat[i]then{42:}
begin if triec[t]=0 then begin triel[trier[t]]:=triel[t];
trier[triel[t]]:=trier[t];triec[t]:=pat[i];triel[t]:=0;trier[t]:=0;
if t>triemax then triemax:=t;end else begin unpack(t-pat[i]);
trieqc[qmax]:=pat[i];trieql[qmax]:=0;trieqr[qmax]:=0;t:=firstfit;
triel[s]:=t;t:=t+pat[i];end;triecount:=triecount+1;end{:42};s:=t;
t:=triel[s];end;trieql[1]:=0;trieqr[1]:=0;qmax:=1;
while i<patlen do begin i:=i+1;trieqc[1]:=pat[i];t:=firstfit;
triel[s]:=t;s:=t+pat[i];triecount:=triecount+1;end;
trier[s]:=newtrieop(val,dot,trier[s]);end;{:41}{44:}
procedure initcounttrie;var c:internalcode;
begin for c:=0 to 255 do begin triecc[1+c]:=c;triecl[1+c]:=0;
triecr[1+c]:=0;triectaken[1+c]:=false;end;triectaken[1]:=true;
triecbmax:=1;triecmax:=256;trieccount:=256;trieckmax:=4096;
triecl[0]:=triecmax+1;triecr[triecmax+1]:=0;patcount:=0;end;{:44}{45:}
function firstcfit:triecpointer;label 40,41;var a,b:triecpointer;
q:qindex;begin{46:}if qmax>3 then a:=triecr[triecmax+1]else a:=0;
while true do begin a:=triecl[a];b:=a-trieqc[1];{47:}
if b>trieckmax-256 then begin if trieckmax=triecsize then begin writeln(
stdout,'PATGEN capacity exceeded, sorry [',triecsize:1,
' count trie nodes','].');uexit(1);end;;
write(stdout,trieckmax div 1024:1,'K ');
if trieckmax>triecsize-4096 then trieckmax:=triecsize else trieckmax:=
trieckmax+4096;end;while triecbmax<b do begin triecbmax:=triecbmax+1;
triectaken[triecbmax]:=false;triecc[triecbmax+255]:=0;
triecl[triecbmax+255]:=triecbmax+256;
triecr[triecbmax+256]:=triecbmax+255;end{:47};
if triectaken[b]then goto 41;
for q:=qmax downto 2 do if triecc[b+trieqc[q]]<>0 then goto 41;goto 40;
41:end;40:{:46};for q:=1 to qmax do begin a:=b+trieqc[q];
triecl[triecr[a]]:=triecl[a];triecr[triecl[a]]:=triecr[a];
triecc[a]:=trieqc[q];triecl[a]:=trieql[q];triecr[a]:=trieqr[q];
if a>triecmax then triecmax:=a;end;triectaken[b]:=true;firstcfit:=b end;
{:45}{48:}procedure unpackc(b:triecpointer);var c:internalcode;
a:triecpointer;begin qmax:=1;for c:=1 to cmax do begin a:=b+c;
if triecc[a]=c then begin trieqc[qmax]:=c;trieql[qmax]:=triecl[a];
trieqr[qmax]:=triecr[a];qmax:=qmax+1;triecr[triecl[0]]:=a;
triecl[a]:=triecl[0];triecl[0]:=a;triecr[a]:=0;triecc[a]:=0;end;end;
triectaken[b]:=false;end;{:48}{49:}
function insertcpat(fpos:wordindex):triecpointer;var spos:wordindex;
a,b:triecpointer;begin spos:=fpos-patlen;spos:=spos+1;b:=1+word[spos];
a:=triecl[b];while(a>0)and(spos<fpos)do begin spos:=spos+1;
a:=a+word[spos];if triecc[a]<>word[spos]then{50:}
begin if triecc[a]=0 then begin triecl[triecr[a]]:=triecl[a];
triecr[triecl[a]]:=triecr[a];triecc[a]:=word[spos];triecl[a]:=0;
triecr[a]:=0;if a>triecmax then triecmax:=a;
end else begin unpackc(a-word[spos]);trieqc[qmax]:=word[spos];
trieql[qmax]:=0;trieqr[qmax]:=0;a:=firstcfit;triecl[b]:=a;
a:=a+word[spos];end;trieccount:=trieccount+1;end{:50};b:=a;a:=triecl[a];
end;trieql[1]:=0;trieqr[1]:=0;qmax:=1;
while spos<fpos do begin spos:=spos+1;trieqc[1]:=word[spos];
a:=firstcfit;triecl[b]:=a;b:=a+word[spos];trieccount:=trieccount+1;end;
insertcpat:=b;patcount:=patcount+1;end;{:49}{54:}
procedure readtranslate;label 30;var c:textchar;n:integer;j:ASCIIcode;
bad:boolean;lower:boolean;i:dottype;s,t:triepointer;begin imax:=1;
argv(4,fname);reset(translate,fname);if eof(translate)then{56:}
begin lefthyphenmin:=2;righthyphenmin:=3;
for j:=65 to 90 do begin imax:=imax+1;c:=xchr[j+32];xclass[c]:=3;
xint[c]:=imax;xext[imax]:=c;c:=xchr[j];xclass[c]:=3;xint[c]:=imax;end;
end{:56}else begin begin bufptr:=0;
while not eoln(translate)do begin if(bufptr>=maxbuflen)then begin begin
bufptr:=0;repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Line too long');uexit(1);end;;end;
bufptr:=bufptr+1;read(translate,buf[bufptr]);end;readln(translate);
while bufptr<maxbuflen do begin bufptr:=bufptr+1;buf[bufptr]:=' ';end;
end;{57:}bad:=false;
if buf[1]=' 'then n:=0 else if xclass[buf[1]]=1 then n:=xint[buf[1]]else
bad:=true;if xclass[buf[2]]=1 then n:=10*n+xint[buf[2]]else bad:=true;
if(n>0)and(n<maxdot)then lefthyphenmin:=n else bad:=true;
if buf[3]=' 'then n:=0 else if xclass[buf[3]]=1 then n:=xint[buf[3]]else
bad:=true;if xclass[buf[4]]=1 then n:=10*n+xint[buf[4]]else bad:=true;
if(n>0)and(n<maxdot)then righthyphenmin:=n else bad:=true;
for j:=1 to 3 do begin if buf[j+4]<>' 'then xhyf[j]:=buf[j+4];
if xclass[xhyf[j]]=5 then xclass[xhyf[j]]:=2 else bad:=true;end;
xclass['.']:=2;if bad then begin begin bufptr:=0;
repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Bad hyphenation data');uexit(1);end;;end{:57};
cmax:=254;while not eof(translate)do{58:}begin begin bufptr:=0;
while not eoln(translate)do begin if(bufptr>=maxbuflen)then begin begin
bufptr:=0;repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Line too long');uexit(1);end;;end;
bufptr:=bufptr+1;read(translate,buf[bufptr]);end;readln(translate);
while bufptr<maxbuflen do begin bufptr:=bufptr+1;buf[bufptr]:=' ';end;
end;bufptr:=1;lower:=true;while not bad do begin patlen:=0;
repeat if bufptr<maxbuflen then bufptr:=bufptr+1 else bad:=true;
if buf[bufptr]=buf[1]then if patlen=0 then goto 30 else begin if lower
then begin if imax=255 then begin begin bufptr:=0;
repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'PATGEN capacity exceeded, sorry [',256:1,
' letters','].');uexit(1);end;;end;imax:=imax+1;
xext[imax]:=xchr[pat[patlen]];end;c:=xchr[pat[1]];
if patlen=1 then begin if xclass[c]<>5 then bad:=true;xclass[c]:=3;
xint[c]:=imax;end else{59:}begin if xclass[c]=5 then xclass[c]:=4;
if xclass[c]<>4 then bad:=true;i:=0;s:=1;t:=triel[s];
while(t>1)and(i<patlen)do begin i:=i+1;t:=t+pat[i];
if triec[t]<>pat[i]then{42:}
begin if triec[t]=0 then begin triel[trier[t]]:=triel[t];
trier[triel[t]]:=trier[t];triec[t]:=pat[i];triel[t]:=0;trier[t]:=0;
if t>triemax then triemax:=t;end else begin unpack(t-pat[i]);
trieqc[qmax]:=pat[i];trieql[qmax]:=0;trieqr[qmax]:=0;t:=firstfit;
triel[s]:=t;t:=t+pat[i];end;triecount:=triecount+1;end{:42}
else if trier[t]>0 then bad:=true;s:=t;t:=triel[s];end;
if t>1 then bad:=true;trieql[1]:=0;trieqr[1]:=0;qmax:=1;
while i<patlen do begin i:=i+1;trieqc[1]:=pat[i];t:=firstfit;
triel[s]:=t;s:=t+pat[i];triecount:=triecount+1;end;trier[s]:=imax;
if not lower then triel[s]:=1;end{:59};
end else if patlen=maxdot then bad:=true else begin patlen:=patlen+1;
pat[patlen]:=getASCII(buf[bufptr]);end;until(buf[bufptr]=buf[1])or bad;
lower:=false;end;30:if bad then begin begin bufptr:=0;
repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Bad representation');uexit(1);end;;end;end{:58};
end;;writeln(stdout,'left_hyphen_min = ',lefthyphenmin:1,
', right_hyphen_min = ',righthyphenmin:1,', ',imax-1:1,' letters');
cmax:=imax;end;{:54}{61:}procedure findletters(b:triepointer;i:dottype);
var c:ASCIIcode;a:triepointer;j:dottype;l:triecpointer;
begin if i=1 then initcounttrie;for c:=1 to 255 do begin a:=b+c;
if triec[a]=c then begin pat[i]:=c;
if trier[a]=0 then findletters(triel[a],i+1)else if triel[a]=0 then{62:}
begin l:=1+trier[a];
for j:=1 to i-1 do begin if triecmax=triecsize then begin writeln(stdout
,'PATGEN capacity exceeded, sorry [',triecsize:1,' count trie nodes',
'].');uexit(1);end;;triecmax:=triecmax+1;triecl[l]:=triecmax;
l:=triecmax;triecc[l]:=pat[j];end;triecl[l]:=0;end{:62};end;end;end;
{:61}{64:}procedure traversecounttrie(b:triecpointer;i:dottype);
var c:internalcode;a:triecpointer;
begin for c:=1 to cmax do begin a:=b+c;
if triecc[a]=c then begin pat[i]:=c;
if i<patlen then traversecounttrie(triecl[a],i+1)else{65:}
if goodwt*triecl[a]<thresh then begin insertpattern(maxval,patdot);
badpatcount:=badpatcount+1 end else if goodwt*triecl[a]-badwt*triecr[a]
>=thresh then begin insertpattern(hyphlevel,patdot);
goodpatcount:=goodpatcount+1;goodcount:=goodcount+triecl[a];
badcount:=badcount+triecr[a];end else moretocome:=true{:65};end;end;end;
{:64}{67:}procedure collectcounttrie;begin goodpatcount:=0;
badpatcount:=0;goodcount:=0;badcount:=0;moretocome:=false;
traversecounttrie(1,1);
write(stdout,goodpatcount:1,' good and ',badpatcount:1,
' bad patterns added');
levelpatterncount:=levelpatterncount+goodpatcount;
if moretocome then writeln(stdout,' (more to come)')else writeln(stdout,
' ');write(stdout,'finding ',goodcount:1,' good and ',badcount:1,
' bad hyphens');
if goodpatcount>0 then begin write(stdout,', efficiency = ');
printreal(goodcount/(goodpatcount+badcount/(thresh/goodwt)),1,2);
writeln(stdout);end else writeln(stdout,' ');
writeln(stdout,'pattern trie has ',triecount:1,' nodes, ','trie_max = ',
triemax:1,', ',opcount:1,' outputs');end;{:67}{68:}
function deletepatterns(s:triepointer):triepointer;var c:internalcode;
t:triepointer;allfreed:boolean;h,n:optype;begin allfreed:=true;
for c:=1 to cmax do begin t:=s+c;if triec[t]=c then begin{69:}
begin h:=0;ops[0].op:=trier[t];n:=ops[0].op;
while n>0 do begin if ops[n].val=maxval then ops[h].op:=ops[n].op else h
:=n;n:=ops[h].op;end;trier[t]:=ops[0].op;end{:69};
if triel[t]>0 then triel[t]:=deletepatterns(triel[t]);
if(triel[t]>0)or(trier[t]>0)or(s=1)then allfreed:=false else{70:}
begin triel[trier[triemax+1]]:=t;trier[t]:=trier[triemax+1];
triel[t]:=triemax+1;trier[triemax+1]:=t;triec[t]:=0;
triecount:=triecount-1;end{:70};end;end;
if allfreed then begin trietaken[s]:=false;s:=0;end;deletepatterns:=s;
end;{:68}{71:}procedure deletebadpatterns;var oldopcount:optype;
oldtriecount:triepointer;t:triepointer;h:optype;
begin oldopcount:=opcount;oldtriecount:=triecount;t:=deletepatterns(1);
for h:=1 to maxops do if ops[h].val=maxval then begin ops[h].val:=0;
opcount:=opcount-1;end;
writeln(stdout,oldtriecount-triecount:1,' nodes and ',oldopcount-opcount
:1,' outputs deleted');qmaxthresh:=7;end;{:71}{72:}
procedure outputpatterns(s:triepointer;patlen:dottype);
var c:internalcode;t:triepointer;h:optype;d:dottype;l:triecpointer;
begin for c:=1 to cmax do begin t:=s+c;
if triec[t]=c then begin pat[patlen]:=c;h:=trier[t];if h>0 then{73:}
begin for d:=0 to patlen do hval[d]:=0;repeat d:=ops[h].dot;
if hval[d]<ops[h].val then hval[d]:=ops[h].val;h:=ops[h].op;until h=0;
if hval[0]>0 then write(patout,xdig[hval[0]]);
for d:=1 to patlen do begin l:=triecl[1+pat[d]];
while l>0 do begin write(patout,xchr[triecc[l]]);l:=triecl[l];end;
write(patout,xext[pat[d]]);
if hval[d]>0 then write(patout,xdig[hval[d]]);end;writeln(patout);
end{:73};if triel[t]>0 then outputpatterns(triel[t],patlen+1);end;end;
end;{:72}{76:}procedure readword;label 30,40;var c:textchar;
t:triepointer;begin begin bufptr:=0;
while not eoln(dictionary)do begin if(bufptr>=maxbuflen)then begin begin
bufptr:=0;repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Line too long');uexit(1);end;;end;
bufptr:=bufptr+1;read(dictionary,buf[bufptr]);end;readln(dictionary);
while bufptr<maxbuflen do begin bufptr:=bufptr+1;buf[bufptr]:=' ';end;
end;word[1]:=1;wlen:=1;bufptr:=0;repeat bufptr:=bufptr+1;c:=buf[bufptr];
case xclass[c]of 0:goto 40;
1:if wlen=1 then begin if xint[c]<>wordwt then wtchg:=true;
wordwt:=xint[c];end else dotw[wlen]:=xint[c];2:dots[wlen]:=xint[c];
3:begin wlen:=wlen+1;if wlen=maxlen then begin begin bufptr:=0;
repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'PATGEN capacity exceeded, sorry [','word length=',
maxlen:1,'].');uexit(1);end;;end;word[wlen]:=xint[c];dots[wlen]:=0;
dotw[wlen]:=wordwt;end;4:begin wlen:=wlen+1;
if wlen=maxlen then begin begin bufptr:=0;repeat bufptr:=bufptr+1;
write(stdout,buf[bufptr]);until bufptr=maxbuflen;writeln(stdout,' ');
end;
begin writeln(stdout,'PATGEN capacity exceeded, sorry [','word length=',
maxlen:1,'].');uexit(1);end;;end;begin t:=1;
while true do begin t:=triel[t]+xord[c];
if triec[t]<>xord[c]then begin begin bufptr:=0;repeat bufptr:=bufptr+1;
write(stdout,buf[bufptr]);until bufptr=maxbuflen;writeln(stdout,' ');
end;begin writeln(stdout,'Bad representation');uexit(1);end;;end;
if trier[t]<>0 then begin word[wlen]:=trier[t];goto 30;end;
if bufptr=maxbuflen then c:=' 'else begin bufptr:=bufptr+1;
c:=buf[bufptr];end;end;30:end;dots[wlen]:=0;dotw[wlen]:=wordwt;end;
5:begin begin bufptr:=0;repeat bufptr:=bufptr+1;
write(stdout,buf[bufptr]);until bufptr=maxbuflen;writeln(stdout,' ');
end;begin writeln(stdout,'Bad character');uexit(1);end;;end;end;
until bufptr=maxbuflen;40:wlen:=wlen+1;word[wlen]:=1;end;{:76}{77:}
procedure hyphenate;label 30;var spos,dpos,fpos:wordindex;t:triepointer;
h:optype;v:valtype;
begin for spos:=wlen-hyfmax downto 0 do begin nomore[spos]:=false;
hval[spos]:=0;fpos:=spos+1;t:=1+word[fpos];repeat h:=trier[t];
while h>0 do{80:}begin dpos:=spos+ops[h].dot;v:=ops[h].val;
if(v<maxval)and(hval[dpos]<v)then hval[dpos]:=v;
if(v>=hyphlevel)then if((fpos-patlen)<=(dpos-patdot))and((dpos-patdot)<=
spos)then nomore[dpos]:=true;h:=ops[h].op;end{:80};t:=triel[t];
if t=0 then goto 30;fpos:=fpos+1;t:=t+word[fpos];
until triec[t]<>word[fpos];30:end;end;{:77}{81:}procedure changedots;
var dpos:wordindex;
begin for dpos:=wlen-hyfmax downto hyfmin do begin if odd(hval[dpos])
then dots[dpos]:=dots[dpos]+1;
if dots[dpos]=3 then goodcount:=goodcount+dotw[dpos]else if dots[dpos]=1
then badcount:=badcount+dotw[dpos]else if dots[dpos]=2 then misscount:=
misscount+dotw[dpos];end;end;{:81}{82:}procedure outputhyphenatedword;
var dpos:wordindex;l:triecpointer;
begin if wtchg then begin write(pattmp,xdig[wordwt]);wtchg:=false end;
for dpos:=2 to wlen-2 do begin l:=triecl[1+word[dpos]];
while l>0 do begin write(pattmp,xchr[triecc[l]]);l:=triecl[l];end;
write(pattmp,xext[word[dpos]]);
if dots[dpos]<>0 then write(pattmp,xhyf[dots[dpos]]);
if dotw[dpos]<>wordwt then write(pattmp,xdig[dotw[dpos]]);end;
l:=triecl[1+word[wlen-1]];
while l>0 do begin write(pattmp,xchr[triecc[l]]);l:=triecl[l];end;
writeln(pattmp,xext[word[wlen-1]]);end;{:82}{83:}procedure doword;
label 22,30;var spos,dpos,fpos:wordindex;a:triecpointer;goodp:boolean;
begin for dpos:=wlen-dotmax downto dotmin do begin spos:=dpos-patdot;
fpos:=spos+patlen;{86:}if nomore[dpos]then goto 22;
if dots[dpos]=gooddot then goodp:=true else if dots[dpos]=baddot then
goodp:=false else goto 22;{:86};spos:=spos+1;a:=1+word[spos];
while spos<fpos do begin spos:=spos+1;a:=triecl[a]+word[spos];
if triecc[a]<>word[spos]then begin a:=insertcpat(fpos);goto 30;end;end;
30:if goodp then triecl[a]:=triecl[a]+dotw[dpos]else triecr[a]:=triecr[a
]+dotw[dpos];22:end;end;{:83}{88:}procedure dodictionary;
begin goodcount:=0;badcount:=0;misscount:=0;wordwt:=1;wtchg:=false;
argv(1,fname);reset(dictionary,fname);{75:}xclass['.']:=5;
xclass[xhyf[1]]:=2;xint[xhyf[1]]:=0;xclass[xhyf[2]]:=2;xint[xhyf[2]]:=2;
xclass[xhyf[3]]:=2;xint[xhyf[3]]:=2;{:75}{79:}hyfmin:=lefthyphenmin+1;
hyfmax:=righthyphenmin+1;hyflen:=hyfmin+hyfmax;{:79}{85:}
if procesp then begin dotmin:=patdot;dotmax:=patlen-patdot;
if dotmin<hyfmin then dotmin:=hyfmin;
if dotmax<hyfmax then dotmax:=hyfmax;dotlen:=dotmin+dotmax;
if odd(hyphlevel)then begin gooddot:=2;baddot:=0;
end else begin gooddot:=1;baddot:=3;end;end;{:85}
if procesp then begin initcounttrie;
writeln(stdout,'processing dictionary with pat_len = ',patlen:1,
', pat_dot = ',patdot:1);end;if hyphp then begin filnam[1]:='p';
filnam[2]:='a';filnam[3]:='t';filnam[4]:='t';filnam[5]:='m';
filnam[6]:='p';filnam[7]:='.';filnam[8]:=xdig[hyphlevel];
rewrite(pattmp,filnam);
writeln(stdout,'writing pattmp.',xdig[hyphlevel]);end;{89:}
while not eof(dictionary)do begin readword;
if wlen>=hyflen then begin hyphenate;changedots;end;
if hyphp then if wlen>2 then outputhyphenatedword;
if procesp then if wlen>=dotlen then doword;end{:89};
writeln(stdout,' ');
writeln(stdout,goodcount:1,' good, ',badcount:1,' bad, ',misscount:1,
' missed');
if(goodcount+misscount)>0 then begin printreal((100*goodcount/(goodcount
+misscount)),1,2);write(stdout,' %, ');
printreal((100*badcount/(goodcount+misscount)),1,2);
write(stdout,' %, ');
printreal((100*misscount/(goodcount+misscount)),1,2);
writeln(stdout,' %');end;
if procesp then writeln(stdout,patcount:1,' patterns, ',trieccount:1,
' nodes in count trie, ','triec_max = ',triecmax:1);if hyphp then;end;
{:88}{90:}procedure readpatterns;label 30,40;var c:textchar;d:digit;
i:dottype;t:triepointer;begin xclass['.']:=3;xint['.']:=1;
levelpatterncount:=0;maxpat:=0;argv(2,fname);reset(patterns,fname);
while not eof(patterns)do begin begin bufptr:=0;
while not eoln(patterns)do begin if(bufptr>=maxbuflen)then begin begin
bufptr:=0;repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Line too long');uexit(1);end;;end;
bufptr:=bufptr+1;read(patterns,buf[bufptr]);end;readln(patterns);
while bufptr<maxbuflen do begin bufptr:=bufptr+1;buf[bufptr]:=' ';end;
end;levelpatterncount:=levelpatterncount+1;{92:}patlen:=0;bufptr:=0;
hval[0]:=0;repeat bufptr:=bufptr+1;c:=buf[bufptr];
case xclass[c]of 0:goto 40;1:begin d:=xint[c];
if d>=maxval then begin begin bufptr:=0;repeat bufptr:=bufptr+1;
write(stdout,buf[bufptr]);until bufptr=maxbuflen;writeln(stdout,' ');
end;begin writeln(stdout,'Bad hyphenation value');uexit(1);end;;end;
if d>maxpat then maxpat:=d;hval[patlen]:=d;end;3:begin patlen:=patlen+1;
hval[patlen]:=0;pat[patlen]:=xint[c];end;4:begin patlen:=patlen+1;
hval[patlen]:=0;begin t:=1;while true do begin t:=triel[t]+xord[c];
if triec[t]<>xord[c]then begin begin bufptr:=0;repeat bufptr:=bufptr+1;
write(stdout,buf[bufptr]);until bufptr=maxbuflen;writeln(stdout,' ');
end;begin writeln(stdout,'Bad representation');uexit(1);end;;end;
if trier[t]<>0 then begin pat[patlen]:=trier[t];goto 30;end;
if bufptr=maxbuflen then c:=' 'else begin bufptr:=bufptr+1;
c:=buf[bufptr];end;end;30:end;end;2,5:begin begin bufptr:=0;
repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Bad character');uexit(1);end;;end;end;
until bufptr=maxbuflen{:92};40:{93:}
if patlen>0 then for i:=0 to patlen do begin if hval[i]<>0 then
insertpattern(hval[i],i);
if i>1 then if i<patlen then if pat[i]=1 then begin begin bufptr:=0;
repeat bufptr:=bufptr+1;write(stdout,buf[bufptr]);
until bufptr=maxbuflen;writeln(stdout,' ');end;
begin writeln(stdout,'Bad edge_of_word');uexit(1);end;;end;end{:93};end;
;writeln(stdout,levelpatterncount:1,' patterns read in');
writeln(stdout,'pattern trie has ',triecount:1,' nodes, ','trie_max = ',
triemax:1,', ',opcount:1,' outputs');end;{:90}{94:}begin initialize;
initpatterntrie;readtranslate;readpatterns;procesp:=true;hyphp:=false;
repeat write(stdout,'hyph_start, hyph_finish: ');input2ints(n1,n2);
if(n1>=1)and(n1<maxval)and(n2>=1)and(n2<maxval)then begin hyphstart:=n1;
hyphfinish:=n2;end else begin n1:=0;
writeln(stdout,'Specify 1<=hyph_start,hyph_finish<=',maxval-1:1,' !');
end;until n1>0;hyphlevel:=maxpat;
for i:=hyphstart to hyphfinish do begin hyphlevel:=i;
levelpatterncount:=0;
if hyphlevel>hyphstart then writeln(stdout,' ')else if hyphstart<=maxpat
then writeln(stdout,'Largest hyphenation value ',maxpat:1,
' in patterns should be less than hyph_start');
repeat write(stdout,'pat_start, pat_finish: ');input2ints(n1,n2);
if(n1>=1)and(n1<=n2)and(n2<=maxdot)then begin patstart:=n1;
patfinish:=n2;end else begin n1:=0;
writeln(stdout,'Specify 1<=pat_start<=pat_finish<=',maxdot:1,' !');end;
until n1>0;write(stdout,'good weight, bad weight, threshold: ');
input3ints(goodwt,badwt,thresh);{96:}
for k:=0 to maxdot do morethislevel[k]:=true;
for j:=patstart to patfinish do begin patlen:=j;patdot:=patlen div 2;
dot1:=patdot*2;repeat patdot:=dot1-patdot;dot1:=patlen*2-dot1-1;
if morethislevel[patdot]then begin dodictionary;collectcounttrie;
morethislevel[patdot]:=moretocome;end;until patdot=patlen;
for k:=maxdot downto 1 do if not morethislevel[k-1]then morethislevel[k]
:=false;end{:96};deletebadpatterns;
writeln(stdout,'total of ',levelpatterncount:1,
' patterns at hyph_level ',hyphlevel:1);end;findletters(triel[1],1);
argv(3,fname);rewrite(patout,fname);outputpatterns(1,1);;{97:}
procesp:=false;hyphp:=true;write(stdout,'hyphenate word list? ');
begin buf[1]:=getc(stdin);readln(stdin);end;
if(buf[1]='Y')or(buf[1]='y')then dodictionary{:97};9999:end.{:94}
