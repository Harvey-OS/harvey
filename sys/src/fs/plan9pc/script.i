unsigned long na_script[] = {
			/*	extern	scsi_id_buf */
			/*	extern	msg_out_buf */
			/*	extern	cmd_buf */
			/*	extern	data_buf */
			/*	extern	status_buf */
			/*	extern	msgin_buf */
			/*	extern	dsa_0 */
			/*	extern  dsa_1 */
			/*	extern	dsa_head */
			/*	SIR_MSG_IO_COMPLETE = 0 */
			/*	error_not_cmd_complete = 1 */
			/*	error_disconnected = 2 */
			/*	error_reselected = 3 */
			/*	error_unexpected_phase = 4 */
			/*	error_weird_message = 5 */
			/*	error_not_msg_in_after_reselect = 6 */
			/*	error_not_identify_after_reselect = 7 */
			/*	error_too_much_data = 8 */
			/*	error_too_little_data = 9 */
			/*	SIR_MSG_REJECT = 10 */
			/*	SIR_MSG_SDTR = 11 */
			/*	SIR_EV_RESPONSE_OK = 12 */
			/*	error_sigp_set = 13 */
			/*	SIR_EV_PHASE_SWITCH_AFTER_ID = 14 */
			/*	SIR_MSG_WDTR = 15 */
			/*	SIR_MSG_IGNORE_WIDE_RESIDUE = 16 */
			/*	SIR_NOTIFY_DISC = 100 */
			/*	SIR_NOTIFY_RESELECT = 101 */
			/*	SIR_NOTIFY_MSG_IN = 102 */
			/*	SIR_NOTIFY_STATUS = 103 */
			/*	SIR_NOTIFY_DUMP = 104 */
			/*	SIR_NOTIFY_DUMP2 = 105 */
			/*	SIR_NOTIFY_SIGP = 106 */
			/*	SIR_NOTIFY_ISSUE = 107 */
			/*	SIR_NOTIFY_WAIT_RESELECT = 108 */
			/*	SIR_NOTIFY_ISSUE_CHECK = 109 */
			/*	SIR_NOTIFY_DUMP_NEXT_CODE = 110 */
			/*	SIR_NOTIFY_COMMAND = 111 */
			/*	SIR_NOTIFY_DATA_IN = 112 */
			/*	SIR_NOTIFY_DATA_OUT = 113 */
			/*	SIR_NOTIFY_BLOCK_DATA_IN = 114 */
			/*	SIR_NOTIFY_WSR = 115 */
			/*	SIR_NOTIFY_LOAD_SYNC = 116 */
			/*	SIR_NOTIFY_RESELECTED_ON_SELECT = 117 */
			/*	STATE_FREE = 0 */
			/*	STATE_ALLOCATED = 1 */
			/*	STATE_ISSUE = 2 */
			/*	STATE_DISCONNECTED = 3 */
			/*	STATE_DONE = 4 */
			/*	RESULT_OK = 0 */
			/*	MSG_IDENTIFY = 0x80 */
			/*	MSG_DISCONNECT = 0x04 */
			/*	MSG_SAVE_DATA_POINTER = 0x02 */
			/*	MSG_RESTORE_POINTERS = 0x03 */
			/*	MSG_IGNORE_WIDE_RESIDUE = 0x23 */
			/*	X_MSG = 0x01 */
			/*	X_MSG_SDTR = 0x01 */
			/*	X_MSG_WDTR = 0x03 */
			/*	MSG_REJECT = 0x07 */
			/*	BSIZE = 512 */
/* 0000 */ 0x80880000L, /*		jump	wait_for_reselection */
/* 0004 */ 0x0000052cL,
/* 0008 */ 0x88880000L, /*		call	load_sync */
/* 000c */ 0x000007a4L,
/* 0010 */ 0x78180d00L, /*		move	13 to ctest0 */
/* 0014 */ 0x00000000L,
/* 0018 */ 0x60000200L, /*		clear	target */
/* 001c */ 0x00000000L,
/* 0020 */ 0x47000000L, /*		select	atn from scsi_id_buf, reselected_on_select */
/* 0024 */ 0x000004fcL,
/* 0028 */ 0x878b0000L, /*		jump	start1, when msg_in */
/* 002c */ 0x00000000L,
/* 0030 */ 0x78180e00L, /*		move	14 to ctest0 */
/* 0034 */ 0x00000000L,
/* 0038 */ 0x1e000000L, /*		move	from msg_out_buf, when msg_out */
/* 003c */ 0x00000001L,
/* 0040 */ 0x868b0000L, /*		jump	start1, when msg_out */
/* 0044 */ 0x00ffffe8L,
/* 0048 */ 0x82830000L, /*		jump	to_decisions, when not cmd */
/* 004c */ 0x00000620L,
/* 0050 */ 0x60000008L, /*		clear	atn */
/* 0054 */ 0x00000000L,
/* 0058 */ 0x1a000000L, /*		move	from cmd_buf, when cmd */
/* 005c */ 0x00000002L,
/* 0060 */ 0x81830000L, /*		jump	to_decisions, when not data_in */
/* 0064 */ 0x00000608L,
/* 0068 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 006c */ 0x000006a0L,
/* 0070 */ 0x00000034L,
/* 0074 */ 0xc0000004L, /*		move	memory 4, dmaaddr, scratchb */
/* 0078 */ 0x000006a4L,
/* 007c */ 0x0000005cL,
/* 0080 */ 0x72360000L, /*		move	scratcha2 to sfbr */
/* 0084 */ 0x00000000L,
/* 0088 */ 0x808c0000L, /*		jump	data_in_normal, if 0 */
/* 008c */ 0x00000078L,
/* 0090 */ 0x29000200L, /*		move	BSIZE, ptr dmaaddr, when data_in */
/* 0094 */ 0x000006a4L,
/* 0098 */ 0x7e5d0200L, /*		move	scratchb1 + BSIZE / 256 to scratchb1 */
/* 009c */ 0x00000000L,
/* 00a0 */ 0x7f5e0000L, /*		move	scratchb2 + 0 to scratchb2 with carry */
/* 00a4 */ 0x00000000L,
/* 00a8 */ 0x7f5f0000L, /*		move	scratchb3 + 0 to scratchb3 with carry */
/* 00ac */ 0x00000000L,
/* 00b0 */ 0x7e36ff00L, /*		move	scratcha2 + 255 to scratcha2 */
/* 00b4 */ 0x00000000L,
/* 00b8 */ 0xc0000004L, /*		move	memory 4, scratchb, dmaaddr */
/* 00bc */ 0x0000005cL,
/* 00c0 */ 0x000006a4L,
/* 00c4 */ 0x818b0000L, /*		jump	data_in_block_loop, when data_in */
/* 00c8 */ 0x00ffffb4L,
/* 00cc */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 00d0 */ 0x00000034L,
/* 00d4 */ 0x000006a0L,
/* 00d8 */ 0x88880000L, /*		call	save_state */
/* 00dc */ 0x00000608L,
/* 00e0 */ 0x80880000L, /*		jump	to_decisions */
/* 00e4 */ 0x00000588L,
/* 00e8 */ 0xc0000004L, /*		move	memory 4, scratchb, dmaaddr */
/* 00ec */ 0x0000005cL,
/* 00f0 */ 0x000006a4L,
/* 00f4 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 00f8 */ 0x00000034L,
/* 00fc */ 0x000006a0L,
/* 0100 */ 0x80880000L, /*		jump	to_decisions */
/* 0104 */ 0x00000568L,
/* 0108 */ 0x72370000L, /*		move	scratcha3 to sfbr */
/* 010c */ 0x00000000L,
/* 0110 */ 0x98040000L, /*		int	error_too_much_data, if not 0 */
/* 0114 */ 0x00000008L,
/* 0118 */ 0x19000000L, /*		move	from data_buf, when data_in */
/* 011c */ 0x00000003L,
/* 0120 */ 0x78370100L, /*		move	1 to scratcha3 */
/* 0124 */ 0x00000000L,
/* 0128 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 012c */ 0x00000034L,
/* 0130 */ 0x000006a0L,
/* 0134 */ 0x88880000L, /*		call	save_state */
/* 0138 */ 0x000005acL,
/* 013c */ 0x80880000L, /*		jump	post_data_to_decisions */
/* 0140 */ 0x00000544L,
/* 0144 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 0148 */ 0x000006a0L,
/* 014c */ 0x00000034L,
/* 0150 */ 0xc0000004L, /*		move	memory 4, dmaaddr, scratchb */
/* 0154 */ 0x000006a4L,
/* 0158 */ 0x0000005cL,
/* 015c */ 0x72360000L, /*		move	scratcha2 to sfbr */
/* 0160 */ 0x00000000L,
/* 0164 */ 0x808c0000L, /*		jump	data_out_normal, if 0 */
/* 0168 */ 0x0000005cL,
/* 016c */ 0xc0000004L, /*		move	memory 4, dmaaddr, scratchb */
/* 0170 */ 0x000006a4L,
/* 0174 */ 0x0000005cL,
/* 0178 */ 0x28000200L, /*		move	BSIZE, ptr dmaaddr, when data_out */
/* 017c */ 0x000006a4L,
/* 0180 */ 0x7e5d0200L, /*		move	scratchb1 + BSIZE / 256 to scratchb1 */
/* 0184 */ 0x00000000L,
/* 0188 */ 0x7f5e0000L, /*		move	scratchb2 + 0 to scratchb2 with carry */
/* 018c */ 0x00000000L,
/* 0190 */ 0x7f5f0000L, /*		move	scratchb3 + 0 to scratchb3 with carry */
/* 0194 */ 0x00000000L,
/* 0198 */ 0x7e36ff00L, /*		move	scratcha2 + 255 to scratcha2 */
/* 019c */ 0x00000000L,
/* 01a0 */ 0xc0000004L, /*		move	memory 4, scratchb, dmaaddr */
/* 01a4 */ 0x0000005cL,
/* 01a8 */ 0x000006a4L,
/* 01ac */ 0x808b0000L, /*		jump	data_out_block_loop, when data_out */
/* 01b0 */ 0x00ffffa8L,
/* 01b4 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 01b8 */ 0x00000034L,
/* 01bc */ 0x000006a0L,
/* 01c0 */ 0x80880000L, /*		jump	to_decisions */
/* 01c4 */ 0x000004a8L,
/* 01c8 */ 0x72370000L, /*		move	scratcha3 to sfbr */
/* 01cc */ 0x00000000L,
/* 01d0 */ 0x98040000L, /*		int	error_too_little_data, if not 0 */
/* 01d4 */ 0x00000009L,
/* 01d8 */ 0x18000000L, /*		move	from data_buf, when data_out */
/* 01dc */ 0x00000003L,
/* 01e0 */ 0x78370100L, /*		move	1 to scratcha3 */
/* 01e4 */ 0x00000000L,
/* 01e8 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 01ec */ 0x00000034L,
/* 01f0 */ 0x000006a0L,
/* 01f4 */ 0x88880000L, /*		call	save_state */
/* 01f8 */ 0x000004ecL,
/* 01fc */ 0x80880000L, /*		jump	post_data_to_decisions */
/* 0200 */ 0x00000484L,
/* 0204 */ 0x1b000000L, /*		move	from status_buf, when status */
/* 0208 */ 0x00000004L,
/* 020c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0210 */ 0x00000004L,
/* 0214 */ 0x0f000001L, /*		move	1, scratcha, when msg_in */
/* 0218 */ 0x00000034L,
/* 021c */ 0x808c0007L, /*		jump	rejected, if MSG_REJECT */
/* 0220 */ 0x00000088L,
/* 0224 */ 0x808c0004L, /*		jump	disconnected, if MSG_DISCONNECT */
/* 0228 */ 0x00000298L,
/* 022c */ 0x808c0002L, /*		jump	msg_in_skip, if MSG_SAVE_DATA_POINTER */
/* 0230 */ 0x00000090L,
/* 0234 */ 0x808c0003L, /*		jump	msg_in_skip, if MSG_RESTORE_POINTERS */
/* 0238 */ 0x00000088L,
/* 023c */ 0x808c0023L, /*		jump	ignore_wide, if MSG_IGNORE_WIDE_RESIDUE */
/* 0240 */ 0x000001f0L,
/* 0244 */ 0x808c0001L, /*		jump	extended, if X_MSG */
/* 0248 */ 0x00000088L,
/* 024c */ 0x98040000L, /*		int	error_not_cmd_complete, if not 0 */
/* 0250 */ 0x00000001L,
/* 0254 */ 0x7c027e00L, /*		move	scntl2&0x7e to scntl2 */
/* 0258 */ 0x00000000L,
/* 025c */ 0x60000040L, /*		clear	ack */
/* 0260 */ 0x00000000L,
/* 0264 */ 0x48000000L, /*		wait	disconnect */
/* 0268 */ 0x00000000L,
/* 026c */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 0270 */ 0x000006a0L,
/* 0274 */ 0x00000034L,
/* 0278 */ 0x78340400L, /*		move	STATE_DONE to scratcha0 */
/* 027c */ 0x00000000L,
/* 0280 */ 0x78350000L, /*		move	RESULT_OK to scratcha1 */
/* 0284 */ 0x00000000L,
/* 0288 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 028c */ 0x00000034L,
/* 0290 */ 0x000006a0L,
/* 0294 */ 0x88880000L, /*		call	save_state */
/* 0298 */ 0x0000044cL,
/* 029c */ 0x98180000L, /*		intfly	0 */
/* 02a0 */ 0x00000000L,
/* 02a4 */ 0x80880000L, /*		jump	issue_check */
/* 02a8 */ 0x00000464L,
/* 02ac */ 0x98080000L, /*		int	SIR_MSG_REJECT */
/* 02b0 */ 0x0000000aL,
/* 02b4 */ 0x60000040L, /*		clear	ack */
/* 02b8 */ 0x00000000L,
/* 02bc */ 0x80880000L, /*		jump	to_decisions */
/* 02c0 */ 0x000003acL,
/* 02c4 */ 0x60000040L, /*		clear	ack */
/* 02c8 */ 0x00000000L,
/* 02cc */ 0x80880000L, /*		jump	to_decisions */
/* 02d0 */ 0x0000039cL,
/* 02d4 */ 0x60000040L, /*		clear	ack */
/* 02d8 */ 0x00000000L,
/* 02dc */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 02e0 */ 0x00000004L,
/* 02e4 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 02e8 */ 0x00000035L,
/* 02ec */ 0x808c0003L, /*		jump	ext_3, if 3 */
/* 02f0 */ 0x00000030L,
/* 02f4 */ 0x808c0002L, /*		jump	ext_2, if 2 */
/* 02f8 */ 0x00000098L,
/* 02fc */ 0x98040001L, /*		int	error_weird_message, if not 1 */
/* 0300 */ 0x00000005L,
/* 0304 */ 0x60000040L, /*		clear	ack */
/* 0308 */ 0x00000000L,
/* 030c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0310 */ 0x00000004L,
/* 0314 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0318 */ 0x00000035L,
/* 031c */ 0x80880000L, /*		jump	ext_done */
/* 0320 */ 0x000000c8L,
/* 0324 */ 0x60000040L, /*	ext_3:	clear	ack */
/* 0328 */ 0x00000000L,
/* 032c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0330 */ 0x00000004L,
/* 0334 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0338 */ 0x00000035L,
/* 033c */ 0x60000040L, /*		clear	ack */
/* 0340 */ 0x00000000L,
/* 0344 */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0348 */ 0x00000004L,
/* 034c */ 0x0f000001L, /*		move	1, scratcha2, when msg_in */
/* 0350 */ 0x00000036L,
/* 0354 */ 0x60000040L, /*		clear	ack */
/* 0358 */ 0x00000000L,
/* 035c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0360 */ 0x00000004L,
/* 0364 */ 0x0f000001L, /*		move	1, scratcha3, when msg_in */
/* 0368 */ 0x00000037L,
/* 036c */ 0x72350000L, /*		move	scratcha1 to sfbr */
/* 0370 */ 0x00000000L,
/* 0374 */ 0x80840001L, /*		jump	ext_done, if not X_MSG_SDTR */
/* 0378 */ 0x00000070L,
/* 037c */ 0x98080000L, /*	sdtr:	int	SIR_MSG_SDTR */
/* 0380 */ 0x0000000bL,
/* 0384 */ 0x60000040L, /*		clear	ack */
/* 0388 */ 0x00000000L,
/* 038c */ 0x80880000L, /*		jump	to_decisions */
/* 0390 */ 0x000002dcL,
/* 0394 */ 0x60000040L, /*	ext_2:	clear	ack */
/* 0398 */ 0x00000000L,
/* 039c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 03a0 */ 0x00000004L,
/* 03a4 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 03a8 */ 0x00000035L,
/* 03ac */ 0x60000040L, /*		clear	ack */
/* 03b0 */ 0x00000000L,
/* 03b4 */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 03b8 */ 0x00000004L,
/* 03bc */ 0x0f000001L, /*		move	1, scratcha2, when msg_in */
/* 03c0 */ 0x00000036L,
/* 03c4 */ 0x72350000L, /*		move	scratcha1 to sfbr */
/* 03c8 */ 0x00000000L,
/* 03cc */ 0x80840003L, /*		jump	ext_done, if not X_MSG_WDTR */
/* 03d0 */ 0x00000018L,
/* 03d4 */ 0x98080000L, /*	wdtr:	int	SIR_MSG_WDTR */
/* 03d8 */ 0x0000000fL,
/* 03dc */ 0x60000040L, /*		clear	ack */
/* 03e0 */ 0x00000000L,
/* 03e4 */ 0x80880000L, /*		jump	to_decisions */
/* 03e8 */ 0x00000284L,
/* 03ec */ 0x58000008L, /*		set	atn */
/* 03f0 */ 0x00000000L,
/* 03f4 */ 0x60000040L, /*		clear	ack */
/* 03f8 */ 0x00000000L,
/* 03fc */ 0x78340700L, /*		move	MSG_REJECT to scratcha */
/* 0400 */ 0x00000000L,
/* 0404 */ 0x9e030000L, /*		int	error_unexpected_phase, when not msg_out */
/* 0408 */ 0x00000004L,
/* 040c */ 0x60000008L, /*		clear	atn */
/* 0410 */ 0x00000000L,
/* 0414 */ 0x0e000001L, /*		move	1, scratcha, when msg_out */
/* 0418 */ 0x00000034L,
/* 041c */ 0x60000040L, /*		clear	ack */
/* 0420 */ 0x00000000L,
/* 0424 */ 0x868b0000L, /*		jump	reject, when msg_out */
/* 0428 */ 0x00ffffc0L,
/* 042c */ 0x80880000L, /*		jump	to_decisions */
/* 0430 */ 0x0000023cL,
/* 0434 */ 0x60000040L, /*		clear	ack */
/* 0438 */ 0x00000000L,
/* 043c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0440 */ 0x00000004L,
/* 0444 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0448 */ 0x00000035L,
/* 044c */ 0x98080000L, /*		int	SIR_MSG_IGNORE_WIDE_RESIDUE */
/* 0450 */ 0x00000010L,
/* 0454 */ 0x60000040L, /*		clear	ack */
/* 0458 */ 0x00000000L,
/* 045c */ 0x80880000L, /*		jump	to_decisions */
/* 0460 */ 0x0000020cL,
/* 0464 */ 0x58000008L, /*		set	atn */
/* 0468 */ 0x00000000L,
/* 046c */ 0x60000040L, /*		clear	ack */
/* 0470 */ 0x00000000L,
/* 0474 */ 0x9e030000L, /*		int	error_unexpected_phase, when not msg_out */
/* 0478 */ 0x00000004L,
/* 047c */ 0x1e000000L, /*		move	from msg_out_buf, when msg_out */
/* 0480 */ 0x00000001L,
/* 0484 */ 0x868b0000L, /*		jump	response_repeat, when msg_out */
/* 0488 */ 0x00fffff0L,
/* 048c */ 0x878b0000L, /*		jump	response_msg_in, when msg_in */
/* 0490 */ 0x00000010L,
/* 0494 */ 0x98080000L, /*		int	SIR_EV_RESPONSE_OK */
/* 0498 */ 0x0000000cL,
/* 049c */ 0x80880000L, /*		jump	to_decisions */
/* 04a0 */ 0x000001ccL,
/* 04a4 */ 0x0f000001L, /*		move	1, scratcha, when msg_in */
/* 04a8 */ 0x00000034L,
/* 04ac */ 0x808c0007L, /*		jump	rejected, if MSG_REJECT */
/* 04b0 */ 0x00fffdf8L,
/* 04b4 */ 0x98080000L, /*		int	SIR_EV_RESPONSE_OK */
/* 04b8 */ 0x0000000cL,
/* 04bc */ 0x80880000L, /*		jump	msg_in_not_reject */
/* 04c0 */ 0x00fffd60L,
/* 04c4 */ 0x78180500L, /*		move	5 to ctest0 */
/* 04c8 */ 0x00000000L,
/* 04cc */ 0x7c027e00L, /*		move	scntl2&0x7e to scntl2 */
/* 04d0 */ 0x00000000L,
/* 04d4 */ 0x60000040L, /*		clear 	ack */
/* 04d8 */ 0x00000000L,
/* 04dc */ 0x48000000L, /*		wait	disconnect */
/* 04e0 */ 0x00000000L,
/* 04e4 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 04e8 */ 0x000006a0L,
/* 04ec */ 0x00000034L,
/* 04f0 */ 0x78340300L, /*		move	STATE_DISCONNECTED to scratcha0 */
/* 04f4 */ 0x00000000L,
/* 04f8 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 04fc */ 0x00000034L,
/* 0500 */ 0x000006a0L,
/* 0504 */ 0x88880000L, /*		call	save_state */
/* 0508 */ 0x000001dcL,
/* 050c */ 0x74020100L, /*		move	scntl2&0x01 to sfbr */
/* 0510 */ 0x00000000L,
/* 0514 */ 0x98040000L, /*		int	SIR_NOTIFY_WSR, if not 0 */
/* 0518 */ 0x00000073L,
/* 051c */ 0x80880000L, /*		jump	issue_check */
/* 0520 */ 0x000001ecL,
/* 0524 */ 0x98080000L, /*		int	SIR_NOTIFY_RESELECTED_ON_SELECT */
/* 0528 */ 0x00000075L,
/* 052c */ 0x80880000L, /*		jump	reselected */
/* 0530 */ 0x00000010L,
/* 0534 */ 0x78180b00L, /*		move	11 to ctest0 */
/* 0538 */ 0x00000000L,
/* 053c */ 0x54000000L, /*		wait reselect sigp_set */
/* 0540 */ 0x000001c4L,
/* 0544 */ 0x78180c00L, /*		move	12 to ctest0 */
/* 0548 */ 0x00000000L,
/* 054c */ 0x60000200L, /*		clear	target */
/* 0550 */ 0x00000000L,
/* 0554 */ 0x9f030000L, /*		int	error_not_msg_in_after_reselect, when not msg_in */
/* 0558 */ 0x00000006L,
/* 055c */ 0x0f000001L, /*		move	1, scratchb, when msg_in */
/* 0560 */ 0x0000005cL,
/* 0564 */ 0x98041f80L, /*		int	error_not_identify_after_reselect, if not MSG_IDENTIFY and mask 0x1f */
/* 0568 */ 0x00000007L,
/* 056c */ 0x78180600L, /*		move	6 to ctest0 */
/* 0570 */ 0x00000000L,
/* 0574 */ 0xc0000004L, /*	 	move	memory 4, dsa_head, dsa */
/* 0578 */ 0x00000008L,
/* 057c */ 0x00000010L,
/* 0580 */ 0x78180700L, /*		move	7 to ctest0 */
/* 0584 */ 0x00000000L,
/* 0588 */ 0x72100000L, /*		move	dsa0 to sfbr */
/* 058c */ 0x00000000L,
/* 0590 */ 0x80840000L, /*		jump	find_dsa_1, if not 0 */
/* 0594 */ 0x00000030L,
/* 0598 */ 0x72110000L, /*		move	dsa1 to sfbr */
/* 059c */ 0x00000000L,
/* 05a0 */ 0x80840000L, /*		jump	find_dsa_1, if not 0 */
/* 05a4 */ 0x00000020L,
/* 05a8 */ 0x72120000L, /*		move	dsa2 to sfbr */
/* 05ac */ 0x00000000L,
/* 05b0 */ 0x80840000L, /*		jump	find_dsa_1, if not 0 */
/* 05b4 */ 0x00000010L,
/* 05b8 */ 0x72130000L, /*		move	dsa3 to sfbr */
/* 05bc */ 0x00000000L,
/* 05c0 */ 0x980c0000L, /*		int	error_reselected, if 0 */
/* 05c4 */ 0x00000003L,
/* 05c8 */ 0x78180800L, /*		move	8 to ctest0 */
/* 05cc */ 0x00000000L,
/* 05d0 */ 0x88880000L, /*		call	load_state */
/* 05d4 */ 0x000000e0L,
/* 05d8 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 05dc */ 0x000006a0L,
/* 05e0 */ 0x00000034L,
/* 05e4 */ 0x72340000L, /*		move	scratcha0 to sfbr */
/* 05e8 */ 0x00000000L,
/* 05ec */ 0x80840003L, /*		jump	find_dsa_next, if not STATE_DISCONNECTED */
/* 05f0 */ 0x00000038L,
/* 05f4 */ 0x740a0700L, /*		move	ssid & 7 to sfbr */
/* 05f8 */ 0x00000000L,
/* 05fc */ 0xc0000001L, /*		move	memory 1, targ, find_dsa_smc1 */
/* 0600 */ 0x000006a8L,
/* 0604 */ 0x00000608L,
/* 0608 */ 0x808400ffL, /*		jump	find_dsa_next, if not 255 */
/* 060c */ 0x0000001cL,
/* 0610 */ 0xc0000001L, /*		move	memory 1, lun, find_dsa_smc2 */
/* 0614 */ 0x000006acL,
/* 0618 */ 0x00000624L,
/* 061c */ 0x725c0000L, /*		move	scratchb0 to sfbr */
/* 0620 */ 0x00000000L,
/* 0624 */ 0x808cf8ffL, /*		jump	reload_sync, if 255 and mask ~7 */
/* 0628 */ 0x00000034L,
/* 062c */ 0xc0000004L, /*		move	memory 4, next, dsa */
/* 0630 */ 0x000006b4L,
/* 0634 */ 0x00000010L,
/* 0638 */ 0x80880000L, /*		jump	find_dsa_loop */
/* 063c */ 0x00ffff40L,
/* 0640 */ 0x60000008L, /*		clear	atn */
/* 0644 */ 0x00000000L,
/* 0648 */ 0x878b0000L, /*	        jump    msg_in_phase, when msg_in */
/* 064c */ 0x00fffbc4L,
/* 0650 */ 0x98080000L, /*	        int     SIR_MSG_REJECT */
/* 0654 */ 0x0000000aL,
/* 0658 */ 0x80880000L, /*	        jump    to_decisions */
/* 065c */ 0x00000010L,
/* 0660 */ 0x88880000L, /*		call	load_sync */
/* 0664 */ 0x0000014cL,
/* 0668 */ 0x60000040L, /*		clear	ack */
/* 066c */ 0x00000000L,
/* 0670 */ 0x818b0000L, /*		jump	data_in_phase, when data_in */
/* 0674 */ 0x00fff9f0L,
/* 0678 */ 0x828a0000L, /*		jump	cmd_phase, if cmd */
/* 067c */ 0x00fff9d0L,
/* 0680 */ 0x808a0000L, /*		jump	data_out_phase, if data_out */
/* 0684 */ 0x00fffabcL,
/* 0688 */ 0x838b0000L, /*		jump	status_phase, when status */
/* 068c */ 0x00fffb74L,
/* 0690 */ 0x878a0000L, /*		jump	msg_in_phase, if msg_in */
/* 0694 */ 0x00fffb7cL,
/* 0698 */ 0x98080000L, /*		int	error_unexpected_phase */
/* 069c */ 0x00000004L,
/* 06a0 */ 0x00000000L, /*	state:	defw	0 */
/* 06a4 */ 0x00000000L, /*	dmaaddr: defw	0 */
/* 06a8 */ 0x00000000L, /*	targ:	defw	0 */
/* 06ac */ 0x00000000L, /*	lun:	defw	0 */
/* 06b0 */ 0x00000000L, /*	sync:	defw	0 */
/* 06b4 */ 0x00000000L, /*	next:	defw	0 */
			/*	dsa_load_len = dsa_load_end - dsa_copy */
			/*	dsa_save_len = dsa_save_end - dsa_copy */
/* 06b8 */ 0x78180900L, /*		move	9 to ctest0 */
/* 06bc */ 0x00000000L,
/* 06c0 */ 0xc0000004L, /*		move	memory 4, dsa, load_state_smc0 + 4 */
/* 06c4 */ 0x00000010L,
/* 06c8 */ 0x000006d0L,
/* 06cc */ 0xc0000018L, /*		move	memory dsa_load_len, 0, dsa_copy */
/* 06d0 */ 0x00000000L,
/* 06d4 */ 0x000006a0L,
/* 06d8 */ 0x78181400L, /*		move	20 to ctest0 */
/* 06dc */ 0x00000000L,
/* 06e0 */ 0x90080000L, /*		return */
/* 06e4 */ 0x00000000L,
/* 06e8 */ 0xc0000004L, /*		move	memory 4, dsa, save_state_smc0 + 8 */
/* 06ec */ 0x00000010L,
/* 06f0 */ 0x000006fcL,
/* 06f4 */ 0xc0000008L, /*		move	memory dsa_save_len, dsa_copy, 0 */
/* 06f8 */ 0x000006a0L,
/* 06fc */ 0x00000000L,
/* 0700 */ 0x90080000L, /*		return */
/* 0704 */ 0x00000000L,
/* 0708 */ 0x721a0000L, /*		move	ctest2 to sfbr */
/* 070c */ 0x00000000L,
/* 0710 */ 0x78180100L, /*		move	1 to ctest0 */
/* 0714 */ 0x00000000L,
/* 0718 */ 0xc0000004L, /*		move	memory 4, dsa_head, dsa */
/* 071c */ 0x00000008L,
/* 0720 */ 0x00000010L,
/* 0724 */ 0x78180200L, /*		move	2 to ctest0 */
/* 0728 */ 0x00000000L,
/* 072c */ 0x72100000L, /*		move	dsa0 to sfbr */
/* 0730 */ 0x00000000L,
/* 0734 */ 0x80840000L, /*		jump	issue_check_1, if not 0 */
/* 0738 */ 0x00000030L,
/* 073c */ 0x72110000L, /*		move	dsa1 to sfbr */
/* 0740 */ 0x00000000L,
/* 0744 */ 0x80840000L, /*		jump	issue_check_1, if not 0 */
/* 0748 */ 0x00000020L,
/* 074c */ 0x72120000L, /*		move	dsa2 to sfbr */
/* 0750 */ 0x00000000L,
/* 0754 */ 0x80840000L, /*		jump	issue_check_1, if not 0 */
/* 0758 */ 0x00000010L,
/* 075c */ 0x72130000L, /*		move	dsa3 to sfbr */
/* 0760 */ 0x00000000L,
/* 0764 */ 0x808c0000L, /*		jump	wait_for_reselection, if 0 */
/* 0768 */ 0x00fffdc8L,
/* 076c */ 0x78180300L, /*		move	3 to ctest0 */
/* 0770 */ 0x00000000L,
/* 0774 */ 0x88880000L, /*	 	call	load_state */
/* 0778 */ 0x00ffff3cL,
/* 077c */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 0780 */ 0x000006a0L,
/* 0784 */ 0x00000034L,
/* 0788 */ 0x72340000L, /*		move	scratcha0 to sfbr */
/* 078c */ 0x00000000L,
/* 0790 */ 0x808c0002L, /*		jump	start, if STATE_ISSUE */
/* 0794 */ 0x00fff870L,
/* 0798 */ 0x78180400L, /*		move	4 to ctest0 */
/* 079c */ 0x00000000L,
/* 07a0 */ 0xc0000004L, /*		move	memory 4, next, dsa */
/* 07a4 */ 0x000006b4L,
/* 07a8 */ 0x00000010L,
/* 07ac */ 0x80880000L, /*		jump	issue_check_loop */
/* 07b0 */ 0x00ffff70L,
/* 07b4 */ 0xc0000004L, /*		move	memory 4, sync, scratcha */
/* 07b8 */ 0x000006b0L,
/* 07bc */ 0x00000034L,
/* 07c0 */ 0x72340000L, /*		move	scratcha0 to sfbr */
/* 07c4 */ 0x00000000L,
/* 07c8 */ 0x6a030000L, /*		move	sfbr to scntl3 */
/* 07cc */ 0x00000000L,
/* 07d0 */ 0x72350000L, /*		move	scratcha1 to sfbr */
/* 07d4 */ 0x00000000L,
/* 07d8 */ 0x6a050000L, /*		move	sfbr to sxfer */
/* 07dc */ 0x00000000L,
/* 07e0 */ 0x90080000L, /*		return */
/* 07e4 */ 0x00000000L,
};

#define NA_SCRIPT_SIZE 506

struct na_patch na_patches[] = {
	{ 0x0008, 5 }, /* 00000020 */
	{ 0x000f, 4 }, /* 0000003c */
	{ 0x0017, 4 }, /* 0000005c */
	{ 0x001b, 1 }, /* 0000006c */
	{ 0x001c, 2 }, /* 00000070 */
	{ 0x001e, 1 }, /* 00000078 */
	{ 0x001f, 2 }, /* 0000007c */
	{ 0x0025, 1 }, /* 00000094 */
	{ 0x002f, 2 }, /* 000000bc */
	{ 0x0030, 1 }, /* 000000c0 */
	{ 0x0034, 2 }, /* 000000d0 */
	{ 0x0035, 1 }, /* 000000d4 */
	{ 0x003b, 2 }, /* 000000ec */
	{ 0x003c, 1 }, /* 000000f0 */
	{ 0x003e, 2 }, /* 000000f8 */
	{ 0x003f, 1 }, /* 000000fc */
	{ 0x0047, 4 }, /* 0000011c */
	{ 0x004b, 2 }, /* 0000012c */
	{ 0x004c, 1 }, /* 00000130 */
	{ 0x0052, 1 }, /* 00000148 */
	{ 0x0053, 2 }, /* 0000014c */
	{ 0x0055, 1 }, /* 00000154 */
	{ 0x0056, 2 }, /* 00000158 */
	{ 0x005c, 1 }, /* 00000170 */
	{ 0x005d, 2 }, /* 00000174 */
	{ 0x005f, 1 }, /* 0000017c */
	{ 0x0069, 2 }, /* 000001a4 */
	{ 0x006a, 1 }, /* 000001a8 */
	{ 0x006e, 2 }, /* 000001b8 */
	{ 0x006f, 1 }, /* 000001bc */
	{ 0x0077, 4 }, /* 000001dc */
	{ 0x007b, 2 }, /* 000001ec */
	{ 0x007c, 1 }, /* 000001f0 */
	{ 0x0082, 4 }, /* 00000208 */
	{ 0x0086, 2 }, /* 00000218 */
	{ 0x009c, 1 }, /* 00000270 */
	{ 0x009d, 2 }, /* 00000274 */
	{ 0x00a3, 2 }, /* 0000028c */
	{ 0x00a4, 1 }, /* 00000290 */
	{ 0x00ba, 2 }, /* 000002e8 */
	{ 0x00c6, 2 }, /* 00000318 */
	{ 0x00ce, 2 }, /* 00000338 */
	{ 0x00d4, 2 }, /* 00000350 */
	{ 0x00da, 2 }, /* 00000368 */
	{ 0x00ea, 2 }, /* 000003a8 */
	{ 0x00f0, 2 }, /* 000003c0 */
	{ 0x0106, 2 }, /* 00000418 */
	{ 0x0112, 2 }, /* 00000448 */
	{ 0x0120, 4 }, /* 00000480 */
	{ 0x012a, 2 }, /* 000004a8 */
	{ 0x013a, 1 }, /* 000004e8 */
	{ 0x013b, 2 }, /* 000004ec */
	{ 0x013f, 2 }, /* 000004fc */
	{ 0x0140, 1 }, /* 00000500 */
	{ 0x0158, 2 }, /* 00000560 */
	{ 0x015e, 4 }, /* 00000578 */
	{ 0x015f, 2 }, /* 0000057c */
	{ 0x0177, 1 }, /* 000005dc */
	{ 0x0178, 2 }, /* 000005e0 */
	{ 0x0180, 1 }, /* 00000600 */
	{ 0x0181, 1 }, /* 00000604 */
	{ 0x0185, 1 }, /* 00000614 */
	{ 0x0186, 1 }, /* 00000618 */
	{ 0x018c, 1 }, /* 00000630 */
	{ 0x018d, 2 }, /* 00000634 */
	{ 0x01b1, 2 }, /* 000006c4 */
	{ 0x01b2, 1 }, /* 000006c8 */
	{ 0x01b5, 1 }, /* 000006d4 */
	{ 0x01bb, 2 }, /* 000006ec */
	{ 0x01bc, 1 }, /* 000006f0 */
	{ 0x01be, 1 }, /* 000006f8 */
	{ 0x01c7, 4 }, /* 0000071c */
	{ 0x01c8, 2 }, /* 00000720 */
	{ 0x01e0, 1 }, /* 00000780 */
	{ 0x01e1, 2 }, /* 00000784 */
	{ 0x01e9, 1 }, /* 000007a4 */
	{ 0x01ea, 2 }, /* 000007a8 */
	{ 0x01ee, 1 }, /* 000007b8 */
	{ 0x01ef, 2 }, /* 000007bc */
};
#define NA_PATCHES 79

enum na_external {
	X_scsi_id_buf,
	X_msg_out_buf,
	X_cmd_buf,
	X_data_buf,
	X_status_buf,
	X_msgin_buf,
	X_dsa_0,
	X_dsa_1,
	X_dsa_head,
};

enum {
	E_issue_check_next = 1944,
	E_issue_check_1 = 1900,
	E_issue_check_loop = 1828,
	E_save_state_smc0 = 1780,
	E_load_state_smc0 = 1740,
	E_dsa_load_end = 1720,
	E_sync = 1712,
	E_dsa_save_end = 1704,
	E_dsa_copy = 1696,
	E_id_out_mismatch_recover = 1600,
	E_next = 1716,
	E_reload_sync = 1632,
	E_find_dsa_smc2 = 1572,
	E_lun = 1708,
	E_find_dsa_smc1 = 1544,
	E_targ = 1704,
	E_find_dsa_next = 1580,
	E_load_state = 1720,
	E_find_dsa_1 = 1480,
	E_find_dsa_loop = 1408,
	E_find_dsa = 1388,
	E_sigp_set = 1800,
	E_reselected = 1348,
	E_wsr_check = 1292,
	E_response_msg_in = 1188,
	E_response_repeat = 1148,
	E_response = 1124,
	E_reject = 1004,
	E_wdtr = 980,
	E_sdtr = 892,
	E_ext_done = 1004,
	E_ext_1 = 772,
	E_ext_2 = 916,
	E_ext_3 = 804,
	E_issue_check = 1808,
	E_extended = 724,
	E_ignore_wide = 1076,
	E_msg_in_skip = 708,
	E_disconnected = 1220,
	E_msg_in_not_reject = 548,
	E_rejected = 684,
	E_msg_in_phase = 532,
	E_status_phase = 516,
	E_data_out_mismatch = 480,
	E_data_out_block_mismatch = 384,
	E_data_out_normal = 456,
	E_data_out_block_loop = 348,
	E_data_out_phase = 324,
	E_post_data_to_decisions = 1672,
	E_data_in_mismatch = 288,
	E_data_block_mismatch_recover = 232,
	E_save_state = 1768,
	E_data_in_block_mismatch = 152,
	E_data_in_normal = 264,
	E_data_in_block_loop = 128,
	E_dmaaddr = 1700,
	E_state = 1696,
	E_data_in_phase = 104,
	E_cmd_phase = 80,
	E_to_decisions = 1648,
	E_id_out_mismatch = 64,
	E_start1 = 48,
	E_reselected_on_select = 1316,
	E_load_sync = 1972,
	E_start = 8,
	E_wait_for_reselection = 1332,
	E_idle = 0,
};
#define A_dsa_save_len 8
#define A_dsa_load_len 24
#define A_BSIZE 512
#define A_MSG_REJECT 7
#define A_X_MSG_WDTR 3
#define A_X_MSG_SDTR 1
#define A_X_MSG 1
#define A_MSG_IGNORE_WIDE_RESIDUE 35
#define A_MSG_RESTORE_POINTERS 3
#define A_MSG_SAVE_DATA_POINTER 2
#define A_MSG_DISCONNECT 4
#define A_MSG_IDENTIFY 128
#define A_RESULT_OK 0
#define A_STATE_DONE 4
#define A_STATE_DISCONNECTED 3
#define A_STATE_ISSUE 2
#define A_STATE_ALLOCATED 1
#define A_STATE_FREE 0
#define A_SIR_NOTIFY_RESELECTED_ON_SELECT 117
#define A_SIR_NOTIFY_LOAD_SYNC 116
#define A_SIR_NOTIFY_WSR 115
#define A_SIR_NOTIFY_BLOCK_DATA_IN 114
#define A_SIR_NOTIFY_DATA_OUT 113
#define A_SIR_NOTIFY_DATA_IN 112
#define A_SIR_NOTIFY_COMMAND 111
#define A_SIR_NOTIFY_DUMP_NEXT_CODE 110
#define A_SIR_NOTIFY_ISSUE_CHECK 109
#define A_SIR_NOTIFY_WAIT_RESELECT 108
#define A_SIR_NOTIFY_ISSUE 107
#define A_SIR_NOTIFY_SIGP 106
#define A_SIR_NOTIFY_DUMP2 105
#define A_SIR_NOTIFY_DUMP 104
#define A_SIR_NOTIFY_STATUS 103
#define A_SIR_NOTIFY_MSG_IN 102
#define A_SIR_NOTIFY_RESELECT 101
#define A_SIR_NOTIFY_DISC 100
#define A_SIR_MSG_IGNORE_WIDE_RESIDUE 16
#define A_SIR_MSG_WDTR 15
#define A_SIR_EV_PHASE_SWITCH_AFTER_ID 14
#define A_error_sigp_set 13
#define A_SIR_EV_RESPONSE_OK 12
#define A_SIR_MSG_SDTR 11
#define A_SIR_MSG_REJECT 10
#define A_error_too_little_data 9
#define A_error_too_much_data 8
#define A_error_not_identify_after_reselect 7
#define A_error_not_msg_in_after_reselect 6
#define A_error_weird_message 5
#define A_error_unexpected_phase 4
#define A_error_reselected 3
#define A_error_disconnected 2
#define A_error_not_cmd_complete 1
#define A_SIR_MSG_IO_COMPLETE 0
