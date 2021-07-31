
/* Intel 82593 CSMA/CD Core LAN Controller */

/* Port 0 Command Register definitions */

/* Execution operations */
enum {
	OP0_NOP			= 0,	/* CHNL = 0 */
	OP0_SWIT_TO_PORT_1	= 0,	/* CHNL = 1 */
	OP0_IA_SETUP		= 1,
	OP0_CONFIGURE		= 2,
	OP0_MC_SETUP		= 3,
	OP0_TRANSMIT		= 4,
	OP0_TDR			= 5,
	OP0_DUMP		= 6,
	OP0_DIAGNOSE		= 7,
	OP0_TRANSMIT_NO_CRC	= 9,
	OP0_RETRANSMIT		= 12,
	OP0_ABORT		= 13,
/* Reception operations */
	OP0_RCV_ENABLE		= 8,
	OP0_RCV_DISABLE		= 10,
	OP0_STOP_RCV		= 11,
/* Status pointer control operations */
	OP0_FIX_PTR		= 15,	/* CHNL = 1 */
	OP0_RLS_PTR		= 15,	/* CHNL = 0 */
	OP0_RESET		= 14,
};
enum {
	CR0_CHNL		= (1 << 4),	/* 0=Channel 0, 1=Channel 1 */
	CR0_STATUS_0		= 0x00,
	CR0_STATUS_1		= 0x20,
	CR0_STATUS_2		= 0x40,
	CR0_STATUS_3		= 0x60,
	CR0_INT_ACK		= (1 << 7),	/* 0=No ack, 1=acknowledge */
};
/* Port 0 Status Register definitions */
enum {
	SR0_NO_RESULT		= 0,		/* dummy */
	SR0_EVENT_MASK		= 0x0f,
	SR0_IA_SETUP_DONE	= 1,
	SR0_CONFIGURE_DONE	= 2,
	SR0_MC_SETUP_DONE	= 3,
	SR0_TRANSMIT_DONE	= 4,
	SR0_TDR_DONE		= 5,
	SR0_DUMP_DONE		= 6,
	SR0_DIAGNOSE_PASSED	= 7,
	SR0_END_OF_FRAME	= 8,
	SR0_TRANSMIT_NO_CRC_DONE = 9,
	SR0_RETRANSMIT_DONE	= 12,
 	SR0_EXECUTION_ABORTED	= 13,
	SR0_RECEPTION_ABORTED	= 10,
	SR0_DIAGNOSE_FAILED	= 15,
	SR0_STOP_REG_HIT	= 11,

	SR0_CHNL		= (1 << 4),
	SR0_EXECUTION		= (1 << 5),
	SR0_RECEPTION		= (1 << 6),
	SR0_INTERRUPT		= (1 << 7),
};
enum {
	SR3_EXEC_STATE_MASK	= 0x03,
	SR3_EXEC_IDLE		= 0,
	SR3_TX_ABORT_IN_PROGRESS = 1,
	SR3_EXEC_ACTIVE		= 2,
	SR3_ABORT_IN_PROGRESS	= 3,
	SR3_EXEC_CHNL		= (1 << 2),
	SR3_STP_ON_NO_RSRC	= (1 << 3),
	SR3_RCVING_NO_RSRC	= (1 << 4),
	SR3_RCV_STATE_MASK	= 0x60,
	SR3_RCV_IDLE		= 0x00,
	SR3_RCV_READY		= 0x20,
	SR3_RCV_ACTIVE		= 0x40,
	SR3_RCV_STOP_IN_PROG	= 0x60,
	SR3_RCV_CHNL		= (1 << 7),
};
/* Port 1 Command Register definitions */
enum {
	OP1_NOP			= 0,
	OP1_SWIT_TO_PORT_0	= 1,
	OP1_INT_DISABLE		= 2,
	OP1_INT_ENABLE		= 3,
	OP1_SET_TS		= 5,
	OP1_RST_TS		= 7,
	OP1_POWER_DOWN		= 8,
	OP1_RESET_RING_MNGMT	= 11,
	OP1_RESET		= 14,
	OP1_SEL_RST		= 15,
};
enum {
	CR1_STATUS_4		= 0x00,
	CR1_STATUS_5		= 0x20,
	CR1_STATUS_6		= 0x40,
	CR1_STOP_REG_UPDATE	= (1 << 7),
};
/* Receive frame status bits */
enum {
	RX_RCLD			= (1 << 0),
	RX_IA_MATCH		= (1 << 1),
	RX_NO_AD_MATCH		= (1 << 2),
	RX_NO_SFD		= (1 << 3),
	RX_SRT_FRM		= (1 << 7),
	RX_OVRRUN		= (1 << 8),
	RX_ALG_ERR		= (1 << 10),
	RX_CRC_ERR		= (1 << 11),
	RX_LEN_ERR		= (1 << 12),
	RX_RCV_OK		= (1 << 13),
	RX_TYP_LEN		= (1 << 15),
};
/* Transmit status bits */
enum {
	TX_NCOL_MASK		= 0x0f,
	TX_FRTL			= (1 << 4),
	TX_MAX_COL		= (1 << 5),
	TX_HRT_BEAT		= (1 << 6),
	TX_DEFER		= (1 << 7),
	TX_UND_RUN		= (1 << 8),
	TX_LOST_CTS		= (1 << 9),
	TX_LOST_CRS		= (1 << 10),
	TX_LTCOL		= (1 << 11),
	TX_OK			= (1 << 13),
	TX_COLL			= (1 << 15),
};

typedef struct {
	Lock	wlock;
	int	port;				/* port number */
	int	status;				/* interrupt status */	
	int	stop;
	int 	rfp;			
	int	attached;
	Block*	rbp;				/* receive buffer */
	Block*	txbp;				/* FIFO -based transmission */
	int	txbusy;
  	uchar	nwid[2];			/* Network ID */
	long	interrupts;			/* statistics */
	long	upinterrupts;
	long	dninterrupts;
	long	out_packets;
	long	in_packets;
	long	tx_too_long;
	long	tx_DMA_underrun;
	long	tx_carrier_error;
	long	tx_congestion;
	long	tx_heart_beat;
	long	rx_overflow;
	long	rx_overrun;
	long	rx_crc;
	long	rx_no_sfd;
	long 	rx_dropped;
	long 	tx_packets;
	long	rx_packets;
	long	int_errors;
	long	read_errors;
	int	upenabled;
	int	dnenabled;
	int 	waiting;
} Ctlr;


struct i82593_conf_block {
  uchar fifo_limit  : 4,
  	 forgnesi   : 1,
  	 fifo_32    : 1,
  	 d6mod      : 1,
  	 throttle_enb : 1;
  uchar throttle    : 6,
	 cntrxint   : 1,
	 contin	    : 1;
  uchar addr_len    : 3,
  	 acloc 	    : 1,
 	 preamb_len : 2,
  	 loopback   : 2;
  uchar lin_prio    : 3,
	 tbofstop   : 1,
	 exp_prio   : 3,
	 bof_met    : 1;
  uchar	    : 4,
	 ifrm_spc   : 4;
  uchar	    : 5,
	 slottim_low : 3;
  uchar slottim_hi  : 3,
		    : 1,
	 max_retr   : 4;
  uchar prmisc      : 1,
	 bc_dis     : 1,
  		    : 1,
	 crs_1	    : 1,
	 nocrc_ins  : 1,
	 crc_1632   : 1,
  	 	    : 1,
  	 crs_cdt    : 1;
  uchar cs_filter   : 3,
	 crs_src    : 1,
	 cd_filter  : 3,
		    : 1;
  uchar	    	    : 2,
  	 min_fr_len : 6;
  uchar lng_typ     : 1,
	 lng_fld    : 1,
	 rxcrc_xf   : 1,
	 artx	    : 1,
	 sarec	    : 1,
	 tx_jabber  : 1,	/* why is this called max_len in the manual? */
	 hash_1	    : 1,
  	 lbpkpol    : 1;
  uchar	    	    : 6,
  	 fdx	    : 1,
  	  	    : 1;
  uchar dummy_6     : 6,	/* supposed to be ones */
  	 mult_ia    : 1,
  	 dis_bof    : 1;
  uchar dummy_1     : 1,	/* supposed to be one */
	 tx_ifs_retrig : 2,
	 mc_all     : 1,
	 rcv_mon    : 2,
	 frag_acpt  : 1,
  	 tstrttrs   : 1;
  uchar fretx	    : 1,
	 runt_eop   : 1,
	 hw_sw_pin  : 1,
	 big_endn   : 1,
	 syncrqs    : 1,
	 sttlen     : 1,
	 tx_eop     : 1,
  	 rx_eop	    : 1;
  uchar rbuf_size   : 5,
	 rcvstop    : 1,
  	 	    : 2;
};

/* WaveLAN host interface definitions */

#define	LCCR(base)	(base)		/* LAN Controller Command Register */
#define	LCSR(base)	(base)		/* LAN Controller Status Register */
#define	HACR(base)	(base+0x1)	/* Host Adapter Command Register */
#define	HASR(base)	(base+0x1)	/* Host Adapter Status Register */
#define PIORL(base)	(base+0x2)	/* Program I/O Register Low */
#define RPLL(base)	(base+0x2)	/* Receive Pointer Latched Low */
#define PIORH(base)	(base+0x3)	/* Program I/O Register High */
#define RPLH(base)	(base+0x3)	/* Receive Pointer Latched High */
#define PIOP(base)	(base+0x4)	/* Program I/O Port */
#define MMR(base)	(base+0x6)	/* MMI Address Register */
#define MMD(base)	(base+0x7)	/* MMI Data Register */

/* Host Adaptor Command Register bit definitions */
enum {
	HACR_LOF	  = (1 << 3),	/* Lock Out Flag, toggle every 250ms */
	HACR_PWR_STAT	  = (1 << 4),	/* Power State, 1=active, 0=sleep */
	HACR_TX_DMA_RESET = (1 << 5),	/* Reset transmit DMA ptr on high */
	HACR_RX_DMA_RESET = (1 << 6),	/* Reset receive DMA ptr on high */
	HACR_ROM_WEN	  = (1 << 7),	/* EEPROM write enabled when true */

	HACR_RESET      = (HACR_TX_DMA_RESET | HACR_RX_DMA_RESET),
	HACR_DEFAULT	= (HACR_PWR_STAT),
};
/* Host Adapter Status Register bit definitions */
enum {
	HASR_MMI_BUSY	= (1 << 2),	/* MMI is busy when true */
	HASR_LOF	= (1 << 3),	/* Lock out flag status */
	HASR_NO_CLK	= (1 << 4),	/* active when modem not connected */
};
/* Miscellaneous bit definitions */
enum {
	PIORH_SEL_TX	= (1 << 5),	/* PIOR points to 0=rx/1=tx buffer */
	MMR_MMI_WR	= (1 << 0),	/* Next MMI cycle is 0=read, 1=write */
	PIORH_MASK	= 0x1f,		/* only low 5 bits are significant */
	RPLH_MASK	= 0x1f,		/* only low 5 bits are significant */
	MMI_ADDR_MASK	= 0x7e,		/* Bits 1-6 of MMR are significant */
};
/* Attribute Memory map */
enum {
	CIS_ADDR	= 0x0000,	/* Card Information Status Register */
	PSA_ADDR	= 0x0e00,	/* Parameter Storage Area address */
	EEPROM_ADDR	= 0x1000,	/* EEPROM address */
	COR_ADDR	= 0x4000,	/* Configuration Option Register */
};
/* Configuration Option Register bit definitions */
enum {
	COR_CONFIG	= (1 << 0),	/* Config Index, 0 when unconfigured */
	COR_SW_RESET	= (1 << 7),	/* Software Reset on true */
	COR_LEVEL_IRQ	= (1 << 6),	/* Level IRQ */
};
/* Local Memory map */
enum {
	RX_BASE		= 0x0000,	/* Receive memory, 8 kB */
	TX_BASE		= 0x2000,	/* Transmit memory, 2 kB */
	UNUSED_BASE	= 0x2800,	/* Unused, 22 kB */
	RX_SIZE		= (TX_BASE-RX_BASE),	/* Size of receive area */
	RX_SIZE_SHIFT	= 6,		/* Bits to shift in stop register */
};
#define MMI_WRITE(base,cmd,val)	\
	while(inb(HASR(base)) & HASR_MMI_BUSY) ; \
	outb(MMR(base), ((cmd) << 1) | MMR_MMI_WR); \
	outb(MMD(base), va);


#define TRUE  1
#define FALSE 0
#define ETH_ZLEN 64		/* minimum length 64 bytes */

#define WAVELAN_ADDR_SIZE	6	/* Size of a MAC address */

#define SA_ADDR0	0x08	/* First octet of AT&T WaveLAN MAC addr */
#define SA_ADDR1	0x00	/* Second octet of AT&T WaveLAN MAC addr */
#define SA_ADDR2	0x0E	/* Third octet of AT&T WaveLAN MAC addr */

#define WAVELAN_MTU	1500	/* Maximum size of Wavelan packet */

/*
 * Parameter Storage Area (PSA).
 */
typedef struct psa_t	psa_t;
struct psa_t
{
  /* For the PCMCIA Adapter, locations 0x00-0x0F are fixed at 00 */
  	uchar	psa_io_base_addr_1;	/* [0x00] Base address 1 ??? */
  	uchar	psa_io_base_addr_2;	/* [0x01] Base address 2 */
  	uchar	psa_io_base_addr_3;	/* [0x02] Base address 3 */
  	uchar	psa_io_base_addr_4;	/* [0x03] Base address 4 */
  	uchar	psa_rem_boot_addr_1;	/* [0x04] Remote Boot Address 1 */
  	uchar	psa_rem_boot_addr_2;	/* [0x05] Remote Boot Address 2 */
  	uchar	psa_rem_boot_addr_3;	/* [0x06] Remote Boot Address 3 */
  	uchar	psa_holi_params;	/* [0x07] HOst Lan Interface (HOLI) Parameters */
  	uchar	psa_int_req_no;		/* [0x08] Interrupt Request Line */
  	uchar	psa_unused0[7];		/* [0x09-0x0F] unused */
  	uchar	psa_univ_mac_addr[WAVELAN_ADDR_SIZE];	
					/* [0x10-0x15] Universal (factory) MAC Address */
  	uchar	psa_local_mac_addr[WAVELAN_ADDR_SIZE];	/* [0x16-1B] Local MAC Address */
  	uchar	psa_univ_local_sel;	/* [0x1C] Universal Local Selection */
  	uchar	psa_comp_number;	/* [0x1D] Compatability Number: */
  	uchar	psa_thr_pre_set;	/* [0x1E] Modem Threshold Preset */
  	uchar	psa_feature_select;	/* [0x1F] Call code required (1=on) */
  	uchar	psa_subband;		/* [0x20] Subband	*/
  	uchar	psa_quality_thr;	/* [0x21] Modem Quality Threshold */
  	uchar	psa_mod_delay;		/* [0x22] Modem Delay ??? (reserved) */
  	uchar	psa_nwid[2];		/* [0x23-0x24] Network ID */
  	uchar	psa_nwid_select;	/* [0x25] Network ID Select */
  	uchar	psa_encryption_select;	/* [0x26] Encryption On Off */
  	uchar	psa_encryption_key[8];	/* [0x27-0x2E] Encryption Key */
  	uchar	psa_databus_width;	/* [0x2F] AT Dbus width select 8/16 */
  	uchar	psa_call_code[8];	/* [0x30-0x37] (Japan) Call Code */
  	uchar	psa_reserved[4];	/* [0x38-0x3B] Reserved - fixed (00) */
  	uchar	psa_conf_status;	/* [0x3C] Conf Status, bit 0=1:config*/
  	uchar	psa_crc[2];		/* [0x3D] CRC-16 over PSA */
  	uchar	psa_crc_status;		/* [0x3F] CRC Valid Flag */
};
enum {
	PSA_UNIVERSAL	= 0,		/* Universal (factory) */
	PSA_LOCAL	= 1,		/* Local */
};
enum {
	PSA_COMP_PC_AT_915	= 0, 	/* PC-AT 915 MHz	*/
	PSA_COMP_PC_MC_915	= 1, 	/* PC-MC 915 MHz	*/
	PSA_COMP_PC_AT_2400	= 2, 	/* PC-AT 2.4 GHz	*/
	PSA_COMP_PC_MC_2400	= 3, 	/* PC-MC 2.4 GHz	*/
	PSA_COMP_PCMCIA_915	= 4, 	/* PCMCIA 915 MHz	*/
};
enum {
	PSA_SUBBAND_915		= 0,	/* 915 MHz	*/
	PSA_SUBBAND_2425	= 1,	/* 2425 MHz	*/
	PSA_SUBBAND_2460	= 2,	/* 2460 MHz	*/
	PSA_SUBBAND_2484	= 3,	/* 2484 MHz	*/
	PSA_SUBBAND_2430_5	= 4,	/* 2430.5 MHz	*/
};
/*
 * Modem Management Controller (MMC) write structure.
 */
typedef struct mmw_t	mmw_t;
struct mmw_t
{
  	uchar	mmw_encr_key[8];	/* 0x00--07 encryption key */
  	uchar	mmw_encr_enable;	/* 0x08 enable/disable encryption */
  	uchar	mmw_unused0;		/* 0x09 unused */
  	uchar	mmw_des_io_invert;	/* 0x0A ??? */
  	uchar	mmw_unused1[5];		/* 0x0B - 0F unused */
  	uchar	mmw_loopt_sel;		/* 0x10 looptest selection */
  	uchar	mmw_jabber_enable;	/* 0x11 jabber timer enable */
  	uchar	mmw_freeze;		/* 0x12 freeze / unfreeeze signal level */
  	uchar	mmw_anten_sel;		/* 0x13 antenna selection */
  	uchar	mmw_ifs;		/* 0x14 inter frame spacing */
  	uchar	mmw_mod_delay;	 	/* 0x15 modem delay */
  	uchar	mmw_jam_time;		/* 0x16 jamming time */
  	uchar	mmw_unused2;		/* 0x17 unused */
  	uchar	mmw_thr_pre_set;	/* 0x18 level threshold preset */
  	uchar	mmw_decay_prm;		/* 0x19 decay parameters */
  	uchar	mmw_decay_updat_prm;	/* 0x1A decay update parameterz */
  	uchar	mmw_quality_thr;	/* 0x1B quality (z-quotient) threshold */
  	uchar	mmw_netw_id_l;		/* 0x1C NWID low order byte */
  	uchar	mmw_netw_id_h;		/* 0x1D NWID high order byte */
	uchar	mmw_mode_sel;		/* 0x1E	*/
	uchar	mmw_dummy;		/* 0x1F	*/
	uchar	mmw_eectrl;		/* 0x20	 2.4 Gz */
	uchar	mmw_eeaddr;		/* 0x21	 2.4 Gz */
	uchar	mmw_eedatal;		/* 0x22	 2.4 Gz */
	uchar	mmw_eedatah;		/* 0x23	 2.4 Gz */
	uchar	mmw_analctrl;		/* 0x24	 2.4 Gz */

};

enum {  MMC_EECTRL		= 0x20,
	MMC_EEADDR		= 0x21,	
	MMC_ANALCTRL		= 0x24,	
};	
enum {
	MMW_LOOPT_SEL_UNDEFINED	= 0x40,	/* undefined */
	MMW_LOOPT_SEL_INT	= 0x20,	/* activate Attention Request */
	MMW_LOOPT_SEL_LS	= 0x10,	/* looptest w/o collision avoidance */
	MMW_LOOPT_SEL_LT3A	= 0x08,	/* looptest 3a */
	MMW_LOOPT_SEL_LT3B	= 0x04,	/* looptest 3b */
	MMW_LOOPT_SEL_LT3C	= 0x02,	/* looptest 3c */
	MMW_LOOPT_SEL_LT3D	= 0x01,	/* looptest 3d */
};
enum {
	MMW_ANTEN_SEL_SEL	= 0x01,	/* direct antenna selection */
	MMW_ANTEN_SEL_ALG_EN	= 0x02,	/* antenna selection algo. enable */
};

#define	mmwoff(p,f) 	(unsigned short)((void *)(&((mmw_t *)((void *)0 + (p)))->f) - (void *)0)

/*
 * Modem Management Controller (MMC) read structure.
 */
typedef struct mmr_t	mmr_t;
struct mmr_t
{
	uchar	mmr_unused0[8];		/* 0x00..07 unused */
	uchar	mmr_des_status;		/* 0x08 encryption status */
	uchar	mmr_des_avail;		/* 0x09 encryption available (0x55 read) */
	uchar	mmr_des_io_invert;	/* 0x0A des I/O invert register */
	uchar	mmr_unused1[5];		/* 0x0B..0F unused */
	uchar	mmr_dce_status;		/* 0x10 DCE status */
	uchar	mmr_mmr_dsp_id;		/* 0x11 DSP id (AA = Deadalus rev A)
	uchar	mmr_unused2[2];		/* 0x12..13 unused */
	uchar	mmr_correct_nwid_l;	/* 0x14 no. of correct NWID's rxd (low) */
	uchar	mmr_correct_nwid_h;	/* 0x15 no. of correct NWID's rxd (high) */
	uchar	mmr_wrong_nwid_l;	/* 0x16 count of wrong NWID's received (low) */
	uchar	mmr_wrong_nwid_h;	/* 0x17 count of wrong NWID's received (high) */
	uchar	mmr_thr_pre_set;	/* 0x18 level threshold preset */
	uchar	mmr_signal_lvl;		/* 0x19 signal level */
	uchar	mmr_silence_lvl;	/* 0x1A silence level */
	uchar	mmr_sgnl_qual;		/* 0x1B signal quality */
	uchar	mmr_netw_id_l;		/* 0x1C NWID low order byte ??? */
	uchar	mmr_unused3[3];		/* 0x1D..1F unused */
	uchar	mmr_fee_status;		/* 0x20 status of frequency eeprom */
	uchar	mmr_unused4[1];		/* 0x21 */
	uchar	mmr_fee_data_l;		/* 0x22 read data from eeprom (low) */
	uchar	mmr_fee_data_h;		/* 0x23 read data from eeprom (high) */
};
enum {
	MMR_DCE_STATUS_ENERG_DET	= 0x01,	/* energy detected */
	MMR_DCE_STATUS_LOOPT_IND	= 0x02,	/* loop test indicated */
	MMR_DCE_STATUS_XMTITR_IND	= 0x04,	/* transmitter on */
	MMR_DCE_STATUS_JBR_EXPIRED	= 0x08,	/* jabber timer expired */
};
enum {
	MMR_FEE_STATUS_ID 	= 0xF0,	/* modem revision id  */
	MMR_FEE_STATUS_DWLD 	= 0x08,	/* download in progress  */
	MMR_FEE_STATUS_BUSY 	= 0x04,	/* EEprom busy  */
};
enum {
	MMR_SGNL_QUAL_0		= 0x01,	/* signal quality 0 */
	MMR_SGNL_QUAL_1		= 0x02,	/* signal quality 1 */
	MMR_SGNL_QUAL_2		= 0x04,	/* signal quality 2 */
	MMR_SGNL_QUAL_3		= 0x08,	/* signal quality 3 */
	MMR_SGNL_QUAL_S_A	= 0x80,	/* currently selected antenna */
};
#define	MMR_LEVEL_MASK	0x3F

/* fields in MMC registers that relate to EEPROM in WaveMODEM daughtercard */
enum{
	MMC_EECTRL_EEPRE	= 0x10,	/* 2.4 Gz EEPROM Protect Reg Enable */
	MMC_EECTRL_DWLD		= 0x08,	/* 2.4 Gz EEPROM Download Synths   */
	MMC_EECTRL_EEOP		= 0x07,	/* 2.4 Gz EEPROM Opcode mask	 */
	MMC_EECTRL_EEOP_READ	= 0x06,	/* 2.4 Gz EEPROM Read Opcode	 */
	MMC_EEADDR_CHAN		= 0xf0,	/* 2.4 Gz EEPROM Channel # mask	 */
	MMC_EEADDR_WDCNT	= 0x0f,	/* 2.4 Gz EEPROM DNLD WordCount-1 */
	MMC_ANALCTRL_ANTPOL	= 0x02,	/* 2.4 Gz Antenna Polarity mask	 */
	MMC_ANALCTRL_EXTANT	= 0x01,	/* 2.4 Gz External Antenna mask	 */
};
#define	mmroff(p,f) 	(unsigned short)((void *)(&((mmr_t *)((void *)0 + (p)))->f) - (void *)0)

#define	MAXDATAZ		(6 + 6 + 2 + WAVELAN_MTU)

struct wl_cntrs {
  int pkt_arp;
  int pkt_ein[32];
  int pkt_lin[128/8];
  int pkt_eout[32]; 
  int pkt_lout[128/8]; 
};
typedef	struct wl_cntrs *wl_cntrs_t;

#endif /* WAVELAN_H */
