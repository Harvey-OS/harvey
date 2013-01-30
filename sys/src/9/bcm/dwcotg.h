/*
 * USB host driver for BCM2835
 *	Synopsis DesignWare Core USB 2.0 OTG controller
 *
 * Device register definitions
 */

typedef unsigned int Reg;
typedef struct Dwcregs Dwcregs;
typedef struct Hostchan Hostchan;

enum {
	Maxchans	= 16,	/* actual number of channels in ghwcfg2 */
};

struct Dwcregs {
	/* Core global registers 0x000-0x140 */
	Reg	gotgctl;	/* OTG Control and Status */
	Reg	gotgint;	/* OTG Interrupt */
	Reg	gahbcfg;	/* Core AHB Configuration */
	Reg	gusbcfg;	/* Core USB Configuration */
	Reg	grstctl;	/* Core Reset */
	Reg	gintsts;	/* Core Interrupt */
	Reg	gintmsk;	/* Core Interrupt Mask */
	Reg	grxstsr;	/* Receive Status Queue Read (RO) */
	Reg	grxstsp;	/* Receive Status Queue Read & POP (RO) */
	Reg	grxfsiz;	/* Receive FIFO Size */
	Reg	gnptxfsiz;	/* Non Periodic Transmit FIFO Size */
	Reg	gnptxsts;	/* Non Periodic Transmit FIFO/Queue Status (RO) */
	Reg	gi2cctl;	/* I2C Access */
	Reg	gpvndctl;	/* PHY Vendor Control */
	Reg	ggpio;		/* General Purpose Input/Output */
	Reg	guid;		/* User ID */
	Reg	gsnpsid;	/* Synopsys ID (RO) */
	Reg	ghwcfg1;	/* User HW Config1 (RO) (DEVICE) */
	Reg	ghwcfg2;	/* User HW Config2 (RO) */
	Reg	ghwcfg3;	/* User HW Config3 (RO) */
	Reg	ghwcfg4;	/* User HW Config4 (RO)*/
	Reg	glpmcfg;	/* Core LPM Configuration */
	Reg	gpwrdn;		/* Global PowerDn */
	Reg	gdfifocfg;	/* Global DFIFO SW Config (DEVICE?) */
	Reg	adpctl;		/* ADP Control */
	Reg	reserved0[39];
	Reg	hptxfsiz;	/* Host Periodic Transmit FIFO Size */
	Reg	dtxfsiz[15];	/* Device Periodic Transmit FIFOs (DEVICE) */
	char	pad0[0x400-0x140];

	/* Host global registers 0x400-0x420 */
	Reg	hcfg;		/* Configuration */
	Reg	hfir;		/* Frame Interval */
	Reg	hfnum;		/* Frame Number / Frame Remaining (RO) */
	Reg	reserved1;
	Reg	hptxsts;	/* Periodic Transmit FIFO / Queue Status */
	Reg	haint;		/* All Channels Interrupt */
	Reg	haintmsk;	/* All Channels Interrupt Mask */
	Reg	hflbaddr;	/* Frame List Base Address */
	char	pad1[0x440-0x420];

	/* Host port register 0x440 */
	Reg	hport0;		/* Host Port 0 Control and Status */
	char	pad2[0x500-0x444];

	/* Host channel specific registers 0x500-0x700 */
	struct	Hostchan {
		Reg	hcchar;	/* Characteristic */
		Reg	hcsplt;	/* Split Control */
		Reg	hcint;	/* Interrupt */
		Reg	hcintmsk; /* Interrupt Mask */
		Reg	hctsiz;	/* Transfer Size */
		Reg	hcdma;	/* DMA Address */
		Reg	reserved;
		Reg	hcdmab;	/* DMA Buffer Address */
	} hchan[Maxchans];
	char	pad3[0xE00-0x700];

	/* Power & clock gating control register 0xE00 */
	Reg	pcgcctl;
};

enum {
	/* gotgctl */
	Sesreqscs	= 1<<0,
	Sesreq		= 1<<1,
	Vbvalidoven	= 1<<2,
	Vbvalidovval	= 1<<3,
	Avalidoven	= 1<<4,
	Avalidovval	= 1<<5,
	Bvalidoven	= 1<<6,
	Bvalidovval	= 1<<7,
	Hstnegscs	= 1<<8,
	Hnpreq		= 1<<9,
	Hstsethnpen	= 1<<10,
	Devhnpen	= 1<<11,
	Conidsts	= 1<<16,
	Dbnctime	= 1<<17,
	Asesvld		= 1<<18,
	Bsesvld		= 1<<19,
	Otgver		= 1<<20,
	Multvalidbc	= 0x1F<<22,
	Chirpen		= 1<<27,

	/* gotgint */
	Sesenddet	= 1<<2,
	Sesreqsucstschng= 1<<8,
	Hstnegsucstschng= 1<<9,
	Hstnegdet	= 1<<17,
	Adevtoutchng	= 1<<18,
	Debdone		= 1<<19,
	Mvic		= 1<<20,

	/* gahbcfg */
	Glblintrmsk	= 1<<0,
	/* bits 1:4 redefined for BCM2835 */
	Axiburstlen	= 0x3<<1,
		BURST1		= 3<<1,
		BURST2		= 2<<1,
		BURST3		= 1<<1,
		BURST4		= 0<<1,
	Axiwaitwrites	= 1<<4,
	Dmaenable	= 1<<5,
	Nptxfemplvl	= 1<<7,
		NPTX_HALFEMPTY	= 0<<7,
		NPTX_EMPTY	= 1<<7,
	Ptxfemplvl	= 1<<8,
		PTX_HALFEMPTY	= 0<<8,
		PTX_EMPTY	= 1<<8,
	Remmemsupp	= 1<<21,
	Notialldmawrit	= 1<<22,
	Ahbsingle	= 1<<23,

	/* gusbcfg */
	Toutcal		= 0x7<<0,
	Phyif		= 1<<3,
	Ulpi_utmi_sel	= 1<<4,
	Fsintf		= 1<<5,
		FsUnidir	= 0<<5,
		FsBidir		= 1<<5,
	Physel		= 1<<6,
		PhyHighspeed	= 0<<6,
		PhyFullspeed	= 1<<6,
	Ddrsel		= 1<<7,
	Srpcap		= 1<<8,
	Hnpcap		= 1<<9,
	Usbtrdtim	= 0xf<<10,
		OUsbtrdtim		= 10,
	Phylpwrclksel	= 1<<15,
	Otgutmifssel	= 1<<16,
	Ulpi_fsls	= 1<<17,
	Ulpi_auto_res	= 1<<18,
	Ulpi_clk_sus_m	= 1<<19,
	Ulpi_ext_vbus_drv= 1<<20,
	Ulpi_int_vbus_indicator= 1<<21,
	Term_sel_dl_pulse= 1<<22,
	Indicator_complement= 1<<23,
	Indicator_pass_through= 1<<24,
	Ulpi_int_prot_dis= 1<<25,
	Ic_usb_cap	= 1<<26,
	Ic_traffic_pull_remove= 1<<27,
	Tx_end_delay	= 1<<28,
	Force_host_mode	= 1<<29,
	Force_dev_mode	= 1<<30,

	/* grstctl */
	Csftrst		= 1<<0,
	Hsftrst		= 1<<1,
	Hstfrm		= 1<<2,
	Intknqflsh	= 1<<3,
	Rxfflsh		= 1<<4,
	Txfflsh		= 1<<5,
	Txfnum		= 0x1f<<6,
		TXF_ALL		= 0x10<<6,
	Dmareq		= 1<<30,
	Ahbidle		= 1<<31,

	/* gintsts, gintmsk */
	Curmode		= 1<<0,
		HOSTMODE	= 1<<0,
		DEVMODE		= 0<<0,
	Modemismatch	= 1<<1,
	Otgintr		= 1<<2,
	Sofintr		= 1<<3,
	Rxstsqlvl	= 1<<4,
	Nptxfempty	= 1<<5,
	Ginnakeff	= 1<<6,
	Goutnakeff	= 1<<7,
	Ulpickint	= 1<<8,
	I2cintr		= 1<<9,
	Erlysuspend	= 1<<10,
	Usbsuspend	= 1<<11,
	Usbreset	= 1<<12,
	Enumdone	= 1<<13,
	Isooutdrop	= 1<<14,
	Eopframe	= 1<<15,
	Restoredone	= 1<<16,
	Epmismatch	= 1<<17,
	Inepintr	= 1<<18,
	Outepintr	= 1<<19,
	Incomplisoin	= 1<<20,
	Incomplisoout	= 1<<21,
	Fetsusp		= 1<<22,
	Resetdet	= 1<<23,
	Portintr	= 1<<24,
	Hcintr		= 1<<25,
	Ptxfempty	= 1<<26,
	Lpmtranrcvd	= 1<<27,
	Conidstschng	= 1<<28,
	Disconnect	= 1<<29,
	Sessreqintr	= 1<<30,
	Wkupintr	= 1<<31,

	/* grxsts[rp] */
	Chnum		= 0xf<<0,
	Bcnt		= 0x7ff<<4,
	Dpid		= 0x3<<15,
	Pktsts		= 0xf<<17,
		PKTSTS_IN		= 2<<17,
		PKTSTS_IN_XFER_COMP	= 3<<17,
		PKTSTS_DATA_TOGGLE_ERR	= 5<<17,
		PKTSTS_CH_HALTED	= 7<<17,

	/* hptxfsiz, gnptxfsiz */
	Startaddr	= 0xffff<<0,
	Depth		= 0xffff<<16,
		ODepth		= 16,

	/* gnptxsts */
	Nptxfspcavail	= 0xffff<<0,
	Nptxqspcavail	= 0xff<<16,
	Nptxqtop_terminate= 1<<24,
	Nptxqtop_token	= 0x3<<25,
	Nptxqtop_chnep	= 0xf<<27,

	/* gpvndctl */
	Regdata		= 0xff<<0,
	Vctrl		= 0xff<<8,
	Regaddr16_21	= 0x3f<<16,
	Regwr		= 1<<22,
	Newregreq	= 1<<25,
	Vstsbsy		= 1<<26,
	Vstsdone	= 1<<27,
	Disulpidrvr	= 1<<31,

	/* ggpio */
	Gpi		= 0xffff<<0,
	Gpo		= 0xffff<<16,

	/* ghwcfg2 */
	Op_mode		= 0x7<<0,
		HNP_SRP_CAPABLE_OTG	= 0<<0,
		SRP_ONLY_CAPABLE_OTG	= 1<<0,
		NO_HNP_SRP_CAPABLE	= 2<<0,
		SRP_CAPABLE_DEVICE	= 3<<0,
		NO_SRP_CAPABLE_DEVICE	= 4<<0,
		SRP_CAPABLE_HOST	= 5<<0,
		NO_SRP_CAPABLE_HOST	= 6<<0,
	Architecture	= 0x3<<3,
		SLAVE_ONLY		= 0<<3,
		EXT_DMA			= 1<<3,
		INT_DMA			= 2<<3,
	Point2point	= 1<<5,
	Hs_phy_type	= 0x3<<6,
		PHY_NOT_SUPPORTED	= 0<<6,
		PHY_UTMI		= 1<<6,
		PHY_ULPI		= 2<<6,
		PHY_UTMI_ULPI		= 3<<6,
	Fs_phy_type	= 0x3<<8,
	Num_dev_ep	= 0xf<<10,
	Num_host_chan	= 0xf<<14,
		ONum_host_chan		= 14,
	Perio_ep_supported= 1<<18,
	Dynamic_fifo	= 1<<19,
	Nonperio_tx_q_depth= 0x3<<22,
	Host_perio_tx_q_depth= 0x3<<24,
	Dev_token_q_depth= 0x1f<<26,
	Otg_enable_ic_usb= 1<<31,

	/* ghwcfg3 */
	Xfer_size_cntr_width	= 0xf<<0,
	Packet_size_cntr_width	= 0x7<<4,
	Otg_func		= 1<<7,
	I2c			= 1<<8,
	Vendor_ctrl_if		= 1<<9,
	Optional_features	= 1<<10,
	Synch_reset_type	= 1<<11,
	Adp_supp		= 1<<12,
	Otg_enable_hsic		= 1<<13,
	Bc_support		= 1<<14,
	Otg_lpm_en		= 1<<15,
	Dfifo_depth		= 0xffff<<16,
		ODfifo_depth		= 16,

	/* ghwcfg4 */
	Num_dev_perio_in_ep	= 0xf<<0,
	Power_optimiz		= 1<<4,
	Min_ahb_freq		= 1<<5,
	Hiber			= 1<<6,
	Xhiber			= 1<<7,
	Utmi_phy_data_width	= 0x3<<14,
	Num_dev_mode_ctrl_ep	= 0xf<<16,
	Iddig_filt_en		= 1<<20,
	Vbus_valid_filt_en	= 1<<21,
	A_valid_filt_en		= 1<<22,
	B_valid_filt_en		= 1<<23,
	Session_end_filt_en	= 1<<24,
	Ded_fifo_en		= 1<<25,
	Num_in_eps		= 0xf<<26,
	Desc_dma		= 1<<30,
	Desc_dma_dyn		= 1<<31,

	/* glpmcfg */
	Lpm_cap_en	= 1<<0,
	Appl_resp	= 1<<1,
	Hird		= 0xf<<2,
	Rem_wkup_en	= 1<<6,
	En_utmi_sleep	= 1<<7,
	Hird_thres	= 0x1f<<8,
	Lpm_resp	= 0x3<<13,
	Prt_sleep_sts	= 1<<15,
	Sleep_state_resumeok= 1<<16,
	Lpm_chan_index	= 0xf<<17,
	Retry_count	= 0x7<<21,
	Send_lpm	= 1<<24,
	Retry_count_sts	= 0x7<<25,
	Hsic_connect	= 1<<30,
	Inv_sel_hsic	= 1<<31,

	/* gpwrdn */
	Pmuintsel	= 1<<0,
	Pmuactv		= 1<<1,
	Restore		= 1<<2,
	Pwrdnclmp	= 1<<3,
	Pwrdnrstn	= 1<<4,
	Pwrdnswtch	= 1<<5,
	Dis_vbus	= 1<<6,
	Lnstschng	= 1<<7,
	Lnstchng_msk	= 1<<8,
	Rst_det		= 1<<9,
	Rst_det_msk	= 1<<10,
	Disconn_det	= 1<<11,
	Disconn_det_msk	= 1<<12,
	Connect_det	= 1<<13,
	Connect_det_msk	= 1<<14,
	Srp_det		= 1<<15,
	Srp_det_msk	= 1<<16,
	Sts_chngint	= 1<<17,
	Sts_chngint_msk	= 1<<18,
	Linestate	= 0x3<<19,
	Idsts		= 1<<21,
	Bsessvld	= 1<<22,
	Adp_int		= 1<<23,
	Mult_val_id_bc	= 0x1f<<24,

	/* gdfifocfg */
	Gdfifocfg	= 0xffff<<0,
	Epinfobase	= 0xffff<<16,

	/* adpctl */
	Prb_dschg	= 0x3<<0,
	Prb_delta	= 0x3<<2,
	Prb_per		= 0x3<<4,
	Rtim		= 0x7ff<<6,
	Enaprb		= 1<<17,
	Enasns		= 1<<18,
	Adpres		= 1<<19,
	Adpen		= 1<<20,
	Adp_prb_int	= 1<<21,
	Adp_sns_int	= 1<<22,
	Adp_tmout_int	= 1<<23,
	Adp_prb_int_msk	= 1<<24,
	Adp_sns_int_msk	= 1<<25,
	Adp_tmout_int_msk= 1<<26,
	Ar		= 0x3<<27,

	/* hcfg */
	Fslspclksel	= 0x3<<0,
		HCFG_30_60_MHZ	= 0<<0,
		HCFG_48_MHZ	= 1<<0,
		HCFG_6_MHZ	= 2<<0,
	Fslssupp	= 1<<2,
	Ena32khzs	= 1<<7,
	Resvalid	= 0xff<<8,
	Descdma		= 1<<23,
	Frlisten	= 0x3<<24,
	Modechtimen	= 1<<31,

	/* hfir */
	Frint		= 0xffff<<0,
	Hfirrldctrl	= 1<<16,

	/* hfnum */
	Frnum		= 0xffff<<0,
		MAX_FRNUM 	= 0x3FFF<<0,
	Frrem		= 0xffff<<16,

	/* hptxsts */
	Ptxfspcavail	= 0xffff<<0,
	Ptxqspcavail	= 0xff<<16,
	Ptxqtop_terminate= 1<<24,
	Ptxqtop_token	= 0x3<<25,
	Ptxqtop_chnum	= 0xf<<27,
	Ptxqtop_odd	= 1<<31,

	/* haint, haintmsk */
#define CHANINT(n)	(1<<(n))

	/* hport0 */
	Prtconnsts	= 1<<0,		/* connect status (RO) */
	Prtconndet	= 1<<1,		/* connect detected R/W1C) */
	Prtena		= 1<<2,		/* enable (R/W1C) */
	Prtenchng	= 1<<3,		/* enable/disable change (R/W1C) */
	Prtovrcurract	= 1<<4,		/* overcurrent active (RO) */
	Prtovrcurrchng	= 1<<5,		/* overcurrent change (R/W1C) */
	Prtres		= 1<<6,		/* resume */
	Prtsusp		= 1<<7,		/* suspend */
	Prtrst		= 1<<8,		/* reset */
	Prtlnsts	= 0x3<<10,	/* line state {D+,D-} (RO) */
	Prtpwr		= 1<<12,	/* power on */
	Prttstctl	= 0xf<<13,	/* test */
	Prtspd		= 0x3<<17,	/* speed (RO) */
		HIGHSPEED	= 0<<17,
		FULLSPEED	= 1<<17,
		LOWSPEED	= 2<<17,

	/* hcchar */
	Mps		= 0x7ff<<0,	/* endpoint maximum packet size */
	Epnum		= 0xf<<11,	/* endpoint number */
		OEpnum		= 11,
	Epdir		= 1<<15,	/* endpoint direction */
		Epout		= 0<<15,
		Epin		= 1<<15,
	Lspddev		= 1<<17,	/* device is lowspeed */
	Eptype		= 0x3<<18,	/* endpoint type */
		Epctl		= 0<<18,
		Episo		= 1<<18,
		Epbulk		= 2<<18,
		Epintr		= 3<<18,
	Multicnt	= 0x3<<20,	/* transactions per μframe */
					/* or retries per periodic split */
		OMulticnt	= 20,
	Devaddr		= 0x7f<<22,	/* device address */
		ODevaddr	= 22,
	Oddfrm		= 1<<29,	/* xfer in odd frame (iso/interrupt) */
	Chdis		= 1<<30,	/* channel disable (write 1 only) */
	Chen		= 1<<31,	/* channel enable (write 1 only) */

	/* hcsplt */
	Prtaddr		= 0x7f<<0,	/* port address of recipient */
					/* transaction translator */
	Hubaddr		= 0x7f<<7,	/* dev address of transaction */
					/* translator's hub */
		OHubaddr	= 7,
	Xactpos		= 0x3<<14,	/* payload's position within transaction */
		POS_MID		= 0<<14,		
		POS_END		= 1<<14,
		POS_BEGIN	= 2<<14,
		POS_ALL		= 3<<14, /* all of data (<= 188 bytes) */
	Compsplt	= 1<<16,	/* do complete split */
	Spltena		= 1<<31,	/* channel enabled to do splits */

	/* hcint, hcintmsk */
	Xfercomp	= 1<<0,		/* transfer completed without error */
	Chhltd		= 1<<1,		/* channel halted */
	Ahberr		= 1<<2,		/* AHB dma error */
	Stall		= 1<<3,
	Nak		= 1<<4,
	Ack		= 1<<5,
	Nyet		= 1<<6,
	Xacterr		= 1<<7,	/* transaction error (crc, t/o, bit stuff, eop) */
	Bblerr		= 1<<8,
	Frmovrun	= 1<<9,
	Datatglerr	= 1<<10,
	Bna		= 1<<11,
	Xcs_xact	= 1<<12,
	Frm_list_roll	= 1<<13,

	/* hctsiz */
	Xfersize	= 0x7ffff<<0,	/* expected total bytes */
	Pktcnt		= 0x3ff<<19,	/* expected number of packets */
		OPktcnt		= 19,
	Pid		= 0x3<<29,	/* packet id for initial transaction */
		DATA0		= 0<<29,
		DATA1		= 2<<29,	/* sic */
		DATA2		= 1<<29,	/* sic */
		MDATA		= 3<<29,	/* (non-ctl ep) */
		SETUP		= 3<<29,	/* (ctl ep) */
	Dopng		= 1<<31,	/* do PING protocol */

	/* pcgcctl */
	Stoppclk		= 1<<0,
	Gatehclk		= 1<<1,
	Pwrclmp			= 1<<2,
	Rstpdwnmodule		= 1<<3,
	Enbl_sleep_gating	= 1<<5,
	Phy_in_sleep		= 1<<6,
	Deep_sleep		= 1<<7,
	Resetaftsusp		= 1<<8,
	Restoremode		= 1<<9,
	Enbl_extnd_hiber	= 1<<10,
	Extnd_hiber_pwrclmp	= 1<<11,
	Extnd_hiber_switch	= 1<<12,
	Ess_reg_restored	= 1<<13,
	Prt_clk_sel		= 0x3<<14,
	Port_power		= 1<<16,
	Max_xcvrselect		= 0x3<<17,
	Max_termsel		= 1<<19,
	Mac_dev_addr		= 0x7f<<20,
	P2hd_dev_enum_spd	= 0x3<<27,
	P2hd_prt_spd		= 0x3<<29,
	If_dev_mode		= 1<<31,
};
