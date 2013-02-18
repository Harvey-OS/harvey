	.section	".text", #alloc, #execinstr
	.align		8
	.skip		16
	.global getcallerpc
	.type getcallerpc, #function
getcallerpc:                  ! ignore argument
	retl                    
	add %i7,0,%o0

	.size   getcallerpc,(.-getcallerpc)

