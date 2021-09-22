void printanewline AA((void));
void markwarning AA((void));
void markerror AA((void));
void markfatal AA((void));
void printoverflow AA((void));
void printconfusion AA((void));
void bufferoverflow AA((void));
boolean zinputln AA((alphafile f));
#define inputln(f) zinputln((alphafile) (f))
void zoutpoolstr AA((alphafile f,strnumber s));
#define outpoolstr(f, s) zoutpoolstr((alphafile) (f), (strnumber) (s))
void zprintapoolstr AA((strnumber s));
#define printapoolstr(s) zprintapoolstr((strnumber) (s))
void pooloverflow AA((void));
void filenmsizeoverflow AA((void));
void zouttoken AA((alphafile f));
#define outtoken(f) zouttoken((alphafile) (f))
void printatoken AA((void));
void printbadinputline AA((void));
void printskippingwhateverremains AA((void));
void samtoolongfilenameprint AA((void));
void samwrongfilenameprint AA((void));
void printauxname AA((void));
void auxerrprint AA((void));
void zauxerrillegalanotherprint AA((integer cmdnum));
#define auxerrillegalanotherprint(cmdnum) zauxerrillegalanotherprint((integer) (cmdnum))
void auxerrnorightbraceprint AA((void));
void auxerrstuffafterrightbraceprint AA((void));
void auxerrwhitespaceinargumentprint AA((void));
void printbibname AA((void));
void printbstname AA((void));
void hashciteconfusion AA((void));
void zcheckciteoverflow AA((citenumber lastcite));
#define checkciteoverflow(lastcite) zcheckciteoverflow((citenumber) (lastcite))
void auxend1errprint AA((void));
void auxend2errprint AA((void));
void bstlnnumprint AA((void));
void bsterrprintandlookforblankline AA((void));
void bstwarnprint AA((void));
void eatbstprint AA((void));
void unknwnfunctionclassconfusion AA((void));
void zprintfnclass AA((hashloc fnloc));
#define printfnclass(fnloc) zprintfnclass((hashloc) (fnloc))
void ztraceprfnclass AA((hashloc fnloc));
#define traceprfnclass(fnloc) ztraceprfnclass((hashloc) (fnloc))
void idscanningconfusion AA((void));
void bstidprint AA((void));
void bstleftbraceprint AA((void));
void bstrightbraceprint AA((void));
void zalreadyseenfunctionprint AA((hashloc seenfnloc));
#define alreadyseenfunctionprint(seenfnloc) zalreadyseenfunctionprint((hashloc) (seenfnloc))
void singlfnoverflow AA((void));
void biblnnumprint AA((void));
void biberrprint AA((void));
void bibwarnprint AA((void));
void zcheckfieldoverflow AA((integer totalfields));
#define checkfieldoverflow(totalfields) zcheckfieldoverflow((integer) (totalfields))
void eatbibprint AA((void));
void zbiboneoftwoprint AA((ASCIIcode char1,ASCIIcode char2));
#define biboneoftwoprint(char1, char2) zbiboneoftwoprint((ASCIIcode) (char1), (ASCIIcode) (char2))
void bibequalssignprint AA((void));
void bibunbalancedbracesprint AA((void));
void bibfieldtoolongprint AA((void));
void macrowarnprint AA((void));
void bibidprint AA((void));
void bibcmdconfusion AA((void));
void citekeydisappearedconfusion AA((void));
void zbadcrossreferenceprint AA((strnumber s));
#define badcrossreferenceprint(s) zbadcrossreferenceprint((strnumber) (s))
void nonexistentcrossreferenceerror AA((void));
void zprintmissingentry AA((strnumber s));
#define printmissingentry(s) zprintmissingentry((strnumber) (s))
void bstexwarnprint AA((void));
void bstmildexwarnprint AA((void));
void bstcantmesswithentriesprint AA((void));
void illeglliteralconfusion AA((void));
void unknwnliteralconfusion AA((void));
void zprintstklit AA((integer stklt,stktype stktp));
#define printstklit(stklt, stktp) zprintstklit((integer) (stklt), (stktype) (stktp))
void zprintlit AA((integer stklt,stktype stktp));
#define printlit(stklt, stktp) zprintlit((integer) (stklt), (stktype) (stktp))
void outputbblline AA((void));
void bst1printstringsizeexceeded AA((void));
void bst2printstringsizeexceeded AA((void));
void zbracesunbalancedcomplaint AA((strnumber poplitvar));
#define bracesunbalancedcomplaint(poplitvar) zbracesunbalancedcomplaint((strnumber) (poplitvar))
void caseconversionconfusion AA((void));
void traceandstatprinting AA((void));
void zstartname AA((strnumber filename));
#define startname(filename) zstartname((strnumber) (filename))
void zaddextension AA((strnumber ext));
#define addextension(ext) zaddextension((strnumber) (ext))
void zaddarea AA((strnumber area));
#define addarea(area) zaddarea((strnumber) (area))
strnumber makestring AA((void));
boolean zstreqbuf AA((strnumber s,buftype buf,bufpointer bfptr,bufpointer len));
#define streqbuf(s, buf, bfptr, len) zstreqbuf((strnumber) (s),  (buf), (bufpointer) (bfptr), (bufpointer) (len))
boolean zstreqstr AA((strnumber s1,strnumber s2));
#define streqstr(s1, s2) zstreqstr((strnumber) (s1), (strnumber) (s2))
void zlowercase AA((buftype buf,bufpointer bfptr,bufpointer len));
#define lowercase(buf, bfptr, len) zlowercase( (buf), (bufpointer) (bfptr), (bufpointer) (len))
void zuppercase AA((buftype buf,bufpointer bfptr,bufpointer len));
#define uppercase(buf, bfptr, len) zuppercase( (buf), (bufpointer) (bfptr), (bufpointer) (len))
hashloc zstrlookup AA((buftype buf,bufpointer j,bufpointer l,strilk ilk,boolean insertit));
#define strlookup(buf, j, l, ilk, insertit) zstrlookup( (buf), (bufpointer) (j), (bufpointer) (l), (strilk) (ilk), (boolean) (insertit))
void zpredefine AA((pdstype pds,pdslen len,strilk ilk));
#define predefine(pds, len, ilk) zpredefine( (pds), (pdslen) (len), (strilk) (ilk))
void zzinttoASCII AA((integer theint,buftype intbuf,bufpointer intbegin,bufpointer * intend));
#define inttoASCII(theint, intbuf, intbegin, intend) zzinttoASCII((integer) (theint),  (intbuf), (bufpointer) (intbegin), (bufpointer *) &(intend))
void zzadddatabasecite AA((citenumber * newcite));
#define adddatabasecite(newcite) zzadddatabasecite((citenumber *) &(newcite))
boolean zfindcitelocsforthiscitekey AA((strnumber citestr));
#define findcitelocsforthiscitekey(citestr) zfindcitelocsforthiscitekey((strnumber) (citestr))
void zswap AA((citenumber swap1,citenumber swap2));
#define swap(swap1, swap2) zswap((citenumber) (swap1), (citenumber) (swap2))
boolean zlessthan AA((citenumber arg1,citenumber arg2));
#define lessthan(arg1, arg2) zlessthan((citenumber) (arg1), (citenumber) (arg2))
void zquicksort AA((citenumber leftend,citenumber rightend));
#define quicksort(leftend, rightend) zquicksort((citenumber) (leftend), (citenumber) (rightend))
void zzbuildin AA((pdstype pds,pdslen len,hashloc * fnhashloc,bltinrange bltinnum));
#define buildin(pds, len, fnhashloc, bltinnum) zzbuildin( (pds), (pdslen) (len), (hashloc *) &(fnhashloc), (bltinrange) (bltinnum))
void predefcertainstrings AA((void));
boolean zscan1 AA((ASCIIcode char1));
#define scan1(char1) zscan1((ASCIIcode) (char1))
boolean zscan1white AA((ASCIIcode char1));
#define scan1white(char1) zscan1white((ASCIIcode) (char1))
boolean zscan2 AA((ASCIIcode char1,ASCIIcode char2));
#define scan2(char1, char2) zscan2((ASCIIcode) (char1), (ASCIIcode) (char2))
boolean zscan2white AA((ASCIIcode char1,ASCIIcode char2));
#define scan2white(char1, char2) zscan2white((ASCIIcode) (char1), (ASCIIcode) (char2))
boolean zscan3 AA((ASCIIcode char1,ASCIIcode char2,ASCIIcode char3));
#define scan3(char1, char2, char3) zscan3((ASCIIcode) (char1), (ASCIIcode) (char2), (ASCIIcode) (char3))
boolean scanalpha AA((void));
void zscanidentifier AA((ASCIIcode char1,ASCIIcode char2,ASCIIcode char3));
#define scanidentifier(char1, char2, char3) zscanidentifier((ASCIIcode) (char1), (ASCIIcode) (char2), (ASCIIcode) (char3))
boolean scannonneginteger AA((void));
boolean scaninteger AA((void));
boolean scanwhitespace AA((void));
boolean eatbstwhitespace AA((void));
void skiptokenprint AA((void));
void printrecursionillegal AA((void));
void skptokenunknownfunctionprint AA((void));
void skipillegalstuffaftertokenprint AA((void));
void zscanfndef AA((hashloc fnhashloc));
#define scanfndef(fnhashloc) zscanfndef((hashloc) (fnhashloc))
boolean eatbibwhitespace AA((void));
boolean compressbibwhite AA((void));
boolean scanbalancedbraces AA((void));
boolean scanafieldtokenandeatwhite AA((void));
boolean scanandstorethefieldvalueandeatwhite AA((void));
void zdecrbracelevel AA((strnumber poplitvar));
#define decrbracelevel(poplitvar) zdecrbracelevel((strnumber) (poplitvar))
void zcheckbracelevel AA((strnumber poplitvar));
#define checkbracelevel(poplitvar) zcheckbracelevel((strnumber) (poplitvar))
void znamescanforand AA((strnumber poplitvar));
#define namescanforand(poplitvar) znamescanforand((strnumber) (poplitvar))
boolean vontokenfound AA((void));
void vonnameendsandlastnamestartsstuff AA((void));
void skipstuffatspbracelevelgreaterthanone AA((void));
void bracelvloneletterscomplaint AA((void));
boolean zenoughtextchars AA((bufpointer enoughchars));
#define enoughtextchars(enoughchars) zenoughtextchars((bufpointer) (enoughchars))
void figureouttheformattedname AA((void));
void zpushlitstk AA((integer pushlt,stktype pushtype));
#define pushlitstk(pushlt, pushtype) zpushlitstk((integer) (pushlt), (stktype) (pushtype))
void zzpoplitstk AA((integer * poplit,stktype * poptype));
#define poplitstk(poplit, poptype) zzpoplitstk((integer *) &(poplit), (stktype *) &(poptype))
void zprintwrongstklit AA((integer stklt,stktype stktp1,stktype stktp2));
#define printwrongstklit(stklt, stktp1, stktp2) zprintwrongstklit((integer) (stklt), (stktype) (stktp1), (stktype) (stktp2))
void poptopandprint AA((void));
void popwholestack AA((void));
void initcommandexecution AA((void));
void checkcommandexecution AA((void));
void addpoolbufandpush AA((void));
void zaddbufpool AA((strnumber pstr));
#define addbufpool(pstr) zaddbufpool((strnumber) (pstr))
void zaddoutpool AA((strnumber pstr));
#define addoutpool(pstr) zaddoutpool((strnumber) (pstr))
void xequals AA((void));
void xgreaterthan AA((void));
void xlessthan AA((void));
void xplus AA((void));
void xminus AA((void));
void xconcatenate AA((void));
void xgets AA((void));
void xaddperiod AA((void));
void xchangecase AA((void));
void xchrtoint AA((void));
void xcite AA((void));
void xduplicate AA((void));
void xempty AA((void));
void xformatname AA((void));
void xinttochr AA((void));
void xinttostr AA((void));
void xmissing AA((void));
void xnumnames AA((void));
void xpreamble AA((void));
void xpurify AA((void));
void xquote AA((void));
void xsubstring AA((void));
void xswap AA((void));
void xtextlength AA((void));
void xtextprefix AA((void));
void xtype AA((void));
void xwarning AA((void));
void xwidth AA((void));
void xwrite AA((void));
void zexecutefn AA((hashloc exfnloc));
#define executefn(exfnloc) zexecutefn((hashloc) (exfnloc))
void getthetoplevelauxfilename AA((void));
void auxbibdatacommand AA((void));
void auxbibstylecommand AA((void));
void auxcitationcommand AA((void));
void auxinputcommand AA((void));
void poptheauxstack AA((void));
void getauxcommandandprocess AA((void));
void lastcheckforauxerrors AA((void));
void bstentrycommand AA((void));
boolean badargumenttoken AA((void));
void bstexecutecommand AA((void));
void bstfunctioncommand AA((void));
void bstintegerscommand AA((void));
void bstiteratecommand AA((void));
void bstmacrocommand AA((void));
void getbibcommandorentryandprocess AA((void));
void bstreadcommand AA((void));
void bstreversecommand AA((void));
void bstsortcommand AA((void));
void bststringscommand AA((void));
void getbstcommandandprocess AA((void));
void initialize AA((void));
void parsearguments AA((void));
