{2:}program POOLtype;type{5:}ASCIIcode=0..255;{:5}var{7:}
xord:array[ASCIIcode]of ASCIIcode;xchr:array[ASCIIcode]of ASCIIcode;{:7}
{12:}k,l:0..255;m,n:ASCIIcode;s:integer;{:12}{13:}count:integer;{:13}
{18:}poolfile:packed file of ASCIIcode;
poolname:array[0..PATHMAX]of char;xsum:boolean;{:18}
procedure initialize;var{6:}i:integer;{:6}begin{8:}xchr[32]:=' ';
xchr[33]:='!';xchr[34]:='"';xchr[35]:='#';xchr[36]:='$';xchr[37]:='%';
xchr[38]:='&';xchr[39]:='''';xchr[40]:='(';xchr[41]:=')';xchr[42]:='*';
xchr[43]:='+';xchr[44]:=',';xchr[45]:='-';xchr[46]:='.';xchr[47]:='/';
xchr[48]:='0';xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';xchr[52]:='4';
xchr[53]:='5';xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';xchr[57]:='9';
xchr[58]:=':';xchr[59]:=';';xchr[60]:='<';xchr[61]:='=';xchr[62]:='>';
xchr[63]:='?';xchr[64]:='@';xchr[65]:='A';xchr[66]:='B';xchr[67]:='C';
xchr[68]:='D';xchr[69]:='E';xchr[70]:='F';xchr[71]:='G';xchr[72]:='H';
xchr[73]:='I';xchr[74]:='J';xchr[75]:='K';xchr[76]:='L';xchr[77]:='M';
xchr[78]:='N';xchr[79]:='O';xchr[80]:='P';xchr[81]:='Q';xchr[82]:='R';
xchr[83]:='S';xchr[84]:='T';xchr[85]:='U';xchr[86]:='V';xchr[87]:='W';
xchr[88]:='X';xchr[89]:='Y';xchr[90]:='Z';xchr[91]:='[';xchr[92]:='\';
xchr[93]:=']';xchr[94]:='^';xchr[95]:='_';xchr[96]:='`';xchr[97]:='a';
xchr[98]:='b';xchr[99]:='c';xchr[100]:='d';xchr[101]:='e';
xchr[102]:='f';xchr[103]:='g';xchr[104]:='h';xchr[105]:='i';
xchr[106]:='j';xchr[107]:='k';xchr[108]:='l';xchr[109]:='m';
xchr[110]:='n';xchr[111]:='o';xchr[112]:='p';xchr[113]:='q';
xchr[114]:='r';xchr[115]:='s';xchr[116]:='t';xchr[117]:='u';
xchr[118]:='v';xchr[119]:='w';xchr[120]:='x';xchr[121]:='y';
xchr[122]:='z';xchr[123]:='{';xchr[124]:='|';xchr[125]:='}';
xchr[126]:='~';{:8}{10:}for i:=0 to 31 do xchr[i]:=chr(i);
for i:=127 to 255 do xchr[i]:=chr(i);{:10}{11:}
for i:=0 to 255 do xord[chr(i)]:=127;
for i:=128 to 255 do xord[xchr[i]]:=i;
for i:=0 to 126 do xord[xchr[i]]:=i;{:11}{14:}count:=0;{:14}end;{:2}
{15:}begin initialize;{16:}for k:=0 to 255 do begin write(k:3,': "');
l:=k;if({17:}(k<32)or(k>126){:17})then begin write(xchr[94],xchr[94]);
if k<64 then l:=k+64 else if k<128 then l:=k-64 else begin l:=k div 16;
if l<10 then l:=l+48 else l:=l+87;write(xchr[l]);l:=k mod 16;
if l<10 then l:=l+48 else l:=l+87;count:=count+1;end;count:=count+2;end;
if l=34 then write(xchr[l],xchr[l])else write(xchr[l]);count:=count+1;
writeln('"');end{:16};s:=256;{19:}
if argc<>2 then begin writeln('Usage: pooltype <pool file>.');uexit(1);
end;argv(1,poolname);reset(poolfile,poolname);xsum:=false;
if eof(poolfile)then begin writeln('! I can''t read the POOL file.');
uexit(1);end;repeat{20:}
if eof(poolfile)then begin writeln('! POOL file contained no check sum')
;uexit(1);end;read(poolfile,m);read(poolfile,n);
if m<>'*'then begin if(xord[m]<48)or(xord[m]>57)or(xord[n]<48)or(xord[n]
>57)then begin writeln('! POOL line doesn''t begin with two digits');
uexit(1);end;l:=xord[m]*10+xord[n]-48*11;write(s:3,': "');
count:=count+l;
for k:=1 to l do begin if eoln(poolfile)then begin writeln('"');
begin writeln('! That POOL line was too short');uexit(1);end;end;
read(poolfile,m);write(xchr[xord[m]]);
if xord[m]=34 then write(xchr[34]);end;writeln('"');s:=s+1;
end else xsum:=true;readln(poolfile){:20};until xsum;
if not eof(poolfile)then begin writeln(
'! There''s junk after the check sum');uexit(1);end{:19};
writeln('(',count:1,' characters in all.)');uexit(0);end.{:15}
