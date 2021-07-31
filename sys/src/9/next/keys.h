uchar key0[]={
	0xff,	/* 00 */
	0xff,	/* 01  dimmer */
	0xff,	/* 02  quieter */
	'\\',	/* 03 */
	']',	/* 04 */
	'[',	/* 05 */
	'i',	/* 06 */
	'o',	/* 07 */
	'p',	/* 08 */
	0x80,	/* 09  left arrow */
	0xff,	/* 0a  ??? */
	0xff,	/* 0b  keypad 0 */
	0xff,	/* 0c  keypad . */
	'\r',	/* 0d  keypad enter */
	0xff,	/* 0e  ??? */
	0x80,	/* 0f  down arrow */
	0x80,	/* 10  right arrow */
	0xff,	/* 11  keypad 1 */
	0xff,	/* 12  keypad 4 */
	0xff,	/* 13  keypad 6 */
	0xff,	/* 14  keypad 3 */
	0xff,	/* 15  keypad + */
	0xff,	/* 16  up arrow */
	0xff,	/* 17  keypad 2 */
	0xff,	/* 18  keypad 5 */
	0xff,	/* 19  brighter */
	0xff,	/* 1a  louder */
	'\b',	/* 1b  backspace arrow */
	'=',	/* 1c */
	'-',	/* 1d */
	'8',	/* 1e */
	'9',	/* 1f */
	'0',	/* 20 */
	0xff,	/* 21  keypad 7 */
	0xff,	/* 22  keypad 8 */
	0xff,	/* 23  keypad 9 */
	0xff,	/* 24  keypad - */
	0x7f,	/* 25  keypad * */
	'`',	/* 26 */
	0xff,	/* 27  keypad = */
	'/',	/* 28  keypad / */
	0xff,	/* 29  ??? */
	'\n',	/* 2a  carriage return */
	'\'',	/* 2b */
	';',	/* 2c */
	'l',	/* 2d */
	',',	/* 2e */
	'.',	/* 2f */
	'/',	/* 30 */
	'z',	/* 31 */
	'x',	/* 32 */
	'c',	/* 33 */
	'v',	/* 34 */
	'b',	/* 35 */
	'm',	/* 36 */
	'n',	/* 37 */
	' ',	/* 38 */
	'a',	/* 39 */
	's',	/* 3a */
	'd',	/* 3b */
	'f',	/* 3c */
	'g',	/* 3d */
	'k',	/* 3e */
	'j',	/* 3f */
	'h',	/* 40 */
	'\t',	/* 41 */
	'q',	/* 42 */
	'w',	/* 43 */
	'e',	/* 44 */
	'r',	/* 45 */
	'u',	/* 46 */
	'y',	/* 47 */
	't',	/* 48 */
	0x1b,	/* 49  escape */
	'1',	/* 4a */
	'2',	/* 4b */
	'3',	/* 4c */
	'4',	/* 4d */
	'7',	/* 4e */
	'6',	/* 4f */
	'5',	/* 50 */
};

uchar keys[]={	/* shifted */
	0xff,	/* 00 */
	0xff,	/* 01  dimmer */
	0xff,	/* 02  quieter */
	'\\',	/* 03 */
	'}',	/* 04 */
	'{',	/* 05 */
	'I',	/* 06 */
	'O',	/* 07 */
	'P',	/* 08 */
	0x80,	/* 09  left arrow */
	0xff,	/* 0a  ??? */
	0xff,	/* 0b  keypad 0 */
	0xff,	/* 0c  keypad . */
	'\r',	/* 0d  keypad enter */
	0xff,	/* 0e  ??? */
	0x80,	/* 0f  down arrow */
	0x80,	/* 10  right arrow */
	0xff,	/* 11  keypad 1 */
	0xff,	/* 12  keypad 4 */
	0xff,	/* 13  keypad 6 */
	0xff,	/* 14  keypad 3 */
	0xff,	/* 15  keypad + */
	0xff,	/* 16  up arrow */
	0xff,	/* 17  keypad 2 */
	0xff,	/* 18  keypad 5 */
	0xff,	/* 19  brighter */
	0xff,	/* 1a  louder */
	'\b',	/* 1b  backspace arrow*/
	'+',	/* 1c */
	'_',	/* 1d */
	'*',	/* 1e */
	'(',	/* 1f */
	')',	/* 20 */
	0xff,	/* 21  keypad 7 */
	0xff,	/* 22  keypad 8 */
	0xff,	/* 23  keypad 9 */
	0xff,	/* 24  keypad - */
	0x7f,	/* 25  keypad * */
	'~',	/* 26 */
	'|',	/* 27  keypad = */
	'\\',	/* 28  keypad / */
	0xff,	/* 29  ??? */
	'\n',	/* 2a  carriage return */
	'"',	/* 2b */
	':',	/* 2c */
	'L',	/* 2d */
	'<',	/* 2e */
	'>',	/* 2f */
	'?',	/* 30 */
	'Z',	/* 31 */
	'X',	/* 32 */
	'C',	/* 33 */
	'V',	/* 34 */
	'B',	/* 35 */
	'M',	/* 36 */
	'N',	/* 37 */
	' ',	/* 38 */
	'A',	/* 39 */
	'S',	/* 3a */
	'D',	/* 3b */
	'F',	/* 3c */
	'G',	/* 3d */
	'K',	/* 3e */
	'J',	/* 3f */
	'H',	/* 40 */
	'\t',	/* 41 */
	'Q',	/* 42 */
	'W',	/* 43 */
	'E',	/* 44 */
	'R',	/* 45 */
	'U',	/* 46 */
	'Y',	/* 47 */
	'T',	/* 48 */
	0x1b,	/* 49  escape */
	'!',	/* 4a */
	'@',	/* 4b */
	'#',	/* 4c */
	'$',	/* 4d */
	'&',	/* 4e */
	'^',	/* 4f */
	'%',	/* 50 */
};

uchar keyc[]={	/* control */
	0xff,	/* 00 */
	0xff,	/* 01  dimmer */
	0xff,	/* 02  quieter */
	0x1c,	/* 03 */
	0x1d,	/* 04 */
	0x1b,	/* 05 */
	'\t',	/* 06 */
	0x17,	/* 07 */
	0x10,	/* 08 */
	0x80,	/* 09  left arrow */
	0xff,	/* 0a  ??? */
	0xff,	/* 0b  keypad 0 */
	0xff,	/* 0c  keypad . */
	'\r',	/* 0d  keypad enter */
	0xff,	/* 0e  ??? */
	0x80,	/* 0f  down arrow */
	0x80,	/* 10  right arrow */
	0xff,	/* 11  keypad 1 */
	0xff,	/* 12  keypad 4 */
	0xff,	/* 13  keypad 6 */
	0xff,	/* 14  keypad 3 */
	0xff,	/* 15  keypad + */
	0xff,	/* 16  up arrow */
	0xff,	/* 17  keypad 2 */
	0xff,	/* 18  keypad 5 */
	0xff,	/* 19  brighter */
	0xff,	/* 1a  louder */
	'\b',	/* 1b  backspace arrow */
	0x1f,	/* 1c */
	'_',	/* 1d */
	'*',	/* 1e */
	'(',	/* 1f */
	')',	/* 20 */
	0xff,	/* 21  keypad 7 */
	0xff,	/* 22  keypad 8 */
	0xff,	/* 23  keypad 9 */
	0xff,	/* 24  keypad - */
	0x7f,	/* 25  keypad * */
	0x1e,	/* 26 */
	'|',	/* 27  keypad = */
	0x1c,	/* 28  keypad / */
	0xff,	/* 29  ??? */
	'\n',	/* 2a  carriage return */
	'"',	/* 2b */
	':',	/* 2c */
	0x0c,	/* 2d */
	'<',	/* 2e */
	'>',	/* 2f */
	'?',	/* 30 */
	0x1a,	/* 31 */
	0x18,	/* 32 */
	0x03,	/* 33 */
	0x16,	/* 34 */
	0x02,	/* 35 */
	'\r',	/* 36 */
	0x0e,	/* 37 */
	' ',	/* 38 */
	0x01,	/* 39 */
	0x13,	/* 3a */
	0x04,	/* 3b */
	0x06,	/* 3c */
	0x07,	/* 3d */
	0x0b,	/* 3e */
	'\n',	/* 3f */
	'\b',	/* 40 */
	'\t',	/* 41 */
	0x11,	/* 42 */
	0x17,	/* 43 */
	0x0e,	/* 44 */
	0x12,	/* 45 */
	0x15,	/* 46 */
	0x19,	/* 47 */
	0x14,	/* 48 */
	0x1b,	/* 49  escape */
	'!',	/* 4a */
	0x00,	/* 4b */
	'#',	/* 4c */
	'$',	/* 4d */
	'&',	/* 4e */
	0x1e,	/* 4f */
	'%',	/* 50 */
};
