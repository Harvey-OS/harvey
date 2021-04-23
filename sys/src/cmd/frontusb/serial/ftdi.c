/* Future Technology Devices International serial ports */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "usb.h"
#include "serial.h"

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
	FT230XDid	= 0x6015,		/* FTDI FT230X Basic UART */

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

/*
 * BUG: This keeps growing, there has to be a better way, but without
 * devices to try it...  We can probably simply look for FTDI in the
 * string, or use regular expressions somehow.
 */
Cinfo ftinfo[] = {
	{ FTVid, FTACTZWAVEDid },
	{ FTSheevaVid, FTSheevaDid },
	{ FTVid, FTOpenRDUltDid},
	{ FTVid, FTIRTRANSDid },
	{ FTVid, FTIPLUSDid },
	{ FTVid, FTSIODid },
	{ FTVid, FT8U232AMDid },
	{ FTVid, FT8U232AMALTDid },
	{ FTVid, FT8U2232CDid },
	{ FTVid, FTRELAISDid },
	{ INTERBIOMVid, INTERBIOMIOBRDDid },
	{ INTERBIOMVid, INTERBIOMMINIIOBRDDid },
	{ FTVid, FTXF632Did },
	{ FTVid, FTXF634Did },
	{ FTVid, FTXF547Did },
	{ FTVid, FTXF633Did },
	{ FTVid, FTXF631Did },
	{ FTVid, FTXF635Did },
	{ FTVid, FTXF640Did },
	{ FTVid, FTXF642Did },
	{ FTVid, FTDSS20Did },
	{ FTNFRICVid, FTNFRICDid },
	{ FTVid, FTVNHCPCUSBDDid },
	{ FTVid, FTMTXORB0Did },
	{ FTVid, FTMTXORB1Did },
	{ FTVid, FTMTXORB2Did },
	{ FTVid, FTMTXORB3Did },
	{ FTVid, FTMTXORB4Did },
	{ FTVid, FTMTXORB5Did },
	{ FTVid, FTMTXORB6Did },
	{ FTVid, FTPERLEULTRAPORTDid },
	{ FTVid, FTPIEGROUPDid },
	{ SEALEVELVid, SEALEVEL2101Did },
	{ SEALEVELVid, SEALEVEL2102Did },
	{ SEALEVELVid, SEALEVEL2103Did },
	{ SEALEVELVid, SEALEVEL2104Did },
	{ SEALEVELVid, SEALEVEL22011Did },
	{ SEALEVELVid, SEALEVEL22012Did },
	{ SEALEVELVid, SEALEVEL22021Did },
	{ SEALEVELVid, SEALEVEL22022Did },
	{ SEALEVELVid, SEALEVEL22031Did },
	{ SEALEVELVid, SEALEVEL22032Did },
	{ SEALEVELVid, SEALEVEL24011Did },
	{ SEALEVELVid, SEALEVEL24012Did },
	{ SEALEVELVid, SEALEVEL24013Did },
	{ SEALEVELVid, SEALEVEL24014Did },
	{ SEALEVELVid, SEALEVEL24021Did },
	{ SEALEVELVid, SEALEVEL24022Did },
	{ SEALEVELVid, SEALEVEL24023Did },
	{ SEALEVELVid, SEALEVEL24024Did },
	{ SEALEVELVid, SEALEVEL24031Did },
	{ SEALEVELVid, SEALEVEL24032Did },
	{ SEALEVELVid, SEALEVEL24033Did },
	{ SEALEVELVid, SEALEVEL24034Did },
	{ SEALEVELVid, SEALEVEL28011Did },
	{ SEALEVELVid, SEALEVEL28012Did },
	{ SEALEVELVid, SEALEVEL28013Did },
	{ SEALEVELVid, SEALEVEL28014Did },
	{ SEALEVELVid, SEALEVEL28015Did },
	{ SEALEVELVid, SEALEVEL28016Did },
	{ SEALEVELVid, SEALEVEL28017Did },
	{ SEALEVELVid, SEALEVEL28018Did },
	{ SEALEVELVid, SEALEVEL28021Did },
	{ SEALEVELVid, SEALEVEL28022Did },
	{ SEALEVELVid, SEALEVEL28023Did },
	{ SEALEVELVid, SEALEVEL28024Did },
	{ SEALEVELVid, SEALEVEL28025Did },
	{ SEALEVELVid, SEALEVEL28026Did },
	{ SEALEVELVid, SEALEVEL28027Did },
	{ SEALEVELVid, SEALEVEL28028Did },
	{ SEALEVELVid, SEALEVEL28031Did },
	{ SEALEVELVid, SEALEVEL28032Did },
	{ SEALEVELVid, SEALEVEL28033Did },
	{ SEALEVELVid, SEALEVEL28034Did },
	{ SEALEVELVid, SEALEVEL28035Did },
	{ SEALEVELVid, SEALEVEL28036Did },
	{ SEALEVELVid, SEALEVEL28037Did },
	{ SEALEVELVid, SEALEVEL28038Did },
	{ IDTECHVid, IDTECHIDT1221UDid },
	{ OCTVid, OCTUS101Did },
	{ FTVid, FTHETIRA1Did }, /* special quirk div = 240 baud = B38400 rtscts = 1 */
	{ FTVid, FTUSBUIRTDid }, /* special quirk div = 77, baud = B38400 */
	{ FTVid, PROTEGOSPECIAL1 },
	{ FTVid, PROTEGOR2X0 },
	{ FTVid, PROTEGOSPECIAL3 },
	{ FTVid, PROTEGOSPECIAL4 },
	{ FTVid, FTGUDEADSE808Did },
	{ FTVid, FTGUDEADSE809Did },
	{ FTVid, FTGUDEADSE80ADid },
	{ FTVid, FTGUDEADSE80BDid },
	{ FTVid, FTGUDEADSE80CDid },
	{ FTVid, FTGUDEADSE80DDid },
	{ FTVid, FTGUDEADSE80EDid },
	{ FTVid, FTGUDEADSE80FDid },
	{ FTVid, FTGUDEADSE888Did },
	{ FTVid, FTGUDEADSE889Did },
	{ FTVid, FTGUDEADSE88ADid },
	{ FTVid, FTGUDEADSE88BDid },
	{ FTVid, FTGUDEADSE88CDid },
	{ FTVid, FTGUDEADSE88DDid },
	{ FTVid, FTGUDEADSE88EDid },
	{ FTVid, FTGUDEADSE88FDid },
	{ FTVid, FTELVUO100Did },
	{ FTVid, FTELVUM100Did },
	{ FTVid, FTELVUR100Did },
	{ FTVid, FTELVALC8500Did },
	{ FTVid, FTPYRAMIDDid },
	{ FTVid, FTELVFHZ1000PCDid },
	{ FTVid, FTELVCLI7000Did },
	{ FTVid, FTELVPPS7330Did },
	{ FTVid, FTELVTFM100Did },
	{ FTVid, FTELVUDF77Did },
	{ FTVid, FTELVUIO88Did },
	{ FTVid, FTELVUAD8Did },
	{ FTVid, FTELVUDA7Did },
	{ FTVid, FTELVUSI2Did },
	{ FTVid, FTELVT1100Did },
	{ FTVid, FTELVPCD200Did },
	{ FTVid, FTELVULA200Did },
	{ FTVid, FTELVCSI8Did },
	{ FTVid, FTELVEM1000DLDid },
	{ FTVid, FTELVPCK100Did },
	{ FTVid, FTELVRFP500Did },
	{ FTVid, FTELVFS20SIGDid },
	{ FTVid, FTELVWS300PCDid },
	{ FTVid, FTELVFHZ1300PCDid },
	{ FTVid, FTELVWS500Did },
	{ FTVid, LINXSDMUSBQSSDid },
	{ FTVid, LINXMASTERDEVEL2Did },
	{ FTVid, LINXFUTURE0Did },
	{ FTVid, LINXFUTURE1Did },
	{ FTVid, LINXFUTURE2Did },
	{ FTVid, FTCCSICDU200Did },
	{ FTVid, FTCCSICDU401Did },
	{ FTVid, INSIDEACCESSO },
	{ INTREDidVid, INTREDidVALUECANDid },
	{ INTREDidVid, INTREDidNEOVIDid },
	{ FALCOMVid, FALCOMTWISTDid },
	{ FALCOMVid, FALCOMSAMBADid },
	{ FTVid, FTSUUNTOSPORTSDid },
	{ FTVid, FTRMCANVIEWDid },
	{ BANDBVid, BANDBUSOTL4Did },
	{ BANDBVid, BANDBUSTL4Did },
	{ BANDBVid, BANDBUSO9ML2Did },
	{ FTVid, EVERECOPROCDSDid },
	{ FTVid, FT4NGALAXYDE0Did },
	{ FTVid, FT4NGALAXYDE1Did },
	{ FTVid, FT4NGALAXYDE2Did },
	{ FTVid, XSENSCONVERTER0Did },
	{ FTVid, XSENSCONVERTER1Did },
	{ FTVid, XSENSCONVERTER2Did },
	{ FTVid, XSENSCONVERTER3Did },
	{ FTVid, XSENSCONVERTER4Did },
	{ FTVid, XSENSCONVERTER5Did },
	{ FTVid, XSENSCONVERTER6Did },
	{ FTVid, XSENSCONVERTER7Did },
	{ MOBILITYVid, MOBILITYUSBSERIALDid },
	{ FTVid, FTACTIVEROBOTSDid },
	{ FTVid, FTMHAMKWDid },
	{ FTVid, FTMHAMYSDid },
	{ FTVid, FTMHAMY6Did },
	{ FTVid, FTMHAMY8Did },
	{ FTVid, FTMHAMICDid },
	{ FTVid, FTMHAMDB9Did },
	{ FTVid, FTMHAMRS232Did },
	{ FTVid, FTMHAMY9Did },
	{ FTVid, FTTERATRONIKVCPDid },
	{ FTVid, FTTERATRONIKD2XXDid },
	{ EVOLUTIONVid, EVOLUTIONER1Did },
	{ FTVid, FTARTEMISDid },
	{ FTVid, FTATIKATK16Did },
	{ FTVid, FTATIKATK16CDid },
	{ FTVid, FTATIKATK16HRDid },
	{ FTVid, FTATIKATK16HRCDid },
	{ KOBILVid, KOBILCONVB1Did },
	{ KOBILVid, KOBILCONVKAANDid },
	{ POSIFLEXVid, POSIFLEXPP7000Did },
	{ FTVid, FTTTUSBDid },
	{ FTVid, FTECLOCOM1WIREDid },
	{ FTVid, FTWESTREXMODEL777Did },
	{ FTVid, FTWESTREXMODEL8900FDid },
	{ FTVid, FTPCDJDAC2Did },
	{ FTVid, FTRRCIRKITSLOCOBUFFERDid },
	{ FTVid, FTASKRDR400Did },
	{ ICOMID1Vid, ICOMID1Did },
	{ PAPOUCHVid, PAPOUCHTMUDid },
	{ FTVid, FTACGHFDUALDid },
	{ FT8U232AMDid, FT4232HDid },
	{ FTVid, FT230XDid },
	{ FTVid, AMONKEYDid },
	{ 0,	0 },
};

static Serialops ftops;

enum {
	Packsz		= 64,		/* default size */
	Maxpacksz	= 512,
	Bufsiz		= 4 * 1024,
};

static int
ftdiread(Serialport *p, int index, int req, uchar *buf, int len)
{
	int res;
	Serial *ser;

	ser = p->s;

	if(req != FTGETE2READ)
		index |= p->interfc + 1;
	dsprint(2, "serial: ftdiread %#p [%d] req: %#x val: %#x idx:%d buf:%p len:%d\n",
		p, p->interfc, req, 0, index, buf, len);
	res = usbcmd(ser->dev, Rd2h | Rftdireq | Rdev, req, 0, index, buf, len);
	dsprint(2, "serial: ftdiread res:%d\n", res);
	return res;
}

static int
ftdiwrite(Serialport *p, int val, int index, int req)
{
	int res;
	Serial *ser;

	ser = p->s;

	if(req != FTGETE2READ || req != FTSETE2ERASE || req != FTSETBAUDRATE)
		index |= p->interfc + 1;
	dsprint(2, "serial: ftdiwrite %#p [%d] req: %#x val: %#x idx:%d\n",
		p, p->interfc, req, val, index);
	res = usbcmd(ser->dev, Rh2d | Rftdireq | Rdev, req, val, index, nil, 0);
	dsprint(2, "serial: ftdiwrite res:%d\n", res);
	return res;
}

static int
ftmodemctl(Serialport *p, int set)
{
	if(set == 0){
		p->mctl = 0;
		ftdiwrite(p, 0, 0, FTSETMODEMCTRL);
		return 0;
	}
	p->mctl = 1;
	ftdiwrite(p, 0, FTRTSCTSHS, FTSETFLOWCTRL);
	return 0;
}

static ushort
ft232ambaudbase2div(int baud, int base)
{
	int divisor3;
	ushort divisor;

	divisor3 = (base / 2) / baud;
	if((divisor3 & 7) == 7)
		divisor3++;			/* round x.7/8 up to x+1 */
	divisor = divisor3 >> 3;
	divisor3 &= 7;

	if(divisor3 == 1)
		divisor |= 0xc000;		/*	0.125 */
	else if(divisor3 >= 4)
		divisor |= 0x4000;		/*	0.5	*/
	else if(divisor3 != 0)
		divisor |= 0x8000;		/*	0.25	*/
	if( divisor == 1)
		divisor = 0;		/* special case for maximum baud rate */
	return divisor;
}

enum{
	ClockNew	= 48000000,
	ClockOld	= 12000000 / 16,
	HetiraDiv	= 240,
	UirtDiv		= 77,
};

static ushort
ft232ambaud2div(int baud)
{
	return ft232ambaudbase2div(baud, ClockNew);
}

static ulong divfrac[8] = { 0, 3, 2, 4, 1, 5, 6, 7};

static ulong
ft232bmbaudbase2div(int baud, int base)
{
	int divisor3;
	u32int divisor;

	divisor3 = (base / 2) / baud;
	divisor = divisor3 >> 3 | divfrac[divisor3 & 7] << 14;

	/* Deal with special cases for highest baud rates. */
	if( divisor == 1)
		divisor = 0;			/* 1.0 */
	else if( divisor == 0x4001)
		divisor = 1;			/* 1.5 */
	return divisor;
}

static ulong
ft232bmbaud2div (int baud)
{
	return ft232bmbaudbase2div (baud, ClockNew);
}

static int
customdiv(Serial *ser)
{
	if(ser->dev->usb->vid == FTVid && ser->dev->usb->did == FTHETIRA1Did)
		return HetiraDiv;
	else if(ser->dev->usb->vid == FTVid && ser->dev->usb->did == FTUSBUIRTDid)
		return UirtDiv;

	fprint(2, "serial: weird custom divisor\n");
	return 0;		/* shouldn't happen, break as much as I can */
}

static ulong
ftbaudcalcdiv(Serial *ser, int baud)
{
	int cusdiv;
	ulong divval;

	if(baud == 38400 && (cusdiv = customdiv(ser)) != 0)
		baud = ser->baudbase / cusdiv;

	if(baud == 0)
		baud = 9600;

	switch(ser->type) {
	case SIO:
		switch(baud) {
		case 300:
			divval = FTb300;
			break;
		case 600:
			divval = FTb600;
			break;
		case 1200:
			divval = FTb1200;
			break;
		case 2400:
			divval = FTb2400;
			break;
		case 4800:
			divval = FTb4800;
			break;
		case 9600:
			divval = FTb9600;
			break;
		case 19200:
			divval = FTb19200;
			break;
		case 38400:
			divval = FTb38400;
			break;
		case 57600:
			divval = FTb57600;
			break;
		case 115200:
			divval = FTb115200;
			break;
		default:
			divval = FTb9600;
			break;
		}
		break;
	case FT8U232AM:
		if(baud <= 3000000)
			divval = ft232ambaud2div(baud);
		else
			divval = ft232ambaud2div(9600);
		break;
	case FT232BM:
	case FT2232C:
	case FTKINDR:
	case FT2232H:
	case FT4232H:
		if(baud <= 3000000)
			divval = ft232bmbaud2div(baud);
		else
			divval = ft232bmbaud2div(9600);
		break;
	default:
		divval = ft232bmbaud2div(9600);
		break;
	}
	return divval;
}

static int
ftsetparam(Serialport *p)
{
	int res;
	ushort val;
	ulong bauddiv;

	val = 0;
	if(p->stop == 1)
		val |= FTSETDATASTOPBITS1;
	else if(p->stop == 2)
		val |= FTSETDATASTOPBITS2;
	else if(p->stop == 15)
		val |= FTSETDATASTOPBITS15;
	switch(p->parity){
	case 0:
		val |= FTSETDATAParNONE;
		break;
	case 1:
		val |= FTSETDATAParODD;
		break;
	case 2:
		val |= FTSETDATAParEVEN;
		break;
	case 3:
		val |= FTSETDATAParMARK;
		break;
	case 4:
		val |= FTSETDATAParSPACE;
		break;
	};

	dsprint(2, "serial: setparam\n");

	res = ftdiwrite(p, val, 0, FTSETDATA);
	if(res < 0)
		return res;

	res = ftmodemctl(p, p->mctl);
	if(res < 0)
		return res;

	bauddiv = ftbaudcalcdiv(p->s, p->baud);
	res = ftdiwrite(p, bauddiv, (bauddiv>>16) & 1, FTSETBAUDRATE);

	dsprint(2, "serial: setparam res: %d\n", res);
	return res;
}

static int
hasjtag(Usbdev *udev)
{
	/* no string, for now, by default we detect no jtag */
	if(udev->product != nil && cistrstr(udev->product, "jtag") != nil)
		return 1;
	/* blank aijuboard has jtag for initial bringup */
	if(udev->csp == 0xffffff)
		return 1;
	return 0;
}

/* ser locked */
static void
ftgettype(Serial *ser)
{
	int i, outhdrsz, dno, pksz;
	ulong baudbase;
	Conf *cnf;

	pksz = Packsz;
	/* Assume it is not the original SIO device for now. */
	baudbase = ClockNew / 2;
	outhdrsz = 0;
	dno = ser->dev->usb->dno;
	cnf = ser->dev->usb->conf[0];
	ser->nifcs = 0;
	for(i = 0; i < Niface; i++)
		if(cnf->iface[i] != nil)
			ser->nifcs++;
	if(ser->nifcs > 1) {
		/*
		 * Multiple interfaces.  default assume FT2232C,
		 */
		if(dno == 0x500)
			ser->type = FT2232C;
		else if(dno == 0x600)
			ser->type = FTKINDR;
		else if(dno == 0x700){
			ser->type = FT2232H;
			pksz = Maxpacksz;
		} else if(dno == 0x800){
			ser->type = FT4232H;
			pksz = Maxpacksz;
		} else
			ser->type = FT2232C;

		if(hasjtag(ser->dev->usb))
			ser->jtag = 0;

		/*
		 * BM-type devices have a bug where dno gets set
		 * to 0x200 when serial is 0.
		 */
		if(dno < 0x500)
			fprint(2, "serial: warning: dno %d too low for "
				"multi-interface device\n", dno);
	} else if(dno < 0x200) {
		/* Old device.  Assume it is the original SIO. */
		ser->type = SIO;
		baudbase = ClockOld/16;
		outhdrsz = 1;
	} else if(dno < 0x400)
		/*
		 * Assume its an FT8U232AM (or FT8U245AM)
		 * (It might be a BM because of the iSerialNumber bug,
		 * but it will still work as an AM device.)
		 */
		ser->type = FT8U232AM;
	else			/* Assume it is an FT232BM (or FT245BM) */
		ser->type = FT232BM;

	ser->maxrtrans = ser->maxwtrans = pksz;
	ser->baudbase = baudbase;
	ser->outhdrsz = outhdrsz;
	ser->inhdrsz = 2;
	ser->Serialops = ftops;

	dsprint (2, "serial: detected type: %#x\n", ser->type);
}

int
ftprobe(Serial *ser)
{
	Usbdev *ud = ser->dev->usb;

	if(matchid(ftinfo, ud->vid, ud->did) == nil)
		return -1;
	ftgettype(ser);
	return 0;
}

static int
ftuseinhdr(Serialport *p, uchar *b)
{
	if(b[0] & FTICTS)
		p->cts = 1;
	else
		p->cts = 0;
	if(b[0] & FTIDSR)
		p->dsr = 1;
	else
		p->dsr = 0;
	if(b[0] & FTIRI)
		p->ring = 1;
	else
		p->ring = 0;
	if(b[0] & FTIRLSD)
		p->rlsd = 1;
	else
		p->rlsd = 0;

	if(b[1] & FTIOE)
		p->novererr++;
	if(b[1] & FTIPE)
		p->nparityerr++;
	if(b[1] & FTIFE)
		p->nframeerr++;
	if(b[1] & FTIBI)
		p->nbreakerr++;
	return 0;
}

static int
ftsetouthdr(Serialport *p, uchar *b, int len)
{
	if(p->s->outhdrsz != 0)
		b[0] = FTOPORT | (FTOLENMSK & len);
	return p->s->outhdrsz;
}

static int
wait4data(Serialport *p, uchar *data, int count)
{
	int d;
	Serial *ser;

	ser = p->s;

	qunlock(ser);
	d = sendul(p->w4data, 1);
	qlock(ser);
	if(d <= 0)
		return -1;
	if(p->ndata >= count)
		p->ndata -= count;
	else{
		count = p->ndata;
		p->ndata = 0;
	}
	memmove(data, p->data, count);
	if(p->ndata != 0)
		memmove(p->data, p->data+count, p->ndata);
	recvul(p->gotdata);
	return count;
}

static int
wait4write(Serialport *p, uchar *data, int count)
{
	int off, fd;
	uchar *b;
	Serial *ser;

	ser = p->s;

	b = emallocz(count+ser->outhdrsz, 1);
	off = ftsetouthdr(p, b, count);
	memmove(b+off, data, count);

	fd = p->epout->dfd;
	qunlock(ser);
	count = write(fd, b, count+off);
	qlock(ser);
	free(b);
	return count;
}

typedef struct Packser Packser;
struct Packser{
	int	nb;
	uchar	b[Bufsiz];
};

typedef struct Areader Areader;
struct Areader{
	Serialport	*p;
	Channel	*c;
};

static void
shutdownchan(Channel *c)
{
	Packser *bp;

	while((bp=nbrecvp(c)) != nil)
		free(bp);
	chanfree(c);
}

int
cpdata(Serial *ser, Serialport *port, uchar *out, uchar *in, int sz)
{
	int i, ncp, ntotcp, pksz;

	pksz = ser->maxrtrans;
	ntotcp = 0;

	for(i = 0; i < sz; i+= pksz){
		ftuseinhdr(port, in + i);
		if(sz - i > pksz)
			ncp = pksz - ser->inhdrsz;
		else
			ncp = sz - i - ser->inhdrsz;
		memmove(out, in + i + ser->inhdrsz, ncp);
		out += ncp;
		ntotcp += ncp;
	}
	return ntotcp;
}

static void
epreader(void *u)
{
	int dfd, rcount, cl, ntries, recov;
	Areader *a;
	Channel *c;
	Packser *pk;
	Serial *ser;
	Serialport *p;

	threadsetname("epreader proc");
	a = u;
	p = a->p;
	ser = p->s;
	c = a->c;
	free(a);

	qlock(ser);	/* this makes the reader wait end of initialization too */
	dfd = p->epin->dfd;
	qunlock(ser);

	ntries = 0;
	pk = nil;
	for(;;) {
		if (pk == nil)
			pk = emallocz(sizeof(Packser), 1);
Eagain:
		rcount = read(dfd, pk->b, sizeof pk->b);
		if(serialdebug > 5)
			dsprint(2, "%d %#ux%#ux ", rcount, p->data[0],
				p->data[1]);

		if(rcount < 0){
			if(ntries++ > 100)
				break;
			qlock(ser);
			recov = serialrecover(ser, p, nil, "epreader: bulkin error");
			qunlock(ser);
			if(recov >= 0)
				goto Eagain;
		}
		if(rcount == 0)
			continue;
		if(rcount >= ser->inhdrsz){
			rcount = cpdata(ser, p, pk->b, pk->b, rcount);
			if(rcount != 0){
				pk->nb = rcount;
				cl = sendp(c, pk);
				if(cl < 0){
					/*
					 * if it was a time-out, I don't want
					 * to give back an error.
					 */
					rcount = 0;
					break;
				}
			}else
				free(pk);
			qlock(ser);
			ser->recover = 0;
			qunlock(ser);
			ntries = 0;
			pk = nil;
		}
	}

	if(rcount < 0)
		fprint(2, "%s: error reading %s: %r\n", argv0, p->name);
	free(pk);
	nbsendp(c, nil);
	if(p->w4data != nil)
		chanclose(p->w4data);
	if(p->gotdata != nil)
		chanclose(p->gotdata);
	devctl(ser->dev, "detach");
	closedev(ser->dev);
}

static void
statusreader(void *u)
{
	Areader *a;
	Channel *c;
	Packser *pk;
	Serialport *p;
	Serial *ser;
	int cl;

	p = u;
	ser = p->s;
	threadsetname("statusreader thread");
	/* big buffering, fewer bytes lost */
	c = chancreate(sizeof(Packser *), 128);
	a = emallocz(sizeof(Areader), 1);
	a->p = p;
	a->c = c;
	incref(ser->dev);
	proccreate(epreader, a, 16*1024);

	while((pk = recvp(c)) != nil){
		memmove(p->data, pk->b, pk->nb);
		p->ndata = pk->nb;
		free(pk);
		/* consume it all */
		while(p->ndata != 0){
			cl = recvul(p->w4data);
			if(cl < 0)
				break;
			cl = sendul(p->gotdata, 1);
			if(cl < 0)
				break;
		}
	}

	shutdownchan(c);
	devctl(ser->dev, "detach");
	closedev(ser->dev);
}

static int
ftreset(Serial *ser, Serialport *p)
{
	int i;

	if(p != nil){
		ftdiwrite(p, FTRESETCTLVAL, 0, FTRESET);
		return 0;
	}
	p = ser->p;
	for(i = 0; i < Maxifc; i++)
		if(p[i].s != nil)
			ftdiwrite(&p[i], FTRESETCTLVAL, 0, FTRESET);
	return 0;
}

static int
ftinit(Serialport *p)
{
	Serial *ser;
	uint timerval;
	int res;

	ser = p->s;
	if(p->isjtag){
		res = ftdiwrite(p, FTSETFLOWCTRL, 0, FTDISABLEFLOWCTRL);
		if(res < 0)
			return -1;
		res = ftdiread(p, FTSETLATENCYTIMER, 0, (uchar *)&timerval,
			FTLATENCYTIMERSZ);
		if(res < 0)
			return -1;
		dsprint(2, "serial: jtag latency timer is %d\n", timerval);
		timerval = 2;
		ftdiwrite(p, FTLATENCYDEFAULT, 0, FTSETLATENCYTIMER);
		res = ftdiread(p, FTSETLATENCYTIMER, 0, (uchar *)&timerval,
			FTLATENCYTIMERSZ);
		if(res < 0)
			return -1;

		dsprint(2, "serial: jtag latency timer set to %d\n", timerval);
		/* 0xb is the mask for lines. plug dependant? */
		ftdiwrite(p, BMMPSSE|0x0b, 0, FTSETBITMODE);
	}
	incref(ser->dev);
	proccreate(statusreader, p, 8*1024);
	return 0;
}

static int
ftsetbreak(Serialport *p, int val)
{
	return ftdiwrite(p, (val != 0? FTSETBREAK: 0), 0, FTSETDATA);
}

static int
ftclearpipes(Serialport *p)
{
	/* maybe can be done in one... */
	ftdiwrite(p, FTRESETCTLVALPURGETX, 0, FTRESET);
	ftdiwrite(p, FTRESETCTLVALPURGERX, 0, FTRESET);
	return 0;
}

static int
setctlline(Serialport *p, uchar val)
{
	return ftdiwrite(p, val | (val << 8), 0, FTSETMODEMCTRL);
}

static void
updatectlst(Serialport *p, int val)
{
	if(p->rts)
		p->ctlstate |= val;
	else
		p->ctlstate &= ~val;
}

static int
setctl(Serialport *p)
{
	int res;
	Serial *ser;

	ser = p->s;

	if(ser->dev->usb->vid == FTVid && ser->dev->usb->did ==  FTHETIRA1Did){
		fprint(2, "serial: cannot set lines for this device\n");
		updatectlst(p, CtlRTS|CtlDTR);
		p->rts = p->dtr = 1;
		return -1;
	}

	/* NB: you can not set DTR and RTS with one control message */
	updatectlst(p, CtlRTS);
	res = setctlline(p, (CtlRTS<<8)|p->ctlstate);
	if(res < 0)
		return res;

	updatectlst(p, CtlDTR);
	res = setctlline(p, (CtlDTR<<8)|p->ctlstate);
	if(res < 0)
		return res;

	return 0;
}

static int
ftsendlines(Serialport *p)
{
	int res;

	dsprint(2, "serial: sendlines: %#2.2x\n", p->ctlstate);
	res = setctl(p);
	dsprint(2, "serial: sendlines res: %d\n", res);
	return 0;
}

static int
ftseteps(Serialport *p)
{
	char *s;
	Serial *ser;

	ser = p->s;

	s = smprint("maxpkt %d", ser->maxrtrans);
	devctl(p->epin, s);
	free(s);

	s = smprint("maxpkt %d", ser->maxwtrans);
	devctl(p->epout, s);
	free(s);
	return 0;
}

static Serialops ftops = {
	.init		= ftinit,
	.seteps		= ftseteps,
	.setparam	= ftsetparam,
	.clearpipes	= ftclearpipes,
	.reset		= ftreset,
	.sendlines	= ftsendlines,
	.modemctl	= ftmodemctl,
	.setbreak	= ftsetbreak,
	.wait4data	= wait4data,
	.wait4write	= wait4write,
};
