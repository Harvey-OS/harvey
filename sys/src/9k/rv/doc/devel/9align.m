From miller@hamnavoe.com Fri Oct 30 12:33:48 PDT 2020
Date: Fri, 30 Oct 2020 19:32:41 +0000
From: Richard Miller <miller@hamnavoe.com>
To: geoff@collyer.net
Subject: kernel patches for ic/il alignment
Message-ID: <9b141bea0d4603fe20dcfa598cde1a8d@hamnavoe.com>

Here are the changes I've made to my (old) 32 bit kernel for alignment.

/n/dump/2020/1027/sys/src/9k/port/syscall.c:179,187 - port/syscall.c:179,188
  	 * e.g. 32-bit SPARC, where the stack must be 8-byte
  	 * aligned although pointers and integers are 32-bits.
  	 */
- 	USED(argc);
+ 	int frame;
  
- 	return STACKALIGN(stack);
+ 	frame = argc * sizeof(char*);
+ 	return STACKALIGN(stack - frame) + frame;
  }
  
  void*
/n/dump/2020/1027/sys/src/9k/port/sysproc.c:489,496 - port/sysproc.c:489,496
  	char **argv;
  
  	p = UINT2PTR(ed->stack);
- 	ed->stack = sysexecstack(ed->stack, ed->argc);
- 	if(ed->stack - (ed->argc+1)*sizeof(char**) - PGSZ < TSTKTOP-USTKSIZE)
+ 	ed->stack = sysexecstack(ed->stack, ed->argc + 2);
+ 	if(ed->stack - (ed->argc+2)*sizeof(char**) - PGSZ < TSTKTOP-USTKSIZE)
  		// error("exec: stack too small");
  		error(Ebadexec);
  
/n/dump/2020/1027/sys/src/9k/pf/dat.h:28,34 - pf/dat.h:28,34
  #pragma incomplete Ureg
  
  #define SYSCALLTYPE	8	/* nominal syscall trap type (not used) */
- #define MAXSYSARG	5	/* for mount(fd, afd, mpt, flag, arg) */
+ #define MAXSYSARG	6	/* for seek(ret, fd, padding, offs, whence) */
  
  #define INTRSVCVOID
  #define Intrsvcret void
/n/dump/2020/1027/sys/src/9k/pf/ethertemu.c:76,86 - pf/ethertemu.c:76,84
  struct Vused
  {
  	Vhdr;
- 	struct {
- 		u32int	id;
- 		u32int	len;
- 	}	ring[1];
+ 	u32int	ring[1][2];
  };
+ enum { Id = 0, Len = 1 };
  
  struct Vqueue {
  	uint	mask;
/n/dump/2020/1027/sys/src/9k/pf/ethertemu.c:230,237 - pf/ethertemu.c:228,235
  		found = 0;
  		q = &ctlr->q[Recv];
  		while(q->used->idx != q->lastused){
- 			i = q->used->ring[q->lastused & q->mask].id;
- 			n = q->used->ring[q->lastused & q->mask].len;
+ 			i = q->used->ring[q->lastused & q->mask][Id];
+ 			n = q->used->ring[q->lastused & q->mask][Len];
  			bp = vdeq(q);
  			if(bp == nil || PADDR(bp->wp) != q->desc[i].addr)
  				panic("vnet recv queue out of sync wp %#p addr %#llux",
/n/dump/2020/1027/sys/src/9k/pf/ethertemu.c:250,256 - pf/ethertemu.c:248,254
  			WR(QueueNotify, Recv);
  		q = &ctlr->q[Xmit];
  		while(q->used->idx != q->lastused){
- 			i = q->used->ring[q->lastused & q->mask].id;
+ 			i = q->used->ring[q->lastused & q->mask][Id];
  			bp = vdeq(q);
  			if(bp == nil || PADDR(bp->rp) != q->desc[i].addr)
  				panic("vnet xmit queue out of sync");
/n/dump/2020/1027/sys/src/9k/pf/main.c:512,518 - pf/main.c:512,518
  	 * unused so it doesn't matter (at the moment...).
  	 * Added +1 to ensure nil isn't stepped on.
  	 */
- 	av = (char**)(p - (oargc+2+1)*sizeof(char*));
+ 	av = (char**)(p - (oargc+2+2)*sizeof(char*));
  	ssize = base + PGSZ - PTR2UINT(av);
  	*av++ = (char*)oargc;
  	for(i = 0; i < oargc; i++)
/n/dump/2020/1027/sys/src/9k/pf/mem.h:48,54 - pf/mem.h:48,54
  #define MACHMAX		16
  
  #define KSTACK		(16*1024)		/* Size of Proc kernel stack */
- #define STACKALIGN(sp)	((sp) & ~(BY2SE-1))	/* bug: assure with alloc */
+ #define STACKALIGN(sp)	((sp) & ~(sizeof(vlong)-1))	/* bug: assure with alloc */
  
  /*
   * Time
/n/dump/2020/1027/sys/src/9k/pf/riscv.h:4,10 - pf/riscv.h:4,10
  #define SYSTEM	0x73			/* op code */
  #define ISCOMPRESSED(inst) (((inst) & MASK(2)) != 3)
  
- #define UREGSIZE (BY2SE*(32+5))		/* room for pc, regs, csrs, mode */
+ #define UREGSIZE (BY2SE*(32+6))		/* room for pc, regs, csrs, mode */
  
  /* registers */
  #define LINK	R1
/n/dump/2020/1027/sys/src/9k/pf/riscvl.h:1,7 - pf/riscvl.h:1,7
  #define SYSTEM	0x73			/* op code */
  #define ISCOMPRESSED(inst) (((inst) & MASK(2)) != 3)
  
- #define UREGSIZE (BY2SE*(32+5))		/* room for pc, regs, csrs, mode */
+ #define UREGSIZE (BY2SE*(32+6))		/* room for pc, regs, csrs, mode */
  
  #define LINK	R1
  #define SP	R2

