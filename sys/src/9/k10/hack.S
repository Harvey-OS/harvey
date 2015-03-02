.global abs
abs: popq %rdi; movq $abs, %rsi; call badcall // ndnr
.global acidthandlers
acidthandlers: popq %rdi; movq $acidthandlers, %rsi; call badcall // ndnr
.global _acsysret
_acsysret: popq %rdi; movq $_acsysret, %rsi; call badcall // ndnr
.global alt
alt: popq %rdi; movq $alt, %rsi; call badcall // ndnr
.global _amd64_bin_auth_factotumcode
_amd64_bin_auth_factotumcode: popq %rdi; movq $_amd64_bin_auth_factotumcode, %rsi; call badcall // ndnr
.global _amd64_bin_auth_factotumlen
_amd64_bin_auth_factotumlen: popq %rdi; movq $_amd64_bin_auth_factotumlen, %rsi; call badcall // ndnr
.global _amd64_bin_ip_ipconfigcode
_amd64_bin_ip_ipconfigcode: popq %rdi; movq $_amd64_bin_ip_ipconfigcode, %rsi; call badcall // ndnr
.global _amd64_bin_ip_ipconfiglen
_amd64_bin_ip_ipconfiglen: popq %rdi; movq $_amd64_bin_ip_ipconfiglen, %rsi; call badcall // ndnr
.global atoi
atoi: popq %rdi; movq $atoi, %rsi; call badcall // ndnr
.global chancreate
chancreate: popq %rdi; movq $chancreate, %rsi; call badcall // ndnr
#if 0
.global chartorune
chartorune: popq %rdi; movq $chartorune, %rsi; call badcall // ndnr
#endif
.global cistrcmp
cistrcmp: popq %rdi; movq $cistrcmp, %rsi; call badcall // ndnr
.global cistrncmp
cistrncmp: popq %rdi; movq $cistrncmp, %rsi; call badcall // ndnr
.global cleanname
cleanname: popq %rdi; movq $cleanname, %rsi; call badcall // ndnr
.global convD2M
convD2M: popq %rdi; movq $convD2M, %rsi; call badcall // ndnr
.global convM2D
convM2D: popq %rdi; movq $convM2D, %rsi; call badcall // ndnr
.global convM2S
convM2S: popq %rdi; movq $convM2S, %rsi; call badcall // ndnr
.global convS2M
convS2M: popq %rdi; movq $convS2M, %rsi; call badcall // ndnr
.global defmask
defmask: popq %rdi; movq $defmask, %rsi; call badcall // ndnr
.global dirfmt
dirfmt: popq %rdi; movq $dirfmt, %rsi; call badcall // ndnr
.global eipfmt
eipfmt: popq %rdi; movq $eipfmt, %rsi; call badcall // ndnr
.global exec
exec: popq %rdi; movq $exec, %rsi; call badcall // ndnr
.global exits
exits: popq %rdi; movq $exits, %rsi; call badcall // ndnr
.global fcallfmt
fcallfmt: popq %rdi; movq $fcallfmt, %rsi; call badcall // ndnr
#if 0
.global fmtinstall
fmtinstall: popq %rdi; movq $fmtinstall, %rsi; call badcall // ndnr
.global fmtprint
fmtprint: popq %rdi; movq $fmtprint, %rsi; call badcall // ndnr
.global fmtstrcpy
fmtstrcpy: popq %rdi; movq $fmtstrcpy, %rsi; call badcall // ndnr
.global fmtstrflush
fmtstrflush: popq %rdi; movq $fmtstrflush, %rsi; call badcall // ndnr
.global fmtstrinit
fmtstrinit: popq %rdi; movq $fmtstrinit, %rsi; call badcall // ndnr
#endif
.global fork
fork: popq %rdi; movq $fork, %rsi; call badcall // ndnr
#if 0
.global fprint
fprint: popq %rdi; movq $fprint, %rsi; call badcall // ndnr
.global getcallerpc
getcallerpc: popq %rdi; movq $getcallerpc, %rsi; call badcall // ndnr
.global getfields
getfields: popq %rdi; movq $getfields, %rsi; call badcall // ndnr
#endif
.global hmac_sha1
hmac_sha1: popq %rdi; movq $hmac_sha1, %rsi; call badcall // ndnr
.global hnputl
hnputl: popq %rdi; movq $hnputl, %rsi; call badcall // ndnr
.global hnputs
hnputs: popq %rdi; movq $hnputs, %rsi; call badcall // ndnr
.global idthandlers
idthandlers: popq %rdi; movq $idthandlers, %rsi; call badcall // ndnr
.global initcode
initcode: popq %rdi; movq $initcode, %rsi; call badcall // ndnr
.global IPnoaddr
IPnoaddr: popq %rdi; movq $IPnoaddr, %rsi; call badcall // ndnr
.global kerndate
kerndate: popq %rdi; movq $kerndate, %rsi; call badcall // ndnr
.global maskip
maskip: popq %rdi; movq $maskip, %rsi; call badcall // ndnr
#if 0
.global memccpy
memccpy: popq %rdi; movq $memccpy, %rsi; call badcall // ndnr
.global memchr
memchr: popq %rdi; movq $memchr, %rsi; call badcall // ndnr
.global memcmp
memcmp: popq %rdi; movq $memcmp, %rsi; call badcall // ndnr
.global memmove
memmove: popq %rdi; movq $memmove, %rsi; call badcall // ndnr
.global memset
memset: popq %rdi; movq $memset, %rsi; call badcall // ndnr
#endif
.global nhgetl
nhgetl: popq %rdi; movq $nhgetl, %rsi; call badcall // ndnr
.global nhgets
nhgets: popq %rdi; movq $nhgets, %rsi; call badcall // ndnr
.global open
open: popq %rdi; movq $open, %rsi; call badcall // ndnr
.global parseip
parseip: popq %rdi; movq $parseip, %rsi; call badcall // ndnr
.global parseipmask
parseipmask: popq %rdi; movq $parseipmask, %rsi; call badcall // ndnr
.global pread
pread: popq %rdi; movq $pread, %rsi; call badcall // ndnr
.global procrfork
procrfork: popq %rdi; movq $procrfork, %rsi; call badcall // ndnr
.global qsort
qsort: popq %rdi; movq $qsort, %rsi; call badcall // ndnr
#if 0
.global quotefmtinstall
quotefmtinstall: popq %rdi; movq $quotefmtinstall, %rsi; call badcall // ndnr
.global runetochar
runetochar: popq %rdi; movq $runetochar, %rsi; call badcall // ndnr
#endif
.global sdfakescsi
sdfakescsi: popq %rdi; movq $sdfakescsi, %rsi; call badcall // ndnr
.global sdsetsense
sdsetsense: popq %rdi; movq $sdsetsense, %rsi; call badcall // ndnr
.global sendp
sendp: popq %rdi; movq $sendp, %rsi; call badcall // ndnr
.global sipi
sipi: popq %rdi; movq $sipi, %rsi; call badcall // ndnr
#if 0
.global seprint
seprint: popq %rdi; movq $seprint, %rsi; call badcall // ndnr
.global smprint
smprint: popq %rdi; movq $smprint, %rsi; call badcall // ndnr
.global snprint
snprint: popq %rdi; movq $snprint, %rsi; call badcall // ndnr
.global sprint
sprint: popq %rdi; movq $sprint, %rsi; call badcall // ndnr
#endif
.global ssldevtab
ssldevtab: popq %rdi; movq $ssldevtab, %rsi; call badcall // ndnr
.global statcheck
statcheck: popq %rdi; movq $statcheck, %rsi; call badcall // ndnr
#if 0
.global strchr
strchr: popq %rdi; movq $strchr, %rsi; call badcall // ndnr
.global strcmp
strcmp: popq %rdi; movq $strcmp, %rsi; call badcall // ndnr
.global strcpy
strcpy: popq %rdi; movq $strcpy, %rsi; call badcall // ndnr
.global strdup
strdup: popq %rdi; movq $strdup, %rsi; call badcall // ndnr
.global strecpy
strecpy: popq %rdi; movq $strecpy, %rsi; call badcall // ndnr
.global strlen
strlen: popq %rdi; movq $strlen, %rsi; call badcall // ndnr
.global strncmp
strncmp: popq %rdi; movq $strncmp, %rsi; call badcall // ndnr
.global strncpy
strncpy: popq %rdi; movq $strncpy, %rsi; call badcall // ndnr
.global strrchr
strrchr: popq %rdi; movq $strrchr, %rsi; call badcall // ndnr
.global strstr
strstr: popq %rdi; movq $strstr, %rsi; call badcall // ndnr
.global strtol
strtol: popq %rdi; movq $strtol, %rsi; call badcall // ndnr
.global strtoul
strtoul: popq %rdi; movq $strtoul, %rsi; call badcall // ndnr
.global strtoull
strtoull: popq %rdi; movq $strtoull, %rsi; call badcall // ndnr
#endif
.global syscallentry
syscallentry: popq %rdi; movq $syscallentry, %rsi; call badcall // ndnr
.global sysrforkret
sysrforkret: popq %rdi; movq $sysrforkret, %rsi; call badcall // ndnr
.global threadexits
threadexits: popq %rdi; movq $threadexits, %rsi; call badcall // ndnr
.global threadexitsall
threadexitsall: popq %rdi; movq $threadexitsall, %rsi; call badcall // ndnr
.global tokenize
tokenize: popq %rdi; movq $tokenize, %rsi; call badcall // ndnr
.global touser
touser: popq %rdi; movq $touser, %rsi; call badcall // ndnr
#if 0
.global utfnlen
utfnlen: popq %rdi; movq $utfnlen, %rsi; call badcall // ndnr
.global utfrune
utfrune: popq %rdi; movq $utfrune, %rsi; call badcall // ndnr
.global vseprint
vseprint: popq %rdi; movq $vseprint, %rsi; call badcall // ndnr
.global vsnprint
vsnprint: popq %rdi; movq $vsnprint, %rsi; call badcall // ndnr
#endif
.global write
write: popq %rdi; movq $write, %rsi; call badcall // ndnr
.global xactouser
xactouser: popq %rdi; movq $xactouser, %rsi; call badcall // ndnr
	ret
.global rerrstr
rerrstr: popq %rdi; movq $rerrstr, %rsi; call badcall // ndnr
