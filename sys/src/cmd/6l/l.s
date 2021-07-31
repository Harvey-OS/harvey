
TEXT	main+0(SB),$44

	MOV	R10, 20		 	/* mema mode 0 */
	MOV	R10, 0		 	/* mema mode 0 */
	MOV	R10, 20(R11)		/* mema mode 1 */

	MOV	R10, 0(R11)		/* memb mode 4 */
	MOV	R10, 0(R11)(R1*16)	/* memb mode 7 */
	MOV	R10, 100000		/* memb mode c */
	MOV	R10, 100000(R11)	/* memb mode d */
	MOV	R10, 100000(R1*16)	/* memb mode e */
	MOV	R10, 100000(R11)(R1*16)	/* memb mode f */
