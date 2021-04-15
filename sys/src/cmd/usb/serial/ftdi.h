enum {
	/* used by devices which don't provide their own Vid */
	FTVid		= 0x0403,

	FTSheevaVid	= 0x9E88,
	FTSheevaDid	= 0x9E8F,
	FTOpenRDUltDid	= 0x9E90,

	FTSIODid	= 0x8372,	/* Product Id SIO appl'n of 8U100AX */
	FT8U232AMDid	= 0x6001,	/* Similar device to SIO above */
	FT8U232AMALTDid	= 0x6006,	/* FT's alternate Did for above*/
	FT8U2232CDid	= 0x6010,	/* Dual channel device */
	FTRELAISDid	= 0xFA10,	/* Relais device */

	/* NF reader */
	FTNFRICVid	= 0x0DCD,
	FTNFRICDid	= 0x0001,

	FTACTZWAVEDid	= 0xF2D0,		/* www.irtrans.de device */

	/*
	 * ACT Solutions HomePro ZWave interface
	 * http://www.act-solutions.com/HomePro.htm)
	 */
	FTIRTRANSDid	= 0xFC60,

	/*
	 * www.thoughttechnology.com/ TT-USB
	 */
	FTTTUSBDid	= 0xFF20,

	/* iPlus device */
	FTIPLUSDid	= 0xD070,

	/* www.crystalfontz.com devices */
	FTXF632Did = 0xFC08,	/* 632: 16x2 Character Display */
	FTXF634Did = 0xFC09,	/* 634: 20x4 Character Display */
	FTXF547Did = 0xFC0A,	/* 547: Two line Display */
	FTXF633Did = 0xFC0B,	/* 633: 16x2 Character Display with Keys */
	FTXF631Did = 0xFC0C,	/* 631: 20x2 Character Display */
	FTXF635Did = 0xFC0D,	/* 635: 20x4 Character Display */
	FTXF640Did = 0xFC0E,	/* 640: Two line Display */
	FTXF642Did = 0xFC0F,	/* 642: Two line Display */

	/*
	 * Video Networks Limited / Homechoice in the UK
	 * use an ftdi-based device for their 1Mb broadband
	 */
	FTVNHCPCUSBDDid	= 0xfe38,

	/*
	 * PCDJ use ftdi based dj-controllers
	 * DAC-2 device http://www.pcdjhardware.com/DAC2.asp
	 */
	FTPCDJDAC2Did = 0xFA88,

	/*
	 * Matrix Orbital LCD displays,
	 * which are the FT232BM (similar to the 8U232AM)
	 */
	FTMTXORB0Did = 0xFA00,
	FTMTXORB1Did = 0xFA01,
	FTMTXORB2Did = 0xFA02,
	FTMTXORB3Did = 0xFA03,
	FTMTXORB4Did = 0xFA04,
	FTMTXORB5Did = 0xFA05,
	FTMTXORB6Did = 0xFA06,

	/* Interbiometrics USB I/O Board */
	INTERBIOMVid		= 0x1209,
	INTERBIOMIOBRDDid	= 0x1002,
	INTERBIOMMINIIOBRDDid	= 0x1006,

	/*
	 * The following are the values for the Perle Systems
	 * UltraPort USB serial converters
	 */
	FTPERLEULTRAPORTDid = 0xF0C0,

	/*
	 * Sealevel SeaLINK+ adapters.
	 */

	SEALEVELVid = 0x0c52,

	SEALEVEL2101Did = 0x2101,	/* SeaLINK+232 (2101/2105) */
	SEALEVEL2102Did = 0x2102,	/* SeaLINK+485 (2102) */
	SEALEVEL2103Did = 0x2103,	/* SeaLINK+232I (2103) */
	SEALEVEL2104Did = 0x2104,	/* SeaLINK+485I (2104) */
	SEALEVEL22011Did = 0x2211,	/* SeaPORT+2/232 (2201) Port 1 */
	SEALEVEL22012Did = 0x2221,	/* SeaPORT+2/232 (2201) Port 2 */
	SEALEVEL22021Did = 0x2212,	/* SeaPORT+2/485 (2202) Port 1 */
	SEALEVEL22022Did = 0x2222,	/* SeaPORT+2/485 (2202) Port 2 */
	SEALEVEL22031Did = 0x2213,	/* SeaPORT+2 (2203) Port 1 */
	SEALEVEL22032Did = 0x2223,	/* SeaPORT+2 (2203) Port 2 */
	SEALEVEL24011Did = 0x2411,	/* SeaPORT+4/232 (2401) Port 1 */
	SEALEVEL24012Did = 0x2421,	/* SeaPORT+4/232 (2401) Port 2 */
	SEALEVEL24013Did = 0x2431,	/* SeaPORT+4/232 (2401) Port 3 */
	SEALEVEL24014Did = 0x2441,	/* SeaPORT+4/232 (2401) Port 4 */
	SEALEVEL24021Did = 0x2412,	/* SeaPORT+4/485 (2402) Port 1 */
	SEALEVEL24022Did = 0x2422,	/* SeaPORT+4/485 (2402) Port 2 */
	SEALEVEL24023Did = 0x2432,	/* SeaPORT+4/485 (2402) Port 3 */
	SEALEVEL24024Did = 0x2442,	/* SeaPORT+4/485 (2402) Port 4 */
	SEALEVEL24031Did = 0x2413,	/* SeaPORT+4 (2403) Port 1 */
	SEALEVEL24032Did = 0x2423,	/* SeaPORT+4 (2403) Port 2 */
	SEALEVEL24033Did = 0x2433,	/* SeaPORT+4 (2403) Port 3 */
	SEALEVEL24034Did = 0x2443,	/* SeaPORT+4 (2403) Port 4 */
	SEALEVEL28011Did = 0x2811,	/* SeaLINK+8/232 (2801) Port 1 */
	SEALEVEL28012Did = 0x2821,	/* SeaLINK+8/232 (2801) Port 2 */
	SEALEVEL28013Did = 0x2831,	/* SeaLINK+8/232 (2801) Port 3 */
	SEALEVEL28014Did = 0x2841,	/* SeaLINK+8/232 (2801) Port 4 */
	SEALEVEL28015Did = 0x2851,	/* SeaLINK+8/232 (2801) Port 5 */
	SEALEVEL28016Did = 0x2861,	/* SeaLINK+8/232 (2801) Port 6 */
	SEALEVEL28017Did = 0x2871,	/* SeaLINK+8/232 (2801) Port 7 */
	SEALEVEL28018Did = 0x2881,	/* SeaLINK+8/232 (2801) Port 8 */
	SEALEVEL28021Did = 0x2812,	/* SeaLINK+8/485 (2802) Port 1 */
	SEALEVEL28022Did = 0x2822,	/* SeaLINK+8/485 (2802) Port 2 */
	SEALEVEL28023Did = 0x2832,	/* SeaLINK+8/485 (2802) Port 3 */
	SEALEVEL28024Did = 0x2842,	/* SeaLINK+8/485 (2802) Port 4 */
	SEALEVEL28025Did = 0x2852,	/* SeaLINK+8/485 (2802) Port 5 */
	SEALEVEL28026Did = 0x2862,	/* SeaLINK+8/485 (2802) Port 6 */
	SEALEVEL28027Did = 0x2872,	/* SeaLINK+8/485 (2802) Port 7 */
	SEALEVEL28028Did = 0x2882,	/* SeaLINK+8/485 (2802) Port 8 */
	SEALEVEL28031Did = 0x2813,	/* SeaLINK+8 (2803) Port 1 */
	SEALEVEL28032Did = 0x2823,	/* SeaLINK+8 (2803) Port 2 */
	SEALEVEL28033Did = 0x2833,	/* SeaLINK+8 (2803) Port 3 */
	SEALEVEL28034Did = 0x2843,	/* SeaLINK+8 (2803) Port 4 */
	SEALEVEL28035Did = 0x2853,	/* SeaLINK+8 (2803) Port 5 */
	SEALEVEL28036Did = 0x2863,	/* SeaLINK+8 (2803) Port 6 */
	SEALEVEL28037Did = 0x2873,	/* SeaLINK+8 (2803) Port 7 */
	SEALEVEL28038Did = 0x2883,	/* SeaLINK+8 (2803) Port 8 */

	/* KOBIL Vendor ID chipcard terminals */
	KOBILVid	= 0x0d46,
	KOBILCONVB1Did	= 0x2020,	/* KOBIL Konverter for B1 */
	KOBILCONVKAANDid = 0x2021,	/* KOBILKonverter for KAAN */

	/* Icom ID-1 digital transceiver */
	ICOMID1Vid	= 0x0C26,
	ICOMID1Did	= 0x0004,

	FTASKRDR400Did	= 0xC991,	/* ASK RDR 400 series card reader */
	FTDSS20Did	= 0xFC82,	/* DSS-20 Sync Station for Sony Ericsson P800 */

	/*
	 * Home Electronics (www.home-electro.com) USB gadgets
	 */
	FTHETIRA1Did	= 0xFA78,	/* Tira-1 IR transceiver */

	/*
	 * An infrared receiver and transmitter using the 8U232AM chip
	 * http://www.usbuirt.com
	 */
	FTUSBUIRTDid	= 0xF850,

	FTELVUR100Did	= 0xFB58,	/* USB-RS232-Umsetzer (UR 100) */
	FTELVUM100Did	= 0xFB5A,	/* USB-Modul UM 100 */
	FTELVUO100Did	= 0xFB5B,	/* USB-Modul UO 100 */
	FTELVALC8500Did	= 0xF06E,	/* ALC 8500 Expert */
	FTELVCLI7000Did	= 0xFB59,	/* Computer-Light-Interface */
	FTELVPPS7330Did	= 0xFB5C,	/* Processor-Power-Supply (PPS 7330) */
	FTELVTFM100Did	= 0xFB5D,	/* Temperartur-Feuchte Messgeraet (TFM 100) */
	FTELVUDF77Did	= 0xFB5E,	/* USB DCF Funkurh (UDF 77) */
	FTELVUIO88Did	= 0xFB5F,	/* USB-I/O Interface (UIO 88) */
	FTELVUAD8Did	= 0xF068,	/* USB-AD-Wandler (UAD 8) */
	FTELVUDA7Did	= 0xF069,	/* USB-DA-Wandler (UDA 7) */
	FTELVUSI2Did	= 0xF06A,	/* USB-Schrittmotoren-Interface (USI 2) */
	FTELVT1100Did	= 0xF06B,	/* Thermometer (T 1100) */
	FTELVPCD200Did	= 0xF06C,	/* PC-Datenlogger (PCD 200) */
	FTELVULA200Did	= 0xF06D,	/* USB-LCD-Ansteuerung (ULA 200) */
	FTELVFHZ1000PCDid= 0xF06F,	/* FHZ 1000 PC */
	FTELVCSI8Did	= 0xE0F0,	/* Computer-Schalt-Interface (CSI 8) */
	FTELVEM1000DLDid= 0xE0F1,	/* PC-Datenlogger fuer Energiemonitor (EM 1000 DL) */
	FTELVPCK100Did	= 0xE0F2,	/* PC-Kabeltester (PCK 100) */
	FTELVRFP500Did	= 0xE0F3,	/* HF-Leistungsmesser (RFP 500) */
	FTELVFS20SIGDid	= 0xE0F4,	/* Signalgeber (FS 20 SIG) */
	FTELVWS300PCDid	= 0xE0F6,	/* PC-Wetterstation (WS 300 PC) */
	FTELVFHZ1300PCDid= 0xE0E8,	/* FHZ 1300 PC */
	FTELVWS500Did	= 0xE0E9,	/* PC-Wetterstation (WS 500) */

	/*
	 * Definitions for ID TECH (http://www.idt-net.com) devices
	 */
	IDTECHVid	= 0x0ACD,	/* ID TECH Vendor ID */
	IDTECHIDT1221UDid= 0x0300,	/* IDT1221U USB to RS-232 */

	/*
	 * Definitions for Omnidirectional Control Technology, Inc. devices
	 */
	OCTVid		= 0x0B39,	/* OCT vendor ID */

	/*
	 * Note: OCT US101 is also rebadged as Dick Smith Electronics
	 * (NZ) XH6381, Dick Smith Electronics (Aus) XH6451, and SIIG
	 * Inc. model US2308 hardware version 1.
	 */
	OCTUS101Did	= 0x0421,	/* OCT US101 USB to RS-232 */

	/*
	 *	infrared receiver for access control with IR tags
	 */
	FTPIEGROUPDid	= 0xF208,

	/*
	 * Definitions for Artemis astronomical USB based cameras
	 * http://www.artemisccd.co.uk/
	 */

	FTARTEMISDid	= 0xDF28,	/* All Artemis Cameras */

	FTATIKATK16Did	= 0xDF30,	/* ATIK ATK-16 Grayscale Camera */
	FTATIKATK16CDid = 0xDF32,	/* ATIK ATK-16C Colour Camera */
	FTATIKATK16HRDid= 0xDF31,	/* ATIK ATK-16HR Grayscale */
	FTATIKATK16HRCDid= 0xDF33,	/* ATIK ATK-16HRC Colour Camera */

	/*
	 * Protego products
	 */
	PROTEGOSPECIAL1	= 0xFC70,	/* special/unknown device */
	PROTEGOR2X0	= 0xFC71,	/* R200-USB TRNG unit (R210, R220, and R230) */
	PROTEGOSPECIAL3	= 0xFC72,	/* special/unknown device */
	PROTEGOSPECIAL4	= 0xFC73,	/* special/unknown device */

	/*
	 * Gude Analog- und Digitalsysteme GmbH
	 */
	FTGUDEADSE808Did = 0xE808,
	FTGUDEADSE809Did = 0xE809,
	FTGUDEADSE80ADid = 0xE80A,
	FTGUDEADSE80BDid = 0xE80B,
	FTGUDEADSE80CDid = 0xE80C,
	FTGUDEADSE80DDid = 0xE80D,
	FTGUDEADSE80EDid = 0xE80E,
	FTGUDEADSE80FDid = 0xE80F,
	FTGUDEADSE888Did = 0xE888,	/* Expert ISDN Control USB */
	FTGUDEADSE889Did = 0xE889,	/* USB RS-232 OptoBridge */
	FTGUDEADSE88ADid = 0xE88A,
	FTGUDEADSE88BDid = 0xE88B,
	FTGUDEADSE88CDid = 0xE88C,
	FTGUDEADSE88DDid = 0xE88D,
	FTGUDEADSE88EDid = 0xE88E,
	FTGUDEADSE88FDid = 0xE88F,

	/*
	 * Linx Technologies
	 */
	LINXSDMUSBQSSDid= 0xF448,	/* Linx SDM-USB-QS-S */
	LINXMASTERDEVEL2Did= 0xF449,	/* Linx Master Development.0 */
	LINXFUTURE0Did	= 0xF44A,	/* Linx future device */
	LINXFUTURE1Did	= 0xF44B,	/* Linx future device */
	LINXFUTURE2Did	= 0xF44C,	/* Linx future device */

	/*
	 * CCS Inc. ICDU/ICDU40 - the FT232BM used in a in-circuit-debugger
	 * unit for PIC16's/PIC18's
	 */
	FTCCSICDU200Did	= 0xF9D0,
	FTCCSICDU401Did	= 0xF9D1,

	/* Inside Accesso contactless reader (http://www.insidefr.com) */
	INSIDEACCESSO	= 0xFAD0,

	/*
	 * Intrepid Control Systems (http://www.intrepidcs.com/)
	 * ValueCAN and NeoVI
	 */
	INTREDidVid	= 0x093C,
	INTREDidVALUECANDid= 0x0601,
	INTREDidNEOVIDid= 0x0701,

	/*
	 * Falcom Wireless Communications GmbH
	 */
	FALCOMVid	= 0x0F94,
	FALCOMTWISTDid	= 0x0001,	/* Falcom Twist USB GPRS modem */
	FALCOMSAMBADid	= 0x0005,	/* Falcom Samba USB GPRS modem */

	/*
	 * SUUNTO
	 */
	FTSUUNTOSPORTSDid= 0xF680,	/* Suunto Sports instrument */

	/*
	 * B&B Electronics
	 */
	BANDBVid	= 0x0856,	/* B&B Electronics Vendor ID */
	BANDBUSOTL4Did	= 0xAC01,	/* USOTL4 Isolated RS-485 */
	BANDBUSTL4Did	= 0xAC02,	/* USTL4 RS-485 Converter */
	BANDBUSO9ML2Did	= 0xAC03,	/* USO9ML2 Isolated RS-232 */

	/*
	 * RM Michaelides CANview USB (http://www.rmcan.com)
	 * CAN fieldbus interface adapter
	 */
	FTRMCANVIEWDid	= 0xfd60,

	/*
	 * EVER Eco Pro UPS (http://www.ever.com.pl/)
	 */
	EVERECOPROCDSDid = 0xe520,	/* RS-232 converter */

	/*
	 * 4N-GALAXY.DE PIDs for CAN-USB, USB-RS232, USB-RS422, USB-RS485,
	 * USB-TTY activ, USB-TTY passiv. Some PIDs are used by several devices
	 */
	FT4NGALAXYDE0Did = 0x8372,
	FT4NGALAXYDE1Did = 0xF3C0,
	FT4NGALAXYDE2Did = 0xF3C1,

	/*
	 * Mobility Electronics products.
	 */
	MOBILITYVid	= 0x1342,
	MOBILITYUSBSERIALDid= 0x0202,	/* EasiDock USB 200 serial */

	/*
	 * microHAM product IDs (http://www.microham.com)
	 */
	FTMHAMKWDid	= 0xEEE8,	/* USB-KW interface */
	FTMHAMYSDid	= 0xEEE9,	/* USB-YS interface */
	FTMHAMY6Did	= 0xEEEA,	/* USB-Y6 interface */
	FTMHAMY8Did	= 0xEEEB,	/* USB-Y8 interface */
	FTMHAMICDid	= 0xEEEC,	/* USB-IC interface */
	FTMHAMDB9Did	= 0xEEED,	/* USB-DB9 interface */
	FTMHAMRS232Did	= 0xEEEE,	/* USB-RS232 interface */
	FTMHAMY9Did	= 0xEEEF,	/* USB-Y9 interface */

	/*
	 * Active Robots product ids.
	 */
	FTACTIVEROBOTSDid	= 0xE548,	/* USB comms board */
	XSENSCONVERTER0Did	= 0xD388,
	XSENSCONVERTER1Did	= 0xD389,
	XSENSCONVERTER2Did	= 0xD38A,
	XSENSCONVERTER3Did	= 0xD38B,
	XSENSCONVERTER4Did	= 0xD38C,
	XSENSCONVERTER5Did	= 0xD38D,
	XSENSCONVERTER6Did	= 0xD38E,
	XSENSCONVERTER7Did	= 0xD38F,

	/*
	 * Xsens Technologies BV products (http://www.xsens.com).
	 */
	FTTERATRONIKVCPDid	= 0xEC88,	/* Teratronik device */
	FTTERATRONIKD2XXDid	= 0xEC89,	/* Teratronik device */

	/*
	 * Evolution Robotics products (http://www.evolution.com/).
	 */
	EVOLUTIONVid	= 0xDEEE,
	EVOLUTIONER1Did	= 0x0300,		/* ER1 Control Module */

	/* Pyramid Computer GmbH */
	FTPYRAMIDDid	= 0xE6C8,		/* Pyramid Appliance Display */

	/*
	 * Posiflex inc retail equipment (http://www.posiflex.com.tw)
	 */
	POSIFLEXVid	= 0x0d3a,
	POSIFLEXPP7000Did= 0x0300,		/* PP-7000II thermal printer */

	/*
	 * Westrex International devices
	 */
	FTWESTREXMODEL777Did	= 0xDC00,	/* Model 777 */
	FTWESTREXMODEL8900FDid	= 0xDC01,	/* Model 8900F */

	/*
	 * RR-CirKits LocoBuffer USB (http://www.rr-cirkits.com)
	 */
	FTRRCIRKITSLOCOBUFFERDid= 0xc7d0,	/* LocoBuffer USB */
	FTECLOCOM1WIREDid	= 0xEA90,	/* COM to 1-Wire USB */

	/*
	 * Papouch products (http://www.papouch.com/)
	 */
	PAPOUCHVid	= 0x5050,
	PAPOUCHTMUDid	= 0x0400,		/* TMU USB Thermometer */

	/*
	 * ACG Identification Technologies GmbH products http://www.acg.de/
	 */
	FTACGHFDUALDid	= 0xDD20,		/* HF Dual ISO Reader (RFID) */
	/*
	 * new high speed devices
	 */
	FT4232HDid	= 0x6011,		/* FTDI FT4232H based device */

	/*
	 * Amontec JTAGkey (http://www.amontec.com/)
	 */
	AMONKEYDid	= 0xCFF8,
};

/* Commands */
enum {
	FTRESET		= 0,		/* Reset the port */
	FTSETMODEMCTRL,			/* Set the modem control register */
	FTSETFLOWCTRL,			/* Set flow control register */
	FTSETBAUDRATE,			/* Set baud rate */
	FTSETDATA,			/* Set the parameters, parity */
	FTGETMODEMSTATUS,		/* Retrieve current value of modem ctl */
	FTSETEVENTCHAR,			/* Set the event character */
	FTSETERRORCHAR,			/* Set the error character */
	FTUNKNOWN,
	FTSETLATENCYTIMER,		/* Set the latency timer */
	FTGETLATENCYTIMER,		/* Get the latency timer */
	FTSETBITMODE,			/* Set bit mode */
	FTGETPINS,			/* Read pins state */
	FTGETE2READ	= 0x90,		/* Read address from 128-byte I2C EEPROM */
	FTSETE2WRITE,			/* Write to address on 128-byte I2C EEPROM */
	FTSETE2ERASE,			/* Erase address on 128-byte I2C EEPROM */
};

/* Port Identifier Table, index for interfaces */
enum {
	PITDEFAULT = 0,		/* SIOA */
	PITA,			/* SIOA jtag if there is one */
};

enum {
	Rftdireq = 1<<6,		/* bit for type of request */
};

/*
 * Commands Data size
 * Sets have wLength = 0
 * Gets have wValue = 0
 */
enum {
	FTMODEMSTATUSSZ	= 1,
	FTLATENCYTIMERSZ= 1,
	FTPINSSZ	= 1,
	FTE2READSZ	= 2,
};

/*
 * bRequest: FTGETE2READ
 * wIndex: Address of word to read
 * Data: Will return a word (2 bytes) of data from E2Address
 * Results put in the I2C 128 byte EEPROM string eeprom+(2*index)
 */

/*
 * bRequest: FTSETE2WRITE
 * wIndex: Address of word to read
 * wValue: Value of the word
 * Data: Will return a word (2 bytes) of data from E2Address
 */

/*
 * bRequest: FTSETE2ERASE
 * Erases the EEPROM
 * wIndex: 0
 */

/*
 * bRequest: FTRESET
 * wValue: Ctl Val
 * wIndex: Port
 */
enum {
	FTRESETCTLVAL		= 0,
	FTRESETCTLVALPURGERX	= 1,
	FTRESETCTLVALPURGETX	= 2,
};

/*
 * BmRequestType: SET
 * bRequest: FTSETBAUDRATE
 * wValue: BaudDivisor value - see below
 * Bits 15 to 0 of the 17-bit divisor are placed in the request value.
 * Bit 16 is placed in bit 0 of the request index.
 */

/* chip type */
enum {
	SIO		= 1,
	FT8U232AM	= 2,
	FT232BM		= 3,
	FT2232C		= 4,
	FTKINDR		= 5,
	FT2232H		= 6,
	FT4232H		= 7,
};

enum {
	 FTb300		= 0,
	 FTb600		= 1,
	 FTb1200	= 2,
	 FTb2400	= 3,
	 FTb4800	= 4,
	 FTb9600	= 5,
	 FTb19200	= 6,
	 FTb38400	= 7,
	 FTb57600	= 8,
	 FTb115200	= 9,
};

/*
 * bRequest: FTSETDATA
 * wValue: Data characteristics
 *	bits 0-7 number of data bits
 * wIndex: Port
 */
enum {
	FTSETDATAParNONE	= 0 << 8,
	FTSETDATAParODD		= 1 << 8,
	FTSETDATAParEVEN	= 2 << 8,
	FTSETDATAParMARK	= 3 << 8,
	FTSETDATAParSPACE	= 4 << 8,
	FTSETDATASTOPBITS1	= 0 << 11,
	FTSETDATASTOPBITS15	= 1 << 11,
	FTSETDATASTOPBITS2	= 2 << 11,
	FTSETBREAK		= 1 << 14,
};

/*
 * bRequest: FTSETMODEMCTRL
 * wValue: ControlValue (see below)
 * wIndex: Port
 */

/*
 * bRequest: FTSETFLOWCTRL
 * wValue: Xoff/Xon
 * wIndex: Protocol/Port - hIndex is protocol; lIndex is port
 */
enum {
	FTDISABLEFLOWCTRL= 0,
	FTRTSCTSHS	= 1 << 8,
	FTDTRDSRHS	= 2 << 8,
	FTXONXOFFHS	= 4 << 8,
};

/*
 * bRequest: FTGETLATENCYTIMER
 * wIndex: Port
 * wLength: 0
 * Data: latency (on return)
 */

/*
 * bRequest: FTSETBITMODE
 * wIndex: Port
 * either it is big bang mode, in which case
 * wValue: 1 byte L is the big bang mode BIG*
 *	or BM is
 * wValue: 1 byte bitbang mode H, 1 byte bitmask for lines L
 */
enum {
	BMSERIAL	= 0,		/* reset, turn off bit-bang mode */

	BIGBMNORMAL	= 1,		/* normal bit-bang mode */
	BIGBMSPI	= 2,		/* spi bit-bang mode */

	BMABM		= 1<<8,		/* async mode */
	BMMPSSE		= 2<<8,
	BMSYNCBB	= 4<<8,		/* sync bit-bang -- 2232x and R-type */
	BMMCU		= 8<<8,		/* MCU Host Bus -- 2232x */
	BMOPTO		= 0x10<<8,	/* opto-isolated<<8, 2232x */
	BMCBUS		= 0x20<<8,	/* CBUS pins of R-type chips */
	BMSYNCFF	= 0x40<<8,	/* Single Channel Sync FIFO, 2232H only */
};

/*
 * bRequest: FTSETLATENCYTIMER
 * wValue: Latency (milliseconds 1-255)
 * wIndex: Port
 */
enum {
	FTLATENCYDEFAULT = 2,
};

/*
 * BmRequestType: SET
 * bRequest: FTSETEVENTCHAR
 * wValue: EventChar
 * wIndex: Port
 * 0-7 lower bits event char
 * 8 enable
 */
enum {
	FTEVCHARENAB = 1<<8,
};

/*
 * BmRequestType: SET
 * bRequest: FTSETERRORCHAR
 * wValue: Error Char
 * wIndex: Port
 * 0-7 lower bits event char
 * 8 enable
 */
enum {
	FTERRCHARENAB = 1<<8,
};
/*
 * BmRequestType: GET
 * bRequest: FTGETMODEMSTATUS
 * wIndex: Port
 * wLength: 1
 * Data: Status
 */
enum {
	FTCTSMASK	= 0x10,
	FTDSRMASK	= 0x20,
	FTRIMASK	= 0x40,
	FTRLSDMASK	= 0x80,
};

enum {
	/* byte 0 of in data hdr */
	FTICTS	= 1 << 4,
	FTIDSR	= 1 << 5,
	FTIRI	= 1 << 6,
	FTIRLSD	= 1 << 7,	/* receive line signal detect */

	/* byte 1 of in data hdr */
	FTIDR	= 1<<0,		/* data ready */
	FTIOE	= 1<<1,		/* overrun error */
	FTIPE	= 1<<2,		/* parity error */
	FTIFE	= 1<<3,		/* framing error */
	FTIBI	= 1<<4,		/* break interrupt */
	FTITHRE	= 1<<5,		/* xmitter holding register */
	FTITEMT	= 1<<6,		/* xmitter empty */
	FTIFIFO	= 1<<7,		/* error in rcv fifo */

	/* byte 0 of out data hdr len does not include byte 0 */
	FTOLENMSK= 0x3F,
	FTOPORT	= 0x80,		/* must be set */
};

extern Serialops ftops;

int	ftmatch(Serial *ser, char *info);
