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
			/*	extern	ssid_mask */
			/*	SIR_MSG_IO_COMPLETE = 0 */
			/*	error_not_cmd_complete = 1 */
			/*	error_disconnected = 2 */
			/*	error_reselected = 3 */
			/*	error_unexpected_phase = 4 */
			/*	error_weird_message = 5 */
			/*	SIR_ERROR_NOT_MSG_IN_AFTER_RESELECT = 6 */
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
/* 0004 */ 0x00000514L,
/* 0008 */ 0x88880000L, /*		call	load_sync */
/* 000c */ 0x0000074cL,
/* 0010 */ 0x60000200L, /*		clear	target */
/* 0014 */ 0x00000000L,
/* 0018 */ 0x47000000L, /*		select	atn from scsi_id_buf, reselected_on_select */
/* 001c */ 0x000004ecL,
/* 0020 */ 0x878b0000L, /*		jump	start1, when msg_in */
/* 0024 */ 0x00000000L,
/* 0028 */ 0x1e000000L, /*		move	from msg_out_buf, when msg_out */
/* 002c */ 0x00000001L,
/* 0030 */ 0x868b0000L, /*		jump	start1, when msg_out */
/* 0034 */ 0x00fffff0L,
/* 0038 */ 0x82830000L, /*		jump	to_decisions, when not cmd */
/* 003c */ 0x000005f0L,
/* 0040 */ 0x60000008L, /*		clear	atn */
/* 0044 */ 0x00000000L,
/* 0048 */ 0x1a000000L, /*		move	from cmd_buf, when cmd */
/* 004c */ 0x00000002L,
/* 0050 */ 0x81830000L, /*		jump	to_decisions, when not data_in */
/* 0054 */ 0x000005d8L,
/* 0058 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 005c */ 0x00000678L,
/* 0060 */ 0x00000034L,
/* 0064 */ 0xc0000004L, /*		move	memory 4, dmaaddr, scratchb */
/* 0068 */ 0x0000067cL,
/* 006c */ 0x0000005cL,
/* 0070 */ 0x72360000L, /*		move	scratcha2 to sfbr */
/* 0074 */ 0x00000000L,
/* 0078 */ 0x808c0000L, /*		jump	data_in_normal, if 0 */
/* 007c */ 0x00000078L,
/* 0080 */ 0x29000200L, /*		move	BSIZE, ptr dmaaddr, when data_in */
/* 0084 */ 0x0000067cL,
/* 0088 */ 0x7e5d0200L, /*		move	scratchb1 + BSIZE / 256 to scratchb1 */
/* 008c */ 0x00000000L,
/* 0090 */ 0x7f5e0000L, /*		move	scratchb2 + 0 to scratchb2 with carry */
/* 0094 */ 0x00000000L,
/* 0098 */ 0x7f5f0000L, /*		move	scratchb3 + 0 to scratchb3 with carry */
/* 009c */ 0x00000000L,
/* 00a0 */ 0x7e36ff00L, /*		move	scratcha2 + 255 to scratcha2 */
/* 00a4 */ 0x00000000L,
/* 00a8 */ 0xc0000004L, /*		move	memory 4, scratchb, dmaaddr */
/* 00ac */ 0x0000005cL,
/* 00b0 */ 0x0000067cL,
/* 00b4 */ 0x818b0000L, /*		jump	data_in_block_loop, when data_in */
/* 00b8 */ 0x00ffffb4L,
/* 00bc */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 00c0 */ 0x00000034L,
/* 00c4 */ 0x00000678L,
/* 00c8 */ 0x88880000L, /*		call	save_state */
/* 00cc */ 0x000005e0L,
/* 00d0 */ 0x80880000L, /*		jump	to_decisions */
/* 00d4 */ 0x00000558L,
/* 00d8 */ 0xc0000004L, /*		move	memory 4, scratchb, dmaaddr */
/* 00dc */ 0x0000005cL,
/* 00e0 */ 0x0000067cL,
/* 00e4 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 00e8 */ 0x00000034L,
/* 00ec */ 0x00000678L,
/* 00f0 */ 0x80880000L, /*		jump	to_decisions */
/* 00f4 */ 0x00000538L,
/* 00f8 */ 0x72370000L, /*		move	scratcha3 to sfbr */
/* 00fc */ 0x00000000L,
/* 0100 */ 0x98040000L, /*		int	error_too_much_data, if not 0 */
/* 0104 */ 0x00000008L,
/* 0108 */ 0x19000000L, /*		move	from data_buf, when data_in */
/* 010c */ 0x00000003L,
/* 0110 */ 0x78370200L, /*		move	2 to scratcha3 */
/* 0114 */ 0x00000000L,
/* 0118 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 011c */ 0x00000034L,
/* 0120 */ 0x00000678L,
/* 0124 */ 0x88880000L, /*		call	save_state */
/* 0128 */ 0x00000584L,
/* 012c */ 0x80880000L, /*		jump	post_data_to_decisions */
/* 0130 */ 0x0000052cL,
/* 0134 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 0138 */ 0x00000678L,
/* 013c */ 0x00000034L,
/* 0140 */ 0xc0000004L, /*		move	memory 4, dmaaddr, scratchb */
/* 0144 */ 0x0000067cL,
/* 0148 */ 0x0000005cL,
/* 014c */ 0x72360000L, /*		move	scratcha2 to sfbr */
/* 0150 */ 0x00000000L,
/* 0154 */ 0x808c0000L, /*		jump	data_out_normal, if 0 */
/* 0158 */ 0x0000005cL,
/* 015c */ 0xc0000004L, /*		move	memory 4, dmaaddr, scratchb */
/* 0160 */ 0x0000067cL,
/* 0164 */ 0x0000005cL,
/* 0168 */ 0x28000200L, /*		move	BSIZE, ptr dmaaddr, when data_out */
/* 016c */ 0x0000067cL,
/* 0170 */ 0x7e5d0200L, /*		move	scratchb1 + BSIZE / 256 to scratchb1 */
/* 0174 */ 0x00000000L,
/* 0178 */ 0x7f5e0000L, /*		move	scratchb2 + 0 to scratchb2 with carry */
/* 017c */ 0x00000000L,
/* 0180 */ 0x7f5f0000L, /*		move	scratchb3 + 0 to scratchb3 with carry */
/* 0184 */ 0x00000000L,
/* 0188 */ 0x7e36ff00L, /*		move	scratcha2 + 255 to scratcha2 */
/* 018c */ 0x00000000L,
/* 0190 */ 0xc0000004L, /*		move	memory 4, scratchb, dmaaddr */
/* 0194 */ 0x0000005cL,
/* 0198 */ 0x0000067cL,
/* 019c */ 0x808b0000L, /*		jump	data_out_block_loop, when data_out */
/* 01a0 */ 0x00ffffa8L,
/* 01a4 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 01a8 */ 0x00000034L,
/* 01ac */ 0x00000678L,
/* 01b0 */ 0x80880000L, /*		jump	to_decisions */
/* 01b4 */ 0x00000478L,
/* 01b8 */ 0x72370000L, /*		move	scratcha3 to sfbr */
/* 01bc */ 0x00000000L,
/* 01c0 */ 0x98040000L, /*		int	error_too_little_data, if not 0 */
/* 01c4 */ 0x00000009L,
/* 01c8 */ 0x18000000L, /*		move	from data_buf, when data_out */
/* 01cc */ 0x00000003L,
/* 01d0 */ 0x78370200L, /*		move	2 to scratcha3 */
/* 01d4 */ 0x00000000L,
/* 01d8 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 01dc */ 0x00000034L,
/* 01e0 */ 0x00000678L,
/* 01e4 */ 0x88880000L, /*		call	save_state */
/* 01e8 */ 0x000004c4L,
/* 01ec */ 0x80880000L, /*		jump	post_data_to_decisions */
/* 01f0 */ 0x0000046cL,
/* 01f4 */ 0x1b000000L, /*		move	from status_buf, when status */
/* 01f8 */ 0x00000004L,
/* 01fc */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0200 */ 0x00000004L,
/* 0204 */ 0x0f000001L, /*		move	1, scratcha, when msg_in */
/* 0208 */ 0x00000034L,
/* 020c */ 0x808c0007L, /*		jump	rejected, if MSG_REJECT */
/* 0210 */ 0x00000088L,
/* 0214 */ 0x808c0004L, /*		jump	disconnected, if MSG_DISCONNECT */
/* 0218 */ 0x00000298L,
/* 021c */ 0x808c0002L, /*		jump	msg_in_skip, if MSG_SAVE_DATA_POINTER */
/* 0220 */ 0x00000090L,
/* 0224 */ 0x808c0003L, /*		jump	msg_in_skip, if MSG_RESTORE_POINTERS */
/* 0228 */ 0x00000088L,
/* 022c */ 0x808c0023L, /*		jump	ignore_wide, if MSG_IGNORE_WIDE_RESIDUE */
/* 0230 */ 0x000001f0L,
/* 0234 */ 0x808c0001L, /*		jump	extended, if X_MSG */
/* 0238 */ 0x00000088L,
/* 023c */ 0x98040000L, /*		int	error_not_cmd_complete, if not 0 */
/* 0240 */ 0x00000001L,
/* 0244 */ 0x7c027e00L, /*		move	scntl2&0x7e to scntl2 */
/* 0248 */ 0x00000000L,
/* 024c */ 0x60000040L, /*		clear	ack */
/* 0250 */ 0x00000000L,
/* 0254 */ 0x48000000L, /*		wait	disconnect */
/* 0258 */ 0x00000000L,
/* 025c */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 0260 */ 0x00000678L,
/* 0264 */ 0x00000034L,
/* 0268 */ 0x78340400L, /*		move	STATE_DONE to scratcha0 */
/* 026c */ 0x00000000L,
/* 0270 */ 0x78350000L, /*		move	RESULT_OK to scratcha1 */
/* 0274 */ 0x00000000L,
/* 0278 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 027c */ 0x00000034L,
/* 0280 */ 0x00000678L,
/* 0284 */ 0x88880000L, /*		call	save_state */
/* 0288 */ 0x00000424L,
/* 028c */ 0x98180000L, /*		intfly	0 */
/* 0290 */ 0x00000000L,
/* 0294 */ 0x80880000L, /*		jump	issue_check */
/* 0298 */ 0x0000043cL,
/* 029c */ 0x98080000L, /*		int	SIR_MSG_REJECT */
/* 02a0 */ 0x0000000aL,
/* 02a4 */ 0x60000040L, /*		clear	ack */
/* 02a8 */ 0x00000000L,
/* 02ac */ 0x80880000L, /*		jump	to_decisions */
/* 02b0 */ 0x0000037cL,
/* 02b4 */ 0x60000040L, /*		clear	ack */
/* 02b8 */ 0x00000000L,
/* 02bc */ 0x80880000L, /*		jump	to_decisions */
/* 02c0 */ 0x0000036cL,
/* 02c4 */ 0x60000040L, /*		clear	ack */
/* 02c8 */ 0x00000000L,
/* 02cc */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 02d0 */ 0x00000004L,
/* 02d4 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 02d8 */ 0x00000035L,
/* 02dc */ 0x808c0003L, /*		jump	ext_3, if 3 */
/* 02e0 */ 0x00000030L,
/* 02e4 */ 0x808c0002L, /*		jump	ext_2, if 2 */
/* 02e8 */ 0x00000098L,
/* 02ec */ 0x98040001L, /*		int	error_weird_message, if not 1 */
/* 02f0 */ 0x00000005L,
/* 02f4 */ 0x60000040L, /*		clear	ack */
/* 02f8 */ 0x00000000L,
/* 02fc */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0300 */ 0x00000004L,
/* 0304 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0308 */ 0x00000035L,
/* 030c */ 0x80880000L, /*		jump	ext_done */
/* 0310 */ 0x000000c8L,
/* 0314 */ 0x60000040L, /*	ext_3:	clear	ack */
/* 0318 */ 0x00000000L,
/* 031c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0320 */ 0x00000004L,
/* 0324 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0328 */ 0x00000035L,
/* 032c */ 0x60000040L, /*		clear	ack */
/* 0330 */ 0x00000000L,
/* 0334 */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0338 */ 0x00000004L,
/* 033c */ 0x0f000001L, /*		move	1, scratcha2, when msg_in */
/* 0340 */ 0x00000036L,
/* 0344 */ 0x60000040L, /*		clear	ack */
/* 0348 */ 0x00000000L,
/* 034c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0350 */ 0x00000004L,
/* 0354 */ 0x0f000001L, /*		move	1, scratcha3, when msg_in */
/* 0358 */ 0x00000037L,
/* 035c */ 0x72350000L, /*		move	scratcha1 to sfbr */
/* 0360 */ 0x00000000L,
/* 0364 */ 0x80840001L, /*		jump	ext_done, if not X_MSG_SDTR */
/* 0368 */ 0x00000070L,
/* 036c */ 0x98080000L, /*	sdtr:	int	SIR_MSG_SDTR */
/* 0370 */ 0x0000000bL,
/* 0374 */ 0x60000040L, /*		clear	ack */
/* 0378 */ 0x00000000L,
/* 037c */ 0x80880000L, /*		jump	to_decisions */
/* 0380 */ 0x000002acL,
/* 0384 */ 0x60000040L, /*	ext_2:	clear	ack */
/* 0388 */ 0x00000000L,
/* 038c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0390 */ 0x00000004L,
/* 0394 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0398 */ 0x00000035L,
/* 039c */ 0x60000040L, /*		clear	ack */
/* 03a0 */ 0x00000000L,
/* 03a4 */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 03a8 */ 0x00000004L,
/* 03ac */ 0x0f000001L, /*		move	1, scratcha2, when msg_in */
/* 03b0 */ 0x00000036L,
/* 03b4 */ 0x72350000L, /*		move	scratcha1 to sfbr */
/* 03b8 */ 0x00000000L,
/* 03bc */ 0x80840003L, /*		jump	ext_done, if not X_MSG_WDTR */
/* 03c0 */ 0x00000018L,
/* 03c4 */ 0x98080000L, /*	wdtr:	int	SIR_MSG_WDTR */
/* 03c8 */ 0x0000000fL,
/* 03cc */ 0x60000040L, /*		clear	ack */
/* 03d0 */ 0x00000000L,
/* 03d4 */ 0x80880000L, /*		jump	to_decisions */
/* 03d8 */ 0x00000254L,
/* 03dc */ 0x58000008L, /*		set	atn */
/* 03e0 */ 0x00000000L,
/* 03e4 */ 0x60000040L, /*		clear	ack */
/* 03e8 */ 0x00000000L,
/* 03ec */ 0x78340700L, /*		move	MSG_REJECT to scratcha */
/* 03f0 */ 0x00000000L,
/* 03f4 */ 0x9e030000L, /*		int	error_unexpected_phase, when not msg_out */
/* 03f8 */ 0x00000004L,
/* 03fc */ 0x60000008L, /*		clear	atn */
/* 0400 */ 0x00000000L,
/* 0404 */ 0x0e000001L, /*		move	1, scratcha, when msg_out */
/* 0408 */ 0x00000034L,
/* 040c */ 0x60000040L, /*		clear	ack */
/* 0410 */ 0x00000000L,
/* 0414 */ 0x868b0000L, /*		jump	reject, when msg_out */
/* 0418 */ 0x00ffffc0L,
/* 041c */ 0x80880000L, /*		jump	to_decisions */
/* 0420 */ 0x0000020cL,
/* 0424 */ 0x60000040L, /*		clear	ack */
/* 0428 */ 0x00000000L,
/* 042c */ 0x9f030000L, /*		int	error_unexpected_phase, when not msg_in */
/* 0430 */ 0x00000004L,
/* 0434 */ 0x0f000001L, /*		move	1, scratcha1, when msg_in */
/* 0438 */ 0x00000035L,
/* 043c */ 0x98080000L, /*		int	SIR_MSG_IGNORE_WIDE_RESIDUE */
/* 0440 */ 0x00000010L,
/* 0444 */ 0x60000040L, /*		clear	ack */
/* 0448 */ 0x00000000L,
/* 044c */ 0x80880000L, /*		jump	to_decisions */
/* 0450 */ 0x000001dcL,
/* 0454 */ 0x58000008L, /*		set	atn */
/* 0458 */ 0x00000000L,
/* 045c */ 0x60000040L, /*		clear	ack */
/* 0460 */ 0x00000000L,
/* 0464 */ 0x9e030000L, /*		int	error_unexpected_phase, when not msg_out */
/* 0468 */ 0x00000004L,
/* 046c */ 0x1e000000L, /*		move	from msg_out_buf, when msg_out */
/* 0470 */ 0x00000001L,
/* 0474 */ 0x868b0000L, /*		jump	response_repeat, when msg_out */
/* 0478 */ 0x00fffff0L,
/* 047c */ 0x878b0000L, /*		jump	response_msg_in, when msg_in */
/* 0480 */ 0x00000010L,
/* 0484 */ 0x98080000L, /*		int	SIR_EV_RESPONSE_OK */
/* 0488 */ 0x0000000cL,
/* 048c */ 0x80880000L, /*		jump	to_decisions */
/* 0490 */ 0x0000019cL,
/* 0494 */ 0x0f000001L, /*		move	1, scratcha, when msg_in */
/* 0498 */ 0x00000034L,
/* 049c */ 0x808c0007L, /*		jump	rejected, if MSG_REJECT */
/* 04a0 */ 0x00fffdf8L,
/* 04a4 */ 0x98080000L, /*		int	SIR_EV_RESPONSE_OK */
/* 04a8 */ 0x0000000cL,
/* 04ac */ 0x80880000L, /*		jump	msg_in_not_reject */
/* 04b0 */ 0x00fffd60L,
/* 04b4 */ 0x7c027e00L, /*		move	scntl2&0x7e to scntl2 */
/* 04b8 */ 0x00000000L,
/* 04bc */ 0x60000040L, /*		clear 	ack */
/* 04c0 */ 0x00000000L,
/* 04c4 */ 0x48000000L, /*		wait	disconnect */
/* 04c8 */ 0x00000000L,
/* 04cc */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 04d0 */ 0x00000678L,
/* 04d4 */ 0x00000034L,
/* 04d8 */ 0x78340300L, /*		move	STATE_DISCONNECTED to scratcha0 */
/* 04dc */ 0x00000000L,
/* 04e0 */ 0xc0000004L, /*		move	memory 4, scratcha, state */
/* 04e4 */ 0x00000034L,
/* 04e8 */ 0x00000678L,
/* 04ec */ 0x88880000L, /*		call	save_state */
/* 04f0 */ 0x000001bcL,
/* 04f4 */ 0x74020100L, /*		move	scntl2&0x01 to sfbr */
/* 04f8 */ 0x00000000L,
/* 04fc */ 0x98040000L, /*		int	SIR_NOTIFY_WSR, if not 0 */
/* 0500 */ 0x00000073L,
/* 0504 */ 0x80880000L, /*		jump	issue_check */
/* 0508 */ 0x000001ccL,
/* 050c */ 0x98080000L, /*		int	SIR_NOTIFY_RESELECTED_ON_SELECT */
/* 0510 */ 0x00000075L,
/* 0514 */ 0x80880000L, /*		jump	reselected */
/* 0518 */ 0x00000008L,
/* 051c */ 0x54000000L, /*		wait reselect sigp_set */
/* 0520 */ 0x000001acL,
/* 0524 */ 0x60000200L, /*		clear	target */
/* 0528 */ 0x00000000L,
/* 052c */ 0x9f030000L, /*		int	SIR_ERROR_NOT_MSG_IN_AFTER_RESELECT, when not msg_in */
/* 0530 */ 0x00000006L,
/* 0534 */ 0x0f000001L, /*		move	1, scratchb, when msg_in */
/* 0538 */ 0x0000005cL,
/* 053c */ 0x98041f80L, /*		int	error_not_identify_after_reselect, if not MSG_IDENTIFY and mask 0x1f */
/* 0540 */ 0x00000007L,
/* 0544 */ 0xc0000004L, /*	 	move	memory 4, dsa_head, dsa */
/* 0548 */ 0x00000008L,
/* 054c */ 0x00000010L,
/* 0550 */ 0x72100000L, /*		move	dsa0 to sfbr */
/* 0554 */ 0x00000000L,
/* 0558 */ 0x80840000L, /*		jump	find_dsa_1, if not 0 */
/* 055c */ 0x00000030L,
/* 0560 */ 0x72110000L, /*		move	dsa1 to sfbr */
/* 0564 */ 0x00000000L,
/* 0568 */ 0x80840000L, /*		jump	find_dsa_1, if not 0 */
/* 056c */ 0x00000020L,
/* 0570 */ 0x72120000L, /*		move	dsa2 to sfbr */
/* 0574 */ 0x00000000L,
/* 0578 */ 0x80840000L, /*		jump	find_dsa_1, if not 0 */
/* 057c */ 0x00000010L,
/* 0580 */ 0x72130000L, /*		move	dsa3 to sfbr */
/* 0584 */ 0x00000000L,
/* 0588 */ 0x980c0000L, /*		int	error_reselected, if 0 */
/* 058c */ 0x00000003L,
/* 0590 */ 0x88880000L, /*		call	load_state */
/* 0594 */ 0x000000f8L,
/* 0598 */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 059c */ 0x00000678L,
/* 05a0 */ 0x00000034L,
/* 05a4 */ 0x72340000L, /*		move	scratcha0 to sfbr */
/* 05a8 */ 0x00000000L,
/* 05ac */ 0x80840003L, /*		jump	find_dsa_next, if not STATE_DISCONNECTED */
/* 05b0 */ 0x00000038L,
/* 05b4 */ 0x740a0900L, /*		move	ssid & ssid_mask to sfbr */
/* 05b8 */ 0x00000000L,
/* 05bc */ 0xc0000001L, /*		move	memory 1, targ, find_dsa_smc1 */
/* 05c0 */ 0x00000680L,
/* 05c4 */ 0x000005c8L,
/* 05c8 */ 0x808400ffL, /*		jump	find_dsa_next, if not 255 */
/* 05cc */ 0x0000001cL,
/* 05d0 */ 0xc0000001L, /*		move	memory 1, lun, find_dsa_smc2 */
/* 05d4 */ 0x00000684L,
/* 05d8 */ 0x000005e4L,
/* 05dc */ 0x725c0000L, /*		move	scratchb0 to sfbr */
/* 05e0 */ 0x00000000L,
/* 05e4 */ 0x808cf8ffL, /*		jump	reload_sync, if 255 and mask ~7 */
/* 05e8 */ 0x00000034L,
/* 05ec */ 0xc0000004L, /*		move	memory 4, next, dsa */
/* 05f0 */ 0x0000068cL,
/* 05f4 */ 0x00000010L,
/* 05f8 */ 0x80880000L, /*		jump	find_dsa_loop */
/* 05fc */ 0x00ffff50L,
/* 0600 */ 0x60000008L, /*		clear	atn */
/* 0604 */ 0x00000000L,
/* 0608 */ 0x878b0000L, /*	        jump    msg_in_phase, when msg_in */
/* 060c */ 0x00fffbf4L,
/* 0610 */ 0x98080000L, /*	        int     SIR_MSG_REJECT */
/* 0614 */ 0x0000000aL,
/* 0618 */ 0x80880000L, /*	        jump    to_decisions */
/* 061c */ 0x00000010L,
/* 0620 */ 0x88880000L, /*		call	load_sync */
/* 0624 */ 0x00000134L,
/* 0628 */ 0x60000040L, /*		clear	ack */
/* 062c */ 0x00000000L,
/* 0630 */ 0x818b0000L, /*		jump	data_in_phase, when data_in */
/* 0634 */ 0x00fffa20L,
/* 0638 */ 0x828a0000L, /*		jump	cmd_phase, if cmd */
/* 063c */ 0x00fffa00L,
/* 0640 */ 0x808a0000L, /*		jump	data_out_phase, if data_out */
/* 0644 */ 0x00fffaecL,
/* 0648 */ 0x838a0000L, /*		jump	status_phase, if status */
/* 064c */ 0x00fffba4L,
/* 0650 */ 0x878a0000L, /*		jump	msg_in_phase, if msg_in */
/* 0654 */ 0x00fffbacL,
/* 0658 */ 0x98080000L, /*		int	error_unexpected_phase */
/* 065c */ 0x00000004L,
/* 0660 */ 0x838b0000L, /*		jump	status_phase, when status */
/* 0664 */ 0x00fffb8cL,
/* 0668 */ 0x878a0000L, /*		jump	msg_in_phase, if msg_in */
/* 066c */ 0x00fffb94L,
/* 0670 */ 0x98080000L, /*		int	error_unexpected_phase */
/* 0674 */ 0x00000004L,
/* 0678 */ 0x00000000L, /*	state:	defw	0 */
/* 067c */ 0x00000000L, /*	dmaaddr: defw	0 */
/* 0680 */ 0x00000000L, /*	targ:	defw	0 */
/* 0684 */ 0x00000000L, /*	lun:	defw	0 */
/* 0688 */ 0x00000000L, /*	sync:	defw	0 */
/* 068c */ 0x00000000L, /*	next:	defw	0 */
			/*	dsa_load_len = dsa_load_end - dsa_copy */
			/*	dsa_save_len = dsa_save_end - dsa_copy */
/* 0690 */ 0xc0000004L, /*		move	memory 4, dsa, load_state_smc0 + 4 */
/* 0694 */ 0x00000010L,
/* 0698 */ 0x000006a0L,
/* 069c */ 0xc0000018L, /*		move	memory dsa_load_len, 0, dsa_copy */
/* 06a0 */ 0x00000000L,
/* 06a4 */ 0x00000678L,
/* 06a8 */ 0x90080000L, /*		return */
/* 06ac */ 0x00000000L,
/* 06b0 */ 0xc0000004L, /*		move	memory 4, dsa, save_state_smc0 + 8 */
/* 06b4 */ 0x00000010L,
/* 06b8 */ 0x000006c4L,
/* 06bc */ 0xc0000008L, /*		move	memory dsa_save_len, dsa_copy, 0 */
/* 06c0 */ 0x00000678L,
/* 06c4 */ 0x00000000L,
/* 06c8 */ 0x90080000L, /*		return */
/* 06cc */ 0x00000000L,
/* 06d0 */ 0x721a0000L, /*		move	ctest2 to sfbr */
/* 06d4 */ 0x00000000L,
/* 06d8 */ 0xc0000004L, /*		move	memory 4, dsa_head, dsa */
/* 06dc */ 0x00000008L,
/* 06e0 */ 0x00000010L,
/* 06e4 */ 0x72100000L, /*		move	dsa0 to sfbr */
/* 06e8 */ 0x00000000L,
/* 06ec */ 0x80840000L, /*		jump	issue_check_1, if not 0 */
/* 06f0 */ 0x00000030L,
/* 06f4 */ 0x72110000L, /*		move	dsa1 to sfbr */
/* 06f8 */ 0x00000000L,
/* 06fc */ 0x80840000L, /*		jump	issue_check_1, if not 0 */
/* 0700 */ 0x00000020L,
/* 0704 */ 0x72120000L, /*		move	dsa2 to sfbr */
/* 0708 */ 0x00000000L,
/* 070c */ 0x80840000L, /*		jump	issue_check_1, if not 0 */
/* 0710 */ 0x00000010L,
/* 0714 */ 0x72130000L, /*		move	dsa3 to sfbr */
/* 0718 */ 0x00000000L,
/* 071c */ 0x808c0000L, /*		jump	wait_for_reselection, if 0 */
/* 0720 */ 0x00fffdf8L,
/* 0724 */ 0x88880000L, /*	 	call	load_state */
/* 0728 */ 0x00ffff64L,
/* 072c */ 0xc0000004L, /*		move	memory 4, state, scratcha */
/* 0730 */ 0x00000678L,
/* 0734 */ 0x00000034L,
/* 0738 */ 0x72340000L, /*		move	scratcha0 to sfbr */
/* 073c */ 0x00000000L,
/* 0740 */ 0x808c0002L, /*		jump	start, if STATE_ISSUE */
/* 0744 */ 0x00fff8c0L,
/* 0748 */ 0xc0000004L, /*		move	memory 4, next, dsa */
/* 074c */ 0x0000068cL,
/* 0750 */ 0x00000010L,
/* 0754 */ 0x80880000L, /*		jump	issue_check_loop */
/* 0758 */ 0x00ffff88L,
/* 075c */ 0xc0000004L, /*		move	memory 4, sync, scratcha */
/* 0760 */ 0x00000688L,
/* 0764 */ 0x00000034L,
/* 0768 */ 0x72340000L, /*		move	scratcha0 to sfbr */
/* 076c */ 0x00000000L,
/* 0770 */ 0x6a030000L, /*		move	sfbr to scntl3 */
/* 0774 */ 0x00000000L,
/* 0778 */ 0x72350000L, /*		move	scratcha1 to sfbr */
/* 077c */ 0x00000000L,
/* 0780 */ 0x6a050000L, /*		move	sfbr to sxfer */
/* 0784 */ 0x00000000L,
/* 0788 */ 0x90080000L, /*		return */
/* 078c */ 0x00000000L,
};

#define NA_SCRIPT_SIZE 484

struct na_patch na_patches[] = {
	{ 0x0006, 5 }, /* 00000018 */
	{ 0x000b, 4 }, /* 0000002c */
	{ 0x0013, 4 }, /* 0000004c */
	{ 0x0017, 1 }, /* 0000005c */
	{ 0x0018, 2 }, /* 00000060 */
	{ 0x001a, 1 }, /* 00000068 */
	{ 0x001b, 2 }, /* 0000006c */
	{ 0x0021, 1 }, /* 00000084 */
	{ 0x002b, 2 }, /* 000000ac */
	{ 0x002c, 1 }, /* 000000b0 */
	{ 0x0030, 2 }, /* 000000c0 */
	{ 0x0031, 1 }, /* 000000c4 */
	{ 0x0037, 2 }, /* 000000dc */
	{ 0x0038, 1 }, /* 000000e0 */
	{ 0x003a, 2 }, /* 000000e8 */
	{ 0x003b, 1 }, /* 000000ec */
	{ 0x0043, 4 }, /* 0000010c */
	{ 0x0047, 2 }, /* 0000011c */
	{ 0x0048, 1 }, /* 00000120 */
	{ 0x004e, 1 }, /* 00000138 */
	{ 0x004f, 2 }, /* 0000013c */
	{ 0x0051, 1 }, /* 00000144 */
	{ 0x0052, 2 }, /* 00000148 */
	{ 0x0058, 1 }, /* 00000160 */
	{ 0x0059, 2 }, /* 00000164 */
	{ 0x005b, 1 }, /* 0000016c */
	{ 0x0065, 2 }, /* 00000194 */
	{ 0x0066, 1 }, /* 00000198 */
	{ 0x006a, 2 }, /* 000001a8 */
	{ 0x006b, 1 }, /* 000001ac */
	{ 0x0073, 4 }, /* 000001cc */
	{ 0x0077, 2 }, /* 000001dc */
	{ 0x0078, 1 }, /* 000001e0 */
	{ 0x007e, 4 }, /* 000001f8 */
	{ 0x0082, 2 }, /* 00000208 */
	{ 0x0098, 1 }, /* 00000260 */
	{ 0x0099, 2 }, /* 00000264 */
	{ 0x009f, 2 }, /* 0000027c */
	{ 0x00a0, 1 }, /* 00000280 */
	{ 0x00b6, 2 }, /* 000002d8 */
	{ 0x00c2, 2 }, /* 00000308 */
	{ 0x00ca, 2 }, /* 00000328 */
	{ 0x00d0, 2 }, /* 00000340 */
	{ 0x00d6, 2 }, /* 00000358 */
	{ 0x00e6, 2 }, /* 00000398 */
	{ 0x00ec, 2 }, /* 000003b0 */
	{ 0x0102, 2 }, /* 00000408 */
	{ 0x010e, 2 }, /* 00000438 */
	{ 0x011c, 4 }, /* 00000470 */
	{ 0x0126, 2 }, /* 00000498 */
	{ 0x0134, 1 }, /* 000004d0 */
	{ 0x0135, 2 }, /* 000004d4 */
	{ 0x0139, 2 }, /* 000004e4 */
	{ 0x013a, 1 }, /* 000004e8 */
	{ 0x014e, 2 }, /* 00000538 */
	{ 0x0152, 4 }, /* 00000548 */
	{ 0x0153, 2 }, /* 0000054c */
	{ 0x0167, 1 }, /* 0000059c */
	{ 0x0168, 2 }, /* 000005a0 */
	{ 0x016d, 3 }, /* 000005b4 */
	{ 0x0170, 1 }, /* 000005c0 */
	{ 0x0171, 1 }, /* 000005c4 */
	{ 0x0175, 1 }, /* 000005d4 */
	{ 0x0176, 1 }, /* 000005d8 */
	{ 0x017c, 1 }, /* 000005f0 */
	{ 0x017d, 2 }, /* 000005f4 */
	{ 0x01a5, 2 }, /* 00000694 */
	{ 0x01a6, 1 }, /* 00000698 */
	{ 0x01a9, 1 }, /* 000006a4 */
	{ 0x01ad, 2 }, /* 000006b4 */
	{ 0x01ae, 1 }, /* 000006b8 */
	{ 0x01b0, 1 }, /* 000006c0 */
	{ 0x01b7, 4 }, /* 000006dc */
	{ 0x01b8, 2 }, /* 000006e0 */
	{ 0x01cc, 1 }, /* 00000730 */
	{ 0x01cd, 2 }, /* 00000734 */
	{ 0x01d3, 1 }, /* 0000074c */
	{ 0x01d4, 2 }, /* 00000750 */
	{ 0x01d8, 1 }, /* 00000760 */
	{ 0x01d9, 2 }, /* 00000764 */
};
#define NA_PATCHES 80

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
	X_ssid_mask,
};

enum {
	E_issue_check_next = 1864,
	E_issue_check_1 = 1828,
	E_issue_check_loop = 1764,
	E_save_state_smc0 = 1724,
	E_load_state_smc0 = 1692,
	E_dsa_load_end = 1680,
	E_sync = 1672,
	E_dsa_save_end = 1664,
	E_dsa_copy = 1656,
	E_id_out_mismatch_recover = 1536,
	E_next = 1676,
	E_reload_sync = 1568,
	E_find_dsa_smc2 = 1508,
	E_lun = 1668,
	E_find_dsa_smc1 = 1480,
	E_targ = 1664,
	E_find_dsa_next = 1516,
	E_load_state = 1680,
	E_find_dsa_1 = 1424,
	E_find_dsa_loop = 1360,
	E_find_dsa = 1348,
	E_sigp_set = 1744,
	E_reselected = 1316,
	E_wsr_check = 1268,
	E_response_msg_in = 1172,
	E_response_repeat = 1132,
	E_response = 1108,
	E_reject = 988,
	E_wdtr = 964,
	E_sdtr = 876,
	E_ext_done = 988,
	E_ext_1 = 756,
	E_ext_2 = 900,
	E_ext_3 = 788,
	E_issue_check = 1752,
	E_extended = 708,
	E_ignore_wide = 1060,
	E_msg_in_skip = 692,
	E_disconnected = 1204,
	E_msg_in_not_reject = 532,
	E_rejected = 668,
	E_msg_in_phase = 516,
	E_status_phase = 500,
	E_data_out_mismatch = 464,
	E_data_out_block_mismatch = 368,
	E_data_out_normal = 440,
	E_data_out_block_loop = 332,
	E_data_out_phase = 308,
	E_post_data_to_decisions = 1632,
	E_data_in_mismatch = 272,
	E_data_mismatch_recover = 228,
	E_data_block_mismatch_recover = 216,
	E_save_state = 1712,
	E_data_in_block_mismatch = 136,
	E_data_in_normal = 248,
	E_data_in_block_loop = 112,
	E_dmaaddr = 1660,
	E_state = 1656,
	E_data_in_phase = 88,
	E_cmd_out_mismatch = 80,
	E_cmd_phase = 64,
	E_to_decisions = 1584,
	E_id_out_mismatch = 48,
	E_start1 = 40,
	E_reselected_on_select = 1292,
	E_load_sync = 1884,
	E_start = 8,
	E_wait_for_reselection = 1308,
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
#define A_SIR_ERROR_NOT_MSG_IN_AFTER_RESELECT 6
#define A_error_weird_message 5
#define A_error_unexpected_phase 4
#define A_error_reselected 3
#define A_error_disconnected 2
#define A_error_not_cmd_complete 1
#define A_SIR_MSG_IO_COMPLETE 0
