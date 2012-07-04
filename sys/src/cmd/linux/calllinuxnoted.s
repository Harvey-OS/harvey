/* Call Noted but use the linux syscall path.
 * Only one arg.
 */
	TEXT	calllinuxnoted+0(SB),0,$0
	MOVQ	RARG, DI
	MOVQ	$1024, AX
	SYSCALL	
	RET	,
	END	,
