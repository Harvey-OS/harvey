.global touser
touser: popq %rdi; movq $touser, %rsi; call badcall // ndnr
.global syscallentry
syscallentry: popq %rdi; movq $syscallentry, %rsi; call badcall // ndnr
.global _amd64_bin_auth_factotumcode
_amd64_bin_auth_factotumcode: popq %rdi; movq $_amd64_bin_auth_factotumcode, %rsi; call badcall // ndnr
.global _amd64_bin_auth_factotumlen
_amd64_bin_auth_factotumlen: popq %rdi; movq $_amd64_bin_auth_factotumlen, %rsi; call badcall // ndnr
.global _amd64_bin_ip_ipconfigcode
_amd64_bin_ip_ipconfigcode: popq %rdi; movq $_amd64_bin_ip_ipconfigcode, %rsi; call badcall // ndnr
.global _amd64_bin_ip_ipconfiglen
_amd64_bin_ip_ipconfiglen: popq %rdi; movq $_amd64_bin_ip_ipconfiglen, %rsi; call badcall // ndnr
.global rerrstr
rerrstr: popq %rdi; movq $rerrstr, %rsi; call badcall // ndnr
.global sysrforkret
sysrforkret: popq %rdi; movq $sysrforkret, %rsi; call badcall // ndnr
