#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

/*
 * 
 * mcc.c
 *
 * This is the driver for the Multi-Channel Communications Controller
 * of the MPC8260.  This version is constructed for the EST SBC8260 to
 * handle data from an interface to an offboard T1 framer.  The driver
 * supports MCC2 + TDM A:2 (channel 128) which is connected to the
 * external interface.
 *
 * Neville Chandler
 * Lucent Technologies - Bell Labs
 * March 2001
 *
 */

#define	PKT_LEN			40
#define MPC82XX_INIT_DELAY      0x10000   

#define HPIC		0xFC000000
#define HPIA		0xFC000010
#define HPID_A		0xFC000020
#define HPID		0xFC000030


#include "mcc2.h"

static ssize_t mcc2_read( struct file *, char *, size_t, loff_t * );
static ssize_t mcc2_write( struct file *, const char *, size_t, loff_t *);
static loff_t mcc2_lseek( struct file *, loff_t, int );
static int mcc2_release( struct inode *, struct file * );
static ssize_t mcc2_ioctl( struct inode *, struct file *, unsigned int, unsigned long );
//static ssize_t mcc2_ioctl( struct inode *, struct file *, unsigned int, char * );

void MPC82xxCpmInit( void );
void PortInit( void );
void PortSelectPin( unsigned short );

void InitMemAlloc( void );
void HeapCreate( U32, U32, U32, U32, char *);
void HeapCreate( U32, U32, U32, U32, char *);
void *HeapSearchMem( U32, U32);
void *HeapAllocMem( U32, U32);
void HeapFreeMem( U32, void *);

void InitLinkedList( void );
boolean DwCreateList( ListDB * );
void *DwMalloc( U32 );
void DwFree( U32, void * );

void ppc_irq_dispatch_handler(struct pt_regs *regs, int irq);

#define NR_MASK_WORDS   ((NR_IRQS + 31) / 32)

extern int ppc_spurious_interrupts;
extern int ppc_second_irq;
extern struct irqaction *ppc_irq_action[NR_IRQS];
extern unsigned int ppc_local_bh_count[NR_CPUS];
extern unsigned int ppc_local_irq_count[NR_CPUS];
extern unsigned int ppc_cached_irq_mask[NR_MASK_WORDS];
extern unsigned int ppc_lost_interrupts[NR_MASK_WORDS];
extern atomic_t ppc_n_lost_interrupts;

//static void disp_led( unsigned char );

void	Mcc2Init( void );
void	MccDisable( unsigned char );
void	MccEnable( unsigned char, unsigned char, unsigned char );
void	MccRiscCmd( unsigned char, dwv_RISC_OPCODE, unsigned char );
boolean	MccTest( void );
int	MccTxBuffer( unsigned char, unsigned char, char *, unsigned short, unsigned short );
extern	U32 PpcDisable( void );
extern	void PpcMsrRestore( U32 );

static	int	mcc2_major = MCC_MAJOR;

static	BOOLEAN insertBD_T( BD_PFIFO *, BD_P );
static	BOOLEAN removBD_T( BD_PFIFO *, BD_P * );
BOOLEAN	empty(volatile register FIFO *);
int	insert( FIFO *, char * );
int	remove( FIFO *, char ** );
void	AppInit( void );

#define physaddr(ADDR)	(0x60020000 | ((ADDR) << 23) | (2 << 18))
  
mcc_iorw_t mcc_iorw;

#if 0
typedef struct mcc_io {
	unsigned int	cmd;
        unsigned int    address;
        unsigned int    *buf;
	int		ind;
        int             nbytes;
	siramctl_t	SiRam;
	cpmux_t		CpMux;
	mcc_t		Mcc_T;
	iop8260_t	Io_Ports;
} mcc_iorw_t;
#endif

static void
ioctl_parm( unsigned int loop_mode )
{

	/* Setup the SIMODE Register */
  	Si2Regs->SiAmr = SIxMR_SAD_BANK0_FIRST_HALF    |  /* SADx  */
			 loop_mode                     |  /* SDMx  */
			 SIxMR_NO_BIT_RX_SYNC_DELAY    |  /* RFSDx */
			 SIxMR_DSC_CH_DATA_CLK_EQU     |  /* DSCx  */
			 SIxMR_CRT_SPEPARATE_PINS      |  /* CRTx  */
			 SIxMR_SLx_NORMAL_OPERATION    |  /* SLx   */
			 SIxMR_CE_TX_RISING_RX_FALLING |  /* CEx   */
			 SIxMR_FE_FALLING_EDGE         |  /* FEx   */
			 SIxMR_GM_GCI_SCIT_MODE        |  /* GMx   */
			 SIxMR_NO_BIT_TX_SYNC_DELAY;      /* TFSDx */
}

#if 0
static void
disp_led( unsigned char byte )
{
     //int i;

     *leds = byte;
     //for(i=0; i<1000; i++);

}
#endif



static ssize_t
mcc2_ioctl( struct inode *inode, struct file *file,
			      unsigned int ioctl_cmd,		// IOCTL number
			      unsigned long param )
//			      char *param )			// IOCTL parameter
{
	static unsigned char mode;
	char cp, *cptr;
	void *vptr;
	unsigned long *lptr;
	int i, j;
	unsigned int ld;
	unsigned long lng;
	volatile immap_t *Mmap;

	cptr = (char *)param;
	mode = (unsigned char)*cptr;
	Mmap = ((volatile immap_t *)IMAP_ADDR);
	switch(ioctl_cmd)
	{
		case IOCTL_SET_MODE:
			    //mode = (unsigned char)*param;
			    mode = ((mcc_iorw_t *)param)->cmd;
			    switch( mode )
			    {
				case NORMAL_OPERATION:
					/* Setup the SIMODE Register */
					D( printk("mcc2_ioctl: ioctl set NORMAL_OPERATION mode\n"); )
					ioctl_parm( (unsigned int)SIxMR_SDM_NORMAL_OPERATION );		/* SDMx  */
					break;

				case AUTOMATIC_ECHO:
					/* Setup the SIMODE Register */
					D( printk("mcc2_ioctl: ioctl set AUTOMATIC_ECHO mode\n"); )
					ioctl_parm( (unsigned int)SIxMR_SDM_AUTOMATIC_ECHO );		/* SDMx  */
					break;

				case INTERNAL_LOOPBACK:
					/* Setup the SIMODE Register */
					D( printk("mcc2_ioctl: ioctl set INTERNAL_LOOPBACK mode\n"); )
					ioctl_parm( (unsigned int)SIxMR_SDM_INTERNAL_LOOPBACK );	/* SDMx  */
					break;

				case LOOPBACK_CONTROL:
					/* Setup the SIMODE Register */
					D( printk("mcc2_ioctl: ioctl set LOOPBACK_CONTROL mode\n"); )
					ioctl_parm( (unsigned int)SIxMR_SDM_LOOPBACK_CONTROL );		/* SDMx  */
					break;


				default:
					printk("mcc2_ioctl: Error, unrecognized ioctl parameter, device operation unchanged.\n");
					break;

			    }
			break;

		case IOCTL_RWX_MODE:
			mode = ((mcc_iorw_t *)param)->cmd;
			switch(mode)
			{
				case HPI_RD:
					lng = (long)(((mcc_iorw_t *)param)->address);
					lptr =  ((unsigned long *)lng);
					vptr = (void *)lptr;
					if (copy_to_user( (((mcc_iorw_t *)param)->buf), (void *)vptr, (((mcc_iorw_t *)param)->nbytes))) {
						printk("mcc2_ioctl: Failed during  read from hpi.\n");
						return -EFAULT;
					}
					break; 
					
								    	  
				case HPI_WR:
					lng = (long)(((mcc_iorw_t *)param)->address);
					lptr =  ((unsigned long *)lng);
					vptr = (void *)lptr;
					if (copy_from_user( (void *)vptr, (((mcc_iorw_t *)param)->buf), (((mcc_iorw_t *)param)->nbytes))) {
						printk("mcc2_ioctl: Failed during  write to hpi\n");
						return -EFAULT;
					}
					break; 
					


                                case FPGA_RD:
                                        lng = (long)(((mcc_iorw_t *)param)->address);
                                        lptr =  ((unsigned long *)lng);
                                        vptr = (void *)lptr;
                                        if (copy_to_user( (((mcc_iorw_t *)param)->buf), (void *)vptr, (((mcc_iorw_t *)param)->nbytes))) {
                                                printk("mcc2_ioctl: Failed during  read from FPGA.\n");
                                                return -EFAULT;
                                        }
                                        break;


                                case FPGA_WR:
                                        lng = (long)(((mcc_iorw_t *)param)->address);
                                        lptr =  ((unsigned long *)lng);
                                        vptr = (void *)lptr;
                                        if (copy_from_user( (void *)vptr, (((mcc_iorw_t *)param)->buf), (((mcc_iorw_t *)param)->nbytes))) {
                                                printk("mcc2_ioctl: Failed during  write to FPGA\n");
                                                return -EFAULT;
                                        }
                                        break;




			   
				case MEM_MODR:
					cptr = (char *)Mmap;
					cptr += ((mcc_iorw_t *)param)->address;
					if (copy_to_user( (((mcc_iorw_t *)param)->buf), (void *)cptr, (((mcc_iorw_t *)param)->nbytes))) {
						printk("mcc2_ioctl: Failed during read of read-modify memory\n");
						return -EFAULT;
					}
					break;

				case MEM_MODW:
					cptr = (char *)Mmap;
					cptr += ((mcc_iorw_t *)param)->address;
					if (copy_from_user( (void *)cptr, (((mcc_iorw_t *)param)->buf), (((mcc_iorw_t *)param)->nbytes))) {
						printk("mcc2_ioctl: Failed during modify of read-modify memory\n");
						return -EFAULT;
					}
					break;

				case IO_PORTS:
					break;
				case SI_RAM_CTL1:
					break;
				case SI_RAM_CTL2:
					if (copy_to_user( (void *)param, (siramctl_t *)&(Mmap->im_siramctl2), sizeof(siramctl_t))) {
						printk("mcc2_ioctl: Failed to copy SI_RAM_CTL2 struct\n");
						return -EFAULT;
					}
					break;



				default:
					break;
			}
			break;

				default:
					//if (copy_to_user((void *)param, &mode, sizeof(mode)))
					printk("We are at the end ...\n");
					return -EFAULT;
					break;
			}
			break;

		default:
			break;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static ssize_t
mcc2_open( struct inode *inode, struct file *file )
{
	MOD_INC_USE_COUNT;
	return 0;
}



////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static int
mcc2_release( struct inode *inode, struct file *file )
{
	MOD_DEC_USE_COUNT;
	return 0;
}






#ifndef MODULE

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

long
mcc2_init( long mem_start, long mem_end )
{

	if ((mcc2_major = register_chrdev(MCC_MAJOR, MCC_NAME, &mcc2_fops)))
		printk("mcc2_init: Unable to get major for mcc2 device %d\n", MCC_MAJOR);
	else {
		//MPC82xxSiuInit();
		MPC82xxCpmInit();
	}
	return mem_start;
}

#else

