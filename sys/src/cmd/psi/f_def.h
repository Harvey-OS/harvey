
enum id_type_enu { F_RTI, F_CSI, F_TSI, F_NONE };

   /* F_RTI        + Ricoh machine remote terminal id (ascii string) 
      F_CSI        + call subscriber ID (phone # of remote machine ) 
      F_TSI        + transmitting subscriber ID (phone # of the remote) 
      F_NONE       + no id was returned */
    
enum mode_enu { F_EOP, F_MPS, F_EOM };
   
   /* F_EOP        + Transmit mode - this was the last page of the file being
		     transmitted. 
      F_MPS        + Transmit mode - the was not the last page of the file 
		     being transmitted. Do not perform Phase B negotiations
                     on the next page if the page was received OK.
      F_EOM        + Transmit mode - this was not the last page of the file
                     being transmitted.  There will be a new set of Phase B
                     negotiations before the next page.   */

enum resolution_enu { STD, FINE };
enum modem_enu { X2400, X4800, X7200, X9600 } ;
enum compression_enu { G3_1D, G3_2D };    /* MH or MR */
enum iospeed_enu { X40, X20, X10, X5, X0 };
enum length_enu { A4_L, B4_L, UNLIMITED_L };
enum width_enu { A4_W, A3_W, B4_W };
enum machine_id_enu { RICOH, NONRICOH };
enum group_enu { G2, G3 };

/********** Structures ************/

struct negconfig_st
{
	long	resolution;
	long		modem;
	long	compression;
	long		group;
	long	machine_id;
	long		length;
	long		width;
};
