
	.section	".text", #alloc, #execinstr
	.align		8
	.skip		16
	.global canlock
	.type	canlock,#function

canlock:
	or	%g0,1,%o1
	swap	[%o0],%o1	! o0 points to lock; key is first word
	cmp	%o1,1
	bne	.gotit
	nop
	retl			! key was 1
	or	%g0,0,%o0
.gotit:
	retl			! key was not 1
	or	%g0,1,%o0

	.size	canlock,(.-canlock)
