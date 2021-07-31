/*
 * Port for WaveLAN I PCMCIA cards running on 2.4 GHz
 * important: only works for WaveLAN I and PCMCIA 2.4 GHz cards
 * Based on Linux driver by Anthony D. Joseph MIT a.o.
 * We have not added the frequency, encryption and NWID selection stuff, this
 * can be done with the WaveLAN provided DOS programs: instconf.exe, setconf.exe, wfreqsel.exe, etc.
 * Gerard Smit 07/22/98
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"
#include "etherwavelan.h"


static int wavelan_receive(Ether *ether);
static int txstart(Ether *ether);

static void 
hacr_write_slow(int base, uchar hacr)
{
	outb(HACR(base), hacr);
	/* delay might only be needed sometimes */
	delay(1);
} /* hacr_write_slow */

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{	
	Ctlr *ctlr;
	int len;
	char *p;

	ctlr = ether->ctlr;
	p = malloc(READSTR);
	len = snprint(p, READSTR, "interrupts: %lud\n", ctlr->interrupts);
	len += snprint(p+len, READSTR-len, "upinterrupts: %lud\n", ctlr->upinterrupts);
	len += snprint(p+len, READSTR-len, "dninterrupts: %lud\n", ctlr->dninterrupts);
	len += snprint(p+len, READSTR-len, "int_errors: %lud\n", ctlr->int_errors);
	len += snprint(p+len, READSTR-len, "read_errors: %lud\n", ctlr->read_errors);
	len += snprint(p+len, READSTR-len, "out_packets: %lud\n", ctlr->out_packets);
	len += snprint(p+len, READSTR-len, "tx_too_long: %lud\n", ctlr->tx_too_long);
	len += snprint(p+len, READSTR-len, "tx_DMA_underrun: %lud\n", ctlr->tx_DMA_underrun);
	len += snprint(p+len, READSTR-len, "tx_carrier_error: %lud\n", ctlr->tx_carrier_error);
	len += snprint(p+len, READSTR-len, "tx_congestion: %lud\n", ctlr->tx_congestion);
	len += snprint(p+len, READSTR-len, "tx_heart_beat: %lud\n", ctlr->tx_heart_beat);
	len += snprint(p+len, READSTR-len, "rx_overflow: %lud\n", ctlr->rx_overflow);
	len += snprint(p+len, READSTR-len, "rx_overrun: %lud\n", ctlr->rx_overrun);
	len += snprint(p+len, READSTR-len, "rx_crc_error: %lud\n", ctlr->rx_crc);
	len += snprint(p+len, READSTR-len, "rx_no_sfd: %lud\n", ctlr->rx_no_sfd);
	len += snprint(p+len, READSTR-len, "rx_dropped: %lud\n", ctlr->rx_dropped);
	len += snprint(p+len, READSTR-len, "tx_packets: %lud\n", ctlr->tx_packets);
	len += snprint(p+len, READSTR-len, "rx_packets: %lud\n", ctlr->rx_packets);
	snprint(p+len, READSTR-len, "in_packets: %lud\n", ctlr->in_packets);

	n = readstr(offset, a, n, p);
	free(p);
	return n;
}

static void
attach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->wlock);
	if(ctlr->attached){
		iunlock(&ctlr->wlock);
		return;
	}
	ctlr->attached = 1;
	iunlock(&ctlr->wlock);
}

static void
interrupt_handler(Ether *ether, uchar status)
{
	Ctlr *ctlr;
	int status0;
	int tx_status;
	int base;
	ctlr = ether->ctlr;
	base = ctlr->port;

	status0 = status;

	/* Return if no actual interrupt from i82593 */
	if(!(status0 & SR0_INTERRUPT)) {
		print("Wavelan: Interrupt from dead card\n");
		return;
	}
	ctlr->status = status0;	/* Save current status (for commands) */
	if (status0 & SR0_RECEPTION) {
		if((status0 & SR0_EVENT_MASK) == SR0_STOP_REG_HIT) {
			print("wavelan: receive buffer overflow\n");
			ctlr->rx_overflow++;
			outb(LCCR(base), CR0_INT_ACK | OP0_NOP);	/* Acknowledge the interrupt */
			return;
		}
		while(wavelan_receive(ether))
			;
		if (status0 & SR0_EXECUTION)
			print("wavelan_cs: interrupt is both rx and tx, status0 = %x\n",
				status0);
		outb(LCCR(base), CR0_INT_ACK | OP0_NOP);	/* Acknowledge the interrupt */
		return;
	}
	if (!(status0 & SR0_EXECUTION)) {
		print("wavelan_cs: interrupt is neither rx or tx, status0 = %x\n",
			status0);
		outb(LCCR(base), CR0_INT_ACK | OP0_NOP);	/* Acknowledge the interrupt */
		return;
	}
	/* interrupt due to configure_done or IA_setup_done */
	if ((status0 & SR0_EVENT_MASK) == SR0_CONFIGURE_DONE ||
			(status0 & SR0_EVENT_MASK) == SR0_IA_SETUP_DONE) {
		outb(LCCR(base), CR0_INT_ACK | OP0_NOP);	/* Acknowledge the interrupt */
		return;
	}
	/* so a transmit interrupt is remaining */
	if((status0 & SR0_EVENT_MASK) == SR0_TRANSMIT_DONE ||
			(status0 & SR0_EVENT_MASK) == SR0_RETRANSMIT_DONE) {
		tx_status = inb(LCSR(base));
		tx_status |= (inb(LCSR(base)) << 8);
		if (!(tx_status & TX_OK)) {
			if (tx_status & TX_FRTL) {
				print("wavelan_cs: frame too long\n");
				ctlr->tx_too_long++;
			}
			if (tx_status & TX_UND_RUN) {
				/* print("wavelan_csd: DMA underrun\n"); */
				ctlr->tx_DMA_underrun++;
			}
			if (tx_status & TX_LOST_CTS) {
				/* print("wavelan: no CTS\n"); */
				ctlr->tx_carrier_error++;
			}
			if (tx_status & TX_LOST_CRS) {
				/* print("wavelan: lost CRS\n"); */
				ctlr->tx_carrier_error++;
			}
			if (tx_status & TX_DEFER) {
				/* print("wavelan: channel jammed\n"); */
				ctlr->tx_congestion++;
			}
			if (tx_status & TX_COLL) {
				if (tx_status & TX_MAX_COL) {
					/* print("wavelan_cs: channel congestion\n"); */
					ctlr->tx_congestion++;
				}
			}
			if (tx_status & TX_HRT_BEAT) {
 				/* print("wavelan_cs: heart beat\n"); */
				ctlr->tx_heart_beat++;
			}
		}
		
		ctlr->tx_packets++;
		ctlr->txbusy = 0;
      		outb(LCCR(base), CR0_INT_ACK | OP0_NOP);	/* Acknowledge the interrupt */
		txstart(ether);					/* start new transfer if any */
		return;
	}
	print("wavelan: unknown interrupt\n");
	outb(LCCR(base), CR0_INT_ACK | OP0_NOP);	/* Acknowledge the interrupt */

}


static Block*
rbpalloc(Block* (*f)(int))
{
	Block *bp;
	ulong addr;

	/*
	 * The receive buffers must be on a 32-byte
	 * boundary for EISA busmastering.
	 */
	if(bp = f(ROUNDUP(sizeof(Etherpkt), 4) + 31)){
		addr = (ulong)bp->base;
		addr = ROUNDUP(addr, 32);
		bp->rp = (uchar*)addr;
	}

	return bp;
}

static int 
wavelan_cmd(Ether *ether, int base, char *str, int cmd, int result)
{
	int status;
	unsigned long spin;

	/* Spin until the chip finishes executing its current command (if any) */
	do {
		outb(LCCR(base), OP0_NOP | CR0_STATUS_3);
		status = inb(LCSR(base));
	} while ((status & SR3_EXEC_STATE_MASK) != SR3_EXEC_IDLE);

	outb(LCCR(base), cmd);			/* Send the command */
	if(result == SR0_NO_RESULT) {		/* Return immediately, if the command
					   	doesn't return a result */
		return(TRUE);
	}

	/* Busy wait while the LAN controller executes the command.
	* Interrupts had better be enabled (or it will be a long wait).
	*  (We could enable just the WaveLAN's IRQ..., we do not bother this is only for setup commands)
	 */
	for(spin = 0; (spin < 10000000); spin++)
		;
	outb(LCCR(base), CR0_STATUS_0 | OP0_NOP);
	status = inb(LCSR(base));
	if(status & SR0_INTERRUPT){
		if (((status & SR0_EVENT_MASK) == SR0_CONFIGURE_DONE) ||
				((status & SR0_EVENT_MASK) == SR0_IA_SETUP_DONE) ||
				((status & SR0_EVENT_MASK) == SR0_EXECUTION_ABORTED) ||
				((status & SR0_EVENT_MASK) == SR0_DIAGNOSE_PASSED))
			outb(LCCR(base), CR0_INT_ACK | OP0_NOP); /* acknowledge interrupt */
		else 
			interrupt_handler(ether, status);
	} else {
		print("wavelan_cmd: %s timeout, status0 = 0x%uX\n", str, status);
  		outb(OP0_ABORT, LCCR(base));
		spin = 0;
		while(spin++ < 250)	/* wait for the command to execute */
		delay(1);
		return 0;
	}
	if((status & SR0_EVENT_MASK) != result){
		print("wavelan_cmd: %s failed, status0 = 0x%uX\n", str, status);
		return 0;  
	}
	return 1;
} /* wavelan_cmd */

static uchar 
mmc_in(int base, uchar o)
{
	while (inb(HASR(base)) & HASR_MMI_BUSY) ;	/* Wait for MMC to go idle */
	outb(MMR(base), o << 1);			/* Set the read address */
	outb(MMD(base), 0);				/* Required dummy write */
	while (inb(HASR(base)) & HASR_MMI_BUSY) ;	/* Wait for MMC to go idle */
	return((uchar) (inb(MMD(base))));	
}

/*
 * a standard insb causes the wavelan card to not be able
 * to simultanisouly receive a packet out of the ether.
 * It appears that the card runs out of bandwidth to it's
 * internal memory
 */
void
slowinsb(int port, void *buf, int len)
{
	int i, j, n;
	uchar *p, *q;

	p = (uchar*)(port + ISAMEM);
	q = buf;
	n = m->delayloop/2000;
	if(n == 0)
		n = 1;
	for(i = 0; i < len; i++){
		*q++ = *p;
		// a small delay of about .5micro seconds
		for(j=0; j<n; j++)
			;
	}
}

static int 
read_ringbuf(Ether *ether, int addr, uchar *buf, int len)
{	Ctlr *ctlr;
	int base;
	int ring_ptr = addr;
	int chunk_len;
	uchar *buf_ptr = buf;
	ctlr = ether->ctlr;
	base = ctlr->port;

	/* If buf is NULL, just increment the ring buffer pointer */
	if (buf == 0)
	return((ring_ptr - RX_BASE + len) % RX_SIZE + RX_BASE);
	while (len > 0) {
		/* Position the Program I/O Register at the ring buffer pointer */
		outb(PIORL(base), ring_ptr & 0xff);
		outb(PIORH(base), ((ring_ptr >> 8) & PIORH_MASK));
		/* First, determine how much we can read without wrapping around the
 		ring buffer */
		if ((ring_ptr + len) < (RX_BASE + RX_SIZE))
			chunk_len = len;
		else
			chunk_len = RX_BASE + RX_SIZE - ring_ptr;
		slowinsb(PIOP(base), buf_ptr, chunk_len);
		buf_ptr += chunk_len;
		len -= chunk_len;
		ring_ptr = (ring_ptr - RX_BASE + chunk_len) % RX_SIZE + RX_BASE;
	}
	return(ring_ptr);
} /* read_ringbuf */

static void 
wavelan_hardware_send_packet(Ether *ether, void *buf, short length)
{
	Ctlr *ctlr;
	int base;
	register ushort xmtdata_base = TX_BASE;
	ctlr = ether->ctlr;
	base = ctlr->port;

	outb(PIORL(base), xmtdata_base & 0xff);
	outb(PIORH(base), ((xmtdata_base >> 8) & PIORH_MASK) | PIORH_SEL_TX);
	outb(PIOP(base), length & 0xff);		/* lsb */
	outb(PIOP(base), length >> 8);  		/* msb */
	outsb(PIOP(base), buf, length);		/* Send the data */
	outb(PIOP(base), OP0_NOP);		/* Indicate end of transmit chain */

 	/* Reset the transmit DMA pointer */
	hacr_write_slow(base, HACR_PWR_STAT | HACR_TX_DMA_RESET);
	outb(HACR(base), HACR_DEFAULT);
	/* Send the transmit command */
	wavelan_cmd(ether, base, "wavelan_hardware_send_packet(): transmit", OP0_TRANSMIT,
	      SR0_NO_RESULT);
} /* wavelan_hardware_send_packet */



static int 
txstart(Ether *ether)
{	
	Ctlr *ctlr;
	Block *bp;
	int base, length;
	int status;
	ctlr = ether->ctlr;
	base = ctlr->port;

	for(;;) { 
		if(ctlr->txbp){
			bp = ctlr->txbp;
			ctlr->txbp = 0;
		}
		else{
			bp = qget(ether->oq);
			if(bp == nil)
				break;
		}

		length = BLEN(bp);
		length = (ETH_ZLEN < length) ? length : ETH_ZLEN;
		outb(LCCR(base), OP0_NOP | CR0_STATUS_3);
		status = inb(LCSR(base));
		if ((status & SR3_EXEC_STATE_MASK) == SR3_EXEC_IDLE) {
			wavelan_hardware_send_packet(ether, bp->rp, length);
			freeb(bp);
			ctlr->out_packets++;
		}
		else{
			ctlr->txbp = bp;
			if(ctlr->txbusy == 0){
				ctlr->txbusy = 1;
			}
			break;
		}

	}
	return 0;
} /* wavelan txstart */


static int 
wavelan_start_of_frame(Ether *ether, int rfp, int wrap)
{
	Ctlr *ctlr;
	int base;
	int rp, len;

	ctlr = ether->ctlr;
	base = ctlr->port;

	rp = (rfp - 5 + RX_SIZE) % RX_SIZE;
	outb(PIORL(base), rp & 0xff);
	outb(PIORH(base), ((rp >> 8) & PIORH_MASK));
	len = inb(PIOP(base));
	len |= inb(PIOP(base)) << 8;

	if (len > 1600) {		/* Sanity check on size */
		print("wavelan_cs: Received frame too large, rfp %d rp %d len 0x%x\n",
			rfp, rp, len);
		return -1;
	}
  
	if(len < 7){
		print("wavelan_start_of_frame: Received null frame, rfp %d len 0x%x\n", rfp, len);
		return(-1);
	}
	/* Wrap around buffer */
	if(len > ((wrap - (rfp - len) + RX_SIZE) % RX_SIZE)) {	/* magic formula ! */
		print("wavelan_start_of_frame: wrap around buffer, wrap %d rfp %d len 0x%x\n",wrap, rfp, len);
		return(-1);
	}

	return((rp - len + RX_SIZE) % RX_SIZE);
} /* wv_start_of_frame */

static void wavelan_read(Ether *ether, int fd_p, int sksize)
{
	Ctlr *ctlr;
	Block *bp;
	uchar stats[3];

	ctlr = ether->ctlr;

	ctlr->rx_packets++;

	if ((bp = rbpalloc(iallocb)) == 0){		
 		print("wavelan: could not rbpalloc(%d).\n", sksize);
		ctlr->rx_dropped++;
		return;
	} else {
		fd_p = read_ringbuf(ether, fd_p, ctlr->rbp->rp, sksize);
		ctlr->rbp->wp = ctlr->rbp->rp + sksize;
		/* read signal level, silence level and signal quality bytes */
		read_ringbuf(ether, (fd_p+4) % RX_SIZE+RX_BASE, stats, 3);
		/*
		* Hand the packet to the Network Module
		*/
		etheriq(ether, ctlr->rbp, 1);
		ctlr->in_packets++;
		ctlr->rbp = bp;
		return;
	}
} /* wavelan_read */

static int 
wavelan_receive(Ether *ether)
{
	Ctlr *ctlr;
	int base;
	int newrfp, rp, len, f_start, status;
	int i593_rfp, stat_ptr;
	uchar c[4];

	ctlr = ether->ctlr;
	base = ctlr->port;

	/* Get the new receive frame pointer from the i82593 chip */
	outb(LCCR(base), CR0_STATUS_2 | OP0_NOP);
	i593_rfp = inb(LCSR(base));
	i593_rfp |= inb(LCSR(base)) << 8;
	i593_rfp %= RX_SIZE;

	/* Get the new receive frame pointer from the WaveLAN card.
	* It is 3 bytes more than the increment of the i82593 receive
	* frame pointer, for each packet. This is because it includes the
	* 3 roaming bytes added by the mmc.
	*/
	newrfp = inb(RPLL(base));
	newrfp |= inb(RPLH(base)) << 8;
	newrfp %= RX_SIZE;

if(0) print("before wavelan_cs: i593_rfp %d stop %d newrfp %d ctlr->rfp %d\n", i593_rfp, ctlr->stop, newrfp, ctlr->rfp);

	if (newrfp == ctlr->rfp) {
		if(0) print("wavelan_cs: odd RFPs:  i593_rfp %d stop %d newrfp %d ctlr->rfp %d\n",
			i593_rfp, ctlr->stop, newrfp, ctlr->rfp);
		return 0;
	}

	while(newrfp != ctlr->rfp) {
		rp = newrfp;
		/* Find the first frame by skipping backwards over the frames */
		while (((f_start = wavelan_start_of_frame(ether,rp, newrfp)) != ctlr->rfp) && (f_start != -1))
			rp = f_start;
		if(f_start == -1){
			print("wavelan_cs: cannot find start of frame ");
			print(" i593_rfp %d stop %d newrfp %d ctlr->rfp %d\n",
				i593_rfp, ctlr->stop, newrfp, ctlr->rfp);
			ctlr->rfp = rp;
			continue;
		}
		stat_ptr = (rp - 7 + RX_SIZE) % RX_SIZE;
		read_ringbuf(ether, stat_ptr, c, 4);
		status = c[0] | (c[1] << 8);
		len = c[2] | (c[3] << 8);

if(0) print("%ld: len = %d status = %ux %ux %ux\n", m->ticks, len, status, newrfp, rp);

		if(!(status & RX_RCV_OK)) {
			if(status & RX_NO_SFD) ctlr->rx_no_sfd++;
			if(status & RX_CRC_ERR) ctlr->rx_crc++;
			if(status & RX_OVRRUN) ctlr->rx_overrun++;

  			print("wavelan_cs: packet not received ok, status = 0x%x\n", status);
		} else	wavelan_read(ether, f_start, len - 2);

		ctlr->rfp = rp;		/* one packet processed, skip it */
	}

	/*
	* Update the frame stop register, but set it to less than
	* the full 8K to allow space for 3 bytes of signal strength
	* per packet.
	*/
	ctlr->stop = (i593_rfp + RX_SIZE - ((RX_SIZE / 64) * 3)) % RX_SIZE;
	outb(LCCR(base), OP0_SWIT_TO_PORT_1 | CR0_CHNL);
	outb(LCCR(base), CR1_STOP_REG_UPDATE | (ctlr->stop >> RX_SIZE_SHIFT));
	outb(LCCR(base), OP1_SWIT_TO_PORT_0);

if(0){ 
	newrfp = inb(RPLL(base));
	newrfp |= inb(RPLH(base)) << 8;
	newrfp %= RX_SIZE;

	if(newrfp != ctlr->rfp)
		print("after wavelan_cs: left over = %d\n", (newrfp - ctlr->rfp + RX_SIZE) % RX_SIZE);
}

	return 1;
} /* wavelan_receive */

static void
interrupt(Ureg*, void* arg)
{
	Ether *ether;
	Ctlr *ctlr;
	int base;

	ether = arg;
	ctlr = ether->ctlr;
	base = ctlr->port;

	ilock(&ctlr->wlock);

	ctlr->interrupts++;

	outb(LCCR(base), CR0_STATUS_0 | OP0_NOP);
	interrupt_handler(ether, inb(LCSR(base)));
	iunlock(&ctlr->wlock);

}; /* wavelan interrupt */



static void
promiscuous()
{
	;
};

static void
multicast()
{	
	;
};

static void 
mmc_read(int base, uchar o, uchar *b, int n)
{
	while (n-- > 0) {
		while (inb(HASR(base)) & HASR_MMI_BUSY) ;	/* Wait for MMC to go idle */
		outb(MMR(base), o << 1);			/* Set the read address */
		o++;

		outb(MMD(base), 0);				/* Required dummy write */
		while (inb(HASR(base)) & HASR_MMI_BUSY) ;	/* Wait for MMC to go idle */
		*b++ = (uchar)(inb(MMD(base)));			/* Now do the actual read */
	}
} /* mmc_read */


static void
fee_wait(int base, int del, int numb)
{	int count = 0;
	while ((count++ < numb) && (mmc_in(base, MMC_EECTRL) & MMR_FEE_STATUS_BUSY))
		delay(del);
	if (count==numb) print("Wavelan: fee wait timed out\n");
}

static void 
mmc_write_b(int base, uchar o, uchar b)
{
	while (inb(HASR(base)) & HASR_MMI_BUSY) ;	/* Wait for MMC to go idle */
	outb(MMR(base), (uchar)((o << 1) | MMR_MMI_WR));
	outb(MMD(base), (uchar)(b));
} /* mmc_write_b */

static void 
mmc_write(int base, uchar o, uchar *b, int n)
{
	o += n;
	b += n;
	while (n-- > 0 ) 
		mmc_write_b(base, --o, *(--b));
} /* mmc_write */


static void wavelan_mmc_init(Ether *ether, int port)
{
	Ctlr *ctlr;
	mmw_t	m;
	ctlr = ether->ctlr;

	memset(&m, 0x00, sizeof(m));

	/*
	* Set default modem control parameters.
	* See NCR document 407-0024326 Rev. A.
	*/
	m.mmw_jabber_enable = 0x01;
	m.mmw_anten_sel = MMW_ANTEN_SEL_ALG_EN;
	m.mmw_ifs = 0x20;
	m.mmw_mod_delay = 0x04;
	m.mmw_jam_time = 0x38;
	m.mmw_encr_enable = 0;
	m.mmw_des_io_invert = 0;
	m.mmw_freeze = 0;
	m.mmw_decay_prm = 0;
	m.mmw_decay_updat_prm = 0;
	m.mmw_loopt_sel = MMW_LOOPT_SEL_UNDEFINED;
	m.mmw_thr_pre_set = 0x04;   /* PCMCIA */
	m.mmw_quality_thr = 0x03;
	m.mmw_netw_id_l = ctlr->nwid[1];		/* use nwid of PSA memory */
	m.mmw_netw_id_h = ctlr->nwid[0];
  
	mmc_write(port, 0, (uchar *)&m, 37); 		/* size of mmw_t == 37 */

	/* Start the modem's receive unit on version 2.00 		     */
						/* 2.4 Gz: half-card ver     */
						/* 2.4 Gz		     */
						/* 2.4 Gz: position ch #     */
	mmc_write_b(port, MMC_EEADDR, 0x0f);	/* 2.4 Gz: named ch, wc=16   */
	mmc_write_b(port, MMC_EECTRL,MMC_EECTRL_DWLD |	/* 2.4 Gz: Download Synths   */
			MMC_EECTRL_EEOP_READ);	/* 2.4 Gz: Read EEPROM	     */
	fee_wait(port, 10, 100);		/* 2.4 Gz: wait for download */
						/* 2.4 Gz	      */
	mmc_write_b(port, MMC_EEADDR,0x61);		/* 2.4 Gz: default pwr, wc=2 */
	mmc_write_b(port, MMC_EECTRL,MMC_EECTRL_DWLD |	/* 2.4 Gz: Download Xmit Pwr */
			MMC_EECTRL_EEOP_READ);	/* 2.4 Gz: Read EEPROM	     */
	fee_wait(port, 10, 100);		/* 2.4 Gz: wait for download */

} /* wavelan_mmc_init */


int wavelan_diag(Ether *ether, int port)
{
  	if (wavelan_cmd(ether, port, "wavelan_diag(): diagnose", OP0_DIAGNOSE,
		  SR0_DIAGNOSE_PASSED)){
		return 0;
	}
	print("wavelan_cs: i82593 Self Test failed!\n");
	return 1;
} /* wavelan_diag */



int wavelan_hw_config(int base, Ether *ether)
{
	
	/* the i82593_conf_block uses bit fields which are not
	 * machine independent.  In particular this code does not work
	 * on the powerPC (bigendian)
	 */
	if(0) {
		struct i82593_conf_block cfblk;

		memset(&cfblk, 0x00, sizeof(struct i82593_conf_block));
		cfblk.d6mod = FALSE;	/* Run in i82593 advanced mode */
		cfblk.fifo_limit = 6;	/* = 48 bytes rx and tx fifo thresholds */
		cfblk.forgnesi = FALSE;	/* 0=82C501, 1=AMD7992B compatibility */
		cfblk.fifo_32 = 0;
		cfblk.throttle_enb = TRUE;
		cfblk.contin = TRUE;	/* enable continuous mode */
		cfblk.cntrxint = FALSE;	/* enable continuous mode receive interrupts */
		cfblk.addr_len = WAVELAN_ADDR_SIZE;
		cfblk.acloc = TRUE;	/* Disable source addr insertion by i82593 */
		cfblk.preamb_len = 2;	/* 7 byte preamble */
		cfblk.loopback = FALSE;
		cfblk.lin_prio = 0;	/* conform to 802.3 backoff algoritm */
		cfblk.exp_prio = 0;	/* conform to 802.3 backoff algoritm */
		cfblk.bof_met = 0;	/* conform to 802.3 backoff algoritm */
		cfblk.ifrm_spc = 6;	/* 96 bit times interframe spacing */
		cfblk.slottim_low = 0x10 & 0x7;	/* 512 bit times slot time */
		cfblk.slottim_hi = 0x10 >> 3;
		cfblk.max_retr = 15;	
		cfblk.prmisc = FALSE;	/* Promiscuous mode ?? */
		cfblk.bc_dis = FALSE;	/* Enable broadcast reception */
		cfblk.crs_1 = TRUE;	/* Transmit without carrier sense */
		cfblk.nocrc_ins = FALSE; /* i82593 generates CRC */	
		cfblk.crc_1632 = FALSE;	/* 32-bit Autodin-II CRC */
		cfblk.crs_cdt = FALSE;	/* CD not to be interpreted as CS */
		cfblk.cs_filter = 0;  	/* CS is recognized immediately */
		cfblk.crs_src = FALSE;	/* External carrier sense */
		cfblk.cd_filter = 0;  	/* CD is recognized immediately */
		cfblk.min_fr_len = 64 >> 2;	/* Minimum frame length 64 bytes */
		cfblk.lng_typ = FALSE;	/* Length field > 1500 = type field */
		cfblk.lng_fld = TRUE; 	/* Disable 802.3 length field check */
		cfblk.rxcrc_xf = TRUE;	/* Don't transfer CRC to memory */
		cfblk.artx = TRUE;	/* Disable automatic retransmission */
		cfblk.sarec = TRUE;	/* Disable source addr trig of CD */
		cfblk.tx_jabber = TRUE;	/* Disable jabber jam sequence */
		cfblk.hash_1 = FALSE; 	/* Use bits 0-5 in mc address hash */
		cfblk.lbpkpol = TRUE; 	/* Loopback pin active high */
		cfblk.fdx = FALSE;	/* Disable full duplex operation */
		cfblk.dummy_6 = 0x3f; 	/* all ones */
		cfblk.mult_ia = FALSE;	/* No multiple individual addresses */
		cfblk.dis_bof = FALSE;	/* Disable the backoff algorithm ?! */
		cfblk.dummy_1 = TRUE; 	/* set to 1 */
		cfblk.tx_ifs_retrig = 3; /* Hmm... Disabled */
		cfblk.mc_all = FALSE;	/* No multicast all mode */	
		cfblk.rcv_mon = 0;	/* Monitor mode disabled */
		cfblk.frag_acpt = TRUE;/* Do not accept fragments */
		cfblk.tstrttrs = FALSE;	/* No start transmission threshold */
		cfblk.fretx = TRUE;	/* FIFO automatic retransmission */
		cfblk.syncrqs = TRUE; 	/* Synchronous DRQ deassertion... */
		cfblk.sttlen = TRUE;  	/* 6 byte status registers */
		cfblk.rx_eop = TRUE;  	/* Signal EOP on packet reception */
		cfblk.tx_eop = TRUE;  	/* Signal EOP on packet transmission */
		cfblk.rbuf_size = RX_SIZE>>11;	/* Set receive buffer size */
		cfblk.rcvstop = TRUE; 	/* Enable Receive Stop Register */
		outb(PIORL(base), (TX_BASE & 0xff));
		outb(PIORH(base), (((TX_BASE >> 8) & PIORH_MASK) | PIORH_SEL_TX));
		outb(PIOP(base), (sizeof(struct i82593_conf_block) & 0xff));    /* lsb */
		outb(PIOP(base), (sizeof(struct i82593_conf_block) >> 8));	/* msb */
		outsb(PIOP(base), ((char *) &cfblk), sizeof(struct i82593_conf_block));
	} else {
		/*
 		 * The following are the values that result in the above on
		 * the 386.  Sleazy, but these values don't change and I
		 * don't have any docs anyway
		 */
		uchar buf[16] = { 0x86, 0x80, 0x2e, 0x00, 0x60, 0x00, 0xf2, 0x08,
						  0x00, 0x40, 0xbe, 0x00, 0x3f, 0x47, 0xf1, 0x24};

		outb(PIORL(base), (TX_BASE & 0xff));
		outb(PIORH(base), (((TX_BASE >> 8) & PIORH_MASK) | PIORH_SEL_TX));
		outb(PIOP(base), (sizeof(buf) & 0xff));    /* lsb */
		outb(PIOP(base), (sizeof(buf) >> 8));	/* msb */
		outsb(PIOP(base), buf, sizeof(buf));
	}

	/* reset transmit DMA pointer */
	hacr_write_slow(base, HACR_PWR_STAT | HACR_TX_DMA_RESET);
	outb(HACR(base), HACR_DEFAULT);
	if(!wavelan_cmd(ether, base, "wavelan_hw_config(): configure", OP0_CONFIGURE,
		  SR0_CONFIGURE_DONE))
		return(FALSE);

	/* Initialize adapter's ethernet MAC address */
	outb(PIORL(base), (TX_BASE & 0xff));
	outb(PIORH(base), (((TX_BASE >> 8) & PIORH_MASK) | PIORH_SEL_TX));
	outb(PIOP(base), WAVELAN_ADDR_SIZE);	/* byte count lsb */
	outb(PIOP(base), 0);			/* byte count msb */
	outsb(PIOP(base), &ether->ea[0], WAVELAN_ADDR_SIZE);
	/* reset transmit DMA pointer */
	hacr_write_slow(base, HACR_PWR_STAT | HACR_TX_DMA_RESET);
	outb(HACR(base), HACR_DEFAULT);
	if(!wavelan_cmd(ether, base, "wavelan_hw_config(): ia-setup", OP0_IA_SETUP, SR0_IA_SETUP_DONE))
		return(FALSE);
	return(TRUE);
} /* wavelan_hw_config */

static void 
wavelan_graceful_shutdown(Ether *ether, int base)
{
  int status;
  
  /* First, send the LAN controller a stop receive command */
  wavelan_cmd(ether, base, "wavelan_graceful_shutdown(): stop-rcv", OP0_STOP_RCV,
	      SR0_NO_RESULT);
  /* Then, spin until the receive unit goes idle */
  do {
    outb(LCCR(base), (OP0_NOP | CR0_STATUS_3));
    status = inb(LCSR(base));
  } while((status & SR3_RCV_STATE_MASK) != SR3_RCV_IDLE);
		       
  /* Now, spin until the chip finishes executing its current command */
  do {
    outb(LCCR(base), (OP0_NOP | CR0_STATUS_3));
    status = inb(LCSR(base));
  } while ((status & SR3_EXEC_STATE_MASK) != SR3_EXEC_IDLE);
} /* wavelan_graceful_shutdown */


static void 
wavelan_ru_start(Ether *ether, int base)
{
	Ctlr *ctlr;
	ctlr = ether->ctlr;

	/*
	* We need to start from a quiescent state. To do so, we could check
	* if the card is already running, but instead we just try to shut
	* it down. First, we disable reception (in case it was already enabled).
	*/

	wavelan_graceful_shutdown(ether, base);

	/* Now we know that no command is being executed. */

	/* Set the receive frame pointer and stop pointer */
	ctlr->rfp = 0;
	outb(LCCR(base), OP0_SWIT_TO_PORT_1 | CR0_CHNL);

	/* Reset ring management.  This sets the receive frame pointer to 1 */
	outb(LCCR(base), OP1_RESET_RING_MNGMT);
	ctlr->stop = (0 + RX_SIZE - ((RX_SIZE / 64) * 3)) % RX_SIZE;
	outb(LCCR(base), CR1_STOP_REG_UPDATE | (ctlr->stop >> RX_SIZE_SHIFT));
	outb(LCCR(base), OP1_INT_ENABLE);
	outb(LCCR(base), OP1_SWIT_TO_PORT_0);

	/* Reset receive DMA pointer */
	outb(HACR(base), HACR_PWR_STAT | HACR_RX_DMA_RESET);
	delay(100);
	outb(HACR(base), HACR_PWR_STAT);
	delay(100);

	/* Receive DMA on channel 1 */
	wavelan_cmd(ether, base, "wavelan_ru_start(): rcv-enable",
	      (CR0_CHNL | OP0_RCV_ENABLE), SR0_NO_RESULT);

} /* wavelan_ru_start */

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;
	ctlr = ether->ctlr;

	ilock(&ctlr->wlock);
	txstart(ether);	
	iunlock(&ctlr->wlock);
}


static int
reset(Ether* ether)
{
	int slot, p;
	int port;
	char *pp;
	Ctlr *ctlr;
	PCMmap *m;

	ctlr = ether->ctlr = malloc(sizeof(Ctlr)); 
	ilock(&ctlr->wlock);

	if(ether->port == 0)
		ether->port = 0x280;
	port = ether->port;

	if((slot = pcmspecial(ether->type, ether)) < 0) {
		print("could not find the PCMCIA WaveLAN card.\n"); 
		return -1;
	}


	print("#l%dWaveLAN: slot %d, port 0x%ulX irq %ld type %s\n", ether->ctlrno, slot, ether->port, ether->irq, ether->type);

	/* create a receive buffer */
	ctlr->rbp = rbpalloc(allocb);

/* map a piece of memory (Attribute memory) first */
	m = pcmmap(slot, 0, 0x5000, 1);
	if (m==0) {
		return 1;
	}
/* read ethernet address from the card and put in ether->ea */
	pp = (char*)(KZERO|m->isa) + 0x0E00 + 2*0x10;
	for(p = 0; p<sizeof(ether->ea); p++)
		ether->ea[p] = (uchar) (*(pp+2*p))&0xFF;

//	print("wavelan: dump of PSA memory\n");
//	pp = (char *) (KZERO|m->isa) + 0x0E00;
//	for(p=0; p<64; p++) {
//		print("%2uX ", (*(pp+2*p)&0xFF));
//		if (p%16==15) print("\n");
//	}

/* read nwid from PSA into ctlr->nwid */
	pp = (char*)(KZERO|m->isa) + 0x0E00;
	ctlr->nwid[0] = *(pp+2*0x23); 
	ctlr->nwid[1] = *(pp+2*0x24); 
	
/* access the configuration option register 	*/
	pp = (char *)(KZERO|m->isa) + 0x4000;	
	*pp = *pp | COR_SW_RESET; 		
	delay(5); 				
	*pp = (COR_LEVEL_IRQ | COR_CONFIG); 	
	delay(5); 				

	hacr_write_slow(port, HACR_RESET);
	outb(HACR(port), HACR_DEFAULT);

	if(inb(HASR(port)) & HASR_NO_CLK) {
		print("wavelan: modem not connected\n");
		return 1;
	}

	wavelan_mmc_init(ether, port);		/* initialize modem */

	outb(LCCR(port), OP0_RESET);	/* reset the LAN controller */
	delay(10);

	if (wavelan_hw_config(port, ether) == FALSE)
		return 1;

	if (wavelan_diag(ether, port) == 1) 
		return 1;
	wavelan_ru_start(ether, port);

	print("wavelan: init done; receiver started\n");

	iunlock(&ctlr->wlock);	

	ctlr->port = port;
	ether->port = port;
	ether->mbps = 2;		/* 2 Mpbs */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;

	ether->promiscuous = promiscuous;
	ether->multicast = multicast;
	ether->arg = ether;

	return 0;			/* reset succeeded */
}

void
etherwavelanlink(void)
{
	addethercard("WaveLAN", reset);
}
