/* Copyright (C) 1996 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gdevlp8k.c,v 1.5 2004/08/10 13:02:36 stefan Exp $*/

/* EPSON LP-8000 ESC-sequence Laser Printer driver for Ghostscript.

This driver structure is most close to that of the Epson 'ESC/P 2' language
printer driver "gdevescp.c" contributed by Richard Brown, but all the control
sequences and data formats are totally different.

The main driver strategy is as follows. The driver scans lines, skips empty
ones, removes leading and trailing zeros for other lines, compresses the
non-zero rest of each line and finally outputs the data.

At the moment the driver supports only 300x300 DPI resolution.  If somebody
needs 240x240, another valid value for LP-8000 printer, he or she can try to
play with the corresponding values in initialization and termination
strings. Or I shall spend some extra time for hacking, if enough people
encourage me to do it. (The only available in our laboratory "Operation
guide" in Japanese does not contain any information about it. And LP-8000
driver for Japanese Windows does not support this mode either.)


The output data format is the following. 

1. Initialization string, pretty long and sophisticated, I don't know why it
was necessary.  

2. Data bits for each line. The most general format includes both starting X
and Y values as well as data type (simple or compressed).

3. Termination string.


	DATA FORMATS

1. A simple (non-compressed) data format. By evident reasons it is NOT
SUPPORTED by the driver and is discussed here just as a starting point for
the future explanations. "\035" here is an alias for 0x1d ESC-character :

"\035" "Starting X point in ASCII format" "X" 
"\035" "Starting Y point in ASCII format" "Y"
"\035" "Number of data BYTES for this printer line in ASCII format" ";" 
"Number of POINTS to print in this line (equals to the 
(Number of BYTES)*8)" ";" 
"1;obi{I" "data BYTES for this line in BINARY format"

Both X and Y printer coordinates are 60 pixels shifted from the corresponding
coordinates of the Ghostscript display, that is X = x - 60, Y = y - 60. For
example, 1 inch left margin requires the value of 300 - 60 = 240 for
starting X printer coordinate. Similar, 1.5 inch top margin requires Y
values to start from 300*1.5 - 60 = 390.

The shortest possible abbreviation for the simple data format string is 

"\035" "Starting Y point in ASCII format" "Y" 
"\035" "Number of data BYTES for this printer line in ASCII format" ";"
"Number of POINTS to print in this line (equals to the 
(Number of BYTES)*8)" ";" 
"1;obi{I" "data BYTES for this line in BINARY format"

In this case the value of the starting X point is assumed to be equal to
that for the previous line.
 
An example of the data output for 2 printer lines

"\035"315X"\035"240Y"\035"2;16;1;obi{I"0ff0""\035"241Y"\035"3;24;1;obi{I"0f000f"

Here "0ff0" is an alias for 0x0f 0xf0 binary data, etc. The first line of the
above example starts from X=315, Y=240 and consists of 2 data bytes
resulting in 4 blank (white) points followed by 8 black points followed by 4
white points on the paper. The second line starts from X=315, Y=241 and
contains 3 data bytes resulting in output of 4 white, 4 black, 12 white and
finally 4 black points.

2. Compressed data format (SUPPORTED BY THE DRIVER).

General description is as follows.

"\035" "Starting X point in ASCII format" "X" 
"\035" "Starting Y point in ASCII format" "Y"
"\035" "3bcI" 
"\035" "Total number of compressed BYTES in ASCII format" ";" 
"Number of POINTS to print in this line" ";"
"1;obi{I" "compressed data BYTES for this line in BINARY format"
"\035" "0bcI"

Additional ESC-sequences "\035" "3bcI" and "\035" "0bcI" mean start and end
of the compressed data format, respectively.  As in the discussed above case
of a non-compressed data format, the shortest abbreviation has the form of

"\035" "Starting Y point in ASCII format" "Y"
"\035" "Total number of compressed BYTES in ASCII format" ";" 
"Number of POINTS to print in this line" ";"
"1;obi{I" "compressed data BYTES for this line in BINARY format"

COMPRESSED DATA BYTES FORMAT has the form of 

"d1 d2 d3 d4 d4 count_d4 d5 d6 d6 count_d6 ... d(n-1) d(n-1) count_d(n-1) dn"

Here dx (x = 1 ... n) means data in a BINARY format. Any 2 repeated bytes
MUST follow by the count, otherwise the printer will interpret the next
data byte as a counter. The count value  indicates how many bytes of the
same value should be INSERTED after the repeated ones. So, the total number of
repeated bytes is (count + 2), not count. If there are only 2 equal data
bytes somewhere in the data stream, they MUST follow by zero.

Example of 2 compressed data strings.

"\035"105X"\035"320Y"\035"3bcI"\035"3;2048;1;obi{I"0000fe"
"\035"105X"\035"321Y"\035"11;2048;1;obi{I"0000021fffffe5fc000011" 

The first one containing 3 bytes of compressed data will result in empty
(zero) line of 2048 blank points started from X=105, Y=320. The second one
containing 11 compressed data bytes will produce the picture of 4*8 + 3 = 35
white points followed by 5 + 16 + 0xe5*8 + 6 = 1859 black points followed by
2 + 8*19 = 154 white points (total 2048 points) started from X=105, Y=321.

Strictly speaking, it was not necessary to adjust the number of points to
the byte boundary. I did it for the sake of simplicity. One more argument in
favor of this step is that the error of positioning does not exceed (7 /
300) inches or (7 / 118) cm, that is 0.6 mm, which is negligible, I guess.


ADDITIONAL INFORMATION

It is also possible to use LP-8000 printer with 180x180 DPI resolution as an
"ibmpro" device from gdevepsn.c The only thing which should be corrected, is
the value 0x30 in static const char ibmpro_init_string[]. Decimal 36
fixes the 1,5 times elongation along the vertical axis. It is also
recommended to choose the appropriate values for all margins. In my case it
was 0.2, 0.6, 0, 0.3 in the device descriptor instead of the 0.2, 0.95, 0,
1.0 

Nevertheless, typical Latex file looked so ugly after printing in this mode,
that I preferred to spend several days for hacking the format of the Japanese
Windows printer output for 300 DPI resolution and create my own driver.

Any suggestions, corrections, critical comments, etc. are welcome!

Oleg Fat'yanov  <faty1@rlem.titech.ac.jp> 

*/


#include "gdevprn.h"

#ifndef X_DPI
#define X_DPI 300
#endif
     
#ifndef Y_DPI
#define Y_DPI 300
#endif
     
#define L_MARGIN 0.25
#define B_MARGIN 0.25
#define R_MARGIN 0.25
#define T_MARGIN 0.25

private dev_proc_print_page(lp8000_print_page);

gx_device_printer far_data gs_lp8000_device =
  prn_device(prn_std_procs, "lp8000",
	DEFAULT_WIDTH_10THS,
	DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	L_MARGIN, B_MARGIN, R_MARGIN, T_MARGIN,
	1, lp8000_print_page);
                                          
       
private int
lp8000_print_page(gx_device_printer *pdev, FILE *prn_stream)
{

        int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
        int in_size = line_size;
                              
	byte *buf1 = (byte *)gs_malloc(pdev->memory, in_size, 1, "lp8000_print_page(buf1)");
        byte *buf2 = (byte *)gs_malloc(pdev->memory, in_size, 1, "lp8000_print_page(buf2)");
        byte *in = buf1;
        byte *out = buf2;
                        
        int lnum, top, bottom, left, width;
        int count, i, left1, left2, left0;

/* Check  memory allocations  */         
        
        if ( buf1 == 0 || buf2 == 0 )
        {       if ( buf1 )
        	gs_free(pdev->memory, (char *)buf1, in_size, 1, "lp8000_print_page(buf1)");
                
                if ( buf2 )
                gs_free(pdev->memory, (char *)buf2, in_size, 1, "lp8000_print_page(buf2)");
          
          return_error(gs_error_VMerror);
      	}

/* Initialize the printer */
       
	fwrite("\033\001@EJL \n",1,8,prn_stream);
	fwrite("@EJL EN LA=ESC/PAGE\n",1,20,prn_stream);
	fwrite("\035rhE\033\001@EJL \n",1,12,prn_stream);
	fwrite("@EJL SE LA=ESC/PAGE\n",1,20,prn_stream);
	fwrite("@EJL SET PU=1 PS=A4 ZO=OFF\n",1,27,prn_stream);
	fwrite("@EJL EN LA=ESC/PAGE\n",1,20,prn_stream);
	fwrite("\0350;0.24muE\0352;300;300drE",1,23,prn_stream);
	fwrite("\0350;300;300drE\0351tsE\0351mmE",1,23,prn_stream);
	fwrite("\0357isE\0355iaF\0355ipP\03514psE\0350poE",1,26,prn_stream);
	fwrite("\03560;60loE\0350X\0350Y",1,15,prn_stream);
	fwrite("\0350;0;2360;3388caE",1,17,prn_stream);
	fwrite("\0351cmE\0350alfP",1,11,prn_stream);
	fwrite("\0350affP\0350boP\0350abP",1,16,prn_stream);
	fwrite("\0354ilG\0350bcI\0350sarG",1,16,prn_stream);
	fwrite("\0351;0;100spE\0352owE",1,16,prn_stream);
	
/* Here the common part of the initialization string ends */


/* Calculate the PRINTER_LEFT_MARGIN = device_left_margin - 60 adjusted to
the byte boundary. Save this value for future comparison and set the
starting X value of the printer line.
*/
 	left1  =  (int) (L_MARGIN * pdev->x_pixels_per_inch) - 60;
 	left1 = (left1 >> 3) << 3;
	left0 = left1;

	fwrite("\035",1,1,prn_stream);
        fprintf(prn_stream,"%d",left1);
        fwrite("X",1,1,prn_stream);
        
	/* Set the compressed data format */
        fwrite("\0353bcI",1,5,prn_stream);
      
      	top = T_MARGIN * pdev->y_pixels_per_inch;
        bottom = pdev->height - B_MARGIN * pdev->y_pixels_per_inch;
        
	left  = ( (int) (L_MARGIN * pdev->x_pixels_per_inch) ) >> 3 ;
 	width = ((pdev->width - (int)(R_MARGIN * pdev->x_pixels_per_inch)) >> 3) - left;
 
	/*
	** Print the page:
	*/

	for ( lnum = top; lnum < bottom ; )


	{	
		byte *in_data;
		byte *inp;
		byte *in_end;
		byte *outp;
		register byte *p, *q;
		int lcnt;

		/*
		** Check buffer for 0 data.
		*/

		gdev_prn_get_bits(pdev, lnum, in, &in_data);
		while ( in_data[0] == 0 &&
		        !memcmp((char *)in_data, (char *)in_data + 1, line_size - 1) &&
		        lnum < bottom )
	    	{	
			lnum++;
			gdev_prn_get_bits(pdev, lnum, in, &in_data);
		}

		if(lnum == bottom ) break;	
		/* finished with this page */


		lcnt = gdev_prn_copy_scan_lines(pdev, lnum, in, in_size);

		inp = in  + left;
		in_end = inp + width;

/* Remove trailing 0s form the scan line data */

		while (in_end > inp &&  in_end[-1] == 0) 
		{
		in_end--;
		}
		
/* Remove leading 0s form the scan line data */
	
	for(left2 = 0; inp < in_end && inp[0] == 0; inp++,left2++);

/* Recalculate starting X value */
 
	left2 = left1 + (left2 << 3);



/* Compress  non-zero data for this line*/

		outp = out;

 for( p = inp, q = inp + 1 ; q < in_end ; )
 	{
	if( *p != *q++ ) 
	  { 
	  /* 
	  Copy non-repeated bytes 
	  to the output buffer  
	  */
	  *outp++ = *p++;
	  } 
	  else 
	    {
	    for (count = 2; ( *p == *q ) && (q < in_end); q++, count++);

		/* 
		Copy repeated bytes and counts to the output buffer. 
		As long as count is <= 255, additional step is necessary 
		for a long repeated sequence
		*/
	    	
	    	while (count > 257)
	    	{
	    	*outp++ = *p;
	    	*outp++ = *p;
	    	*outp++ = 255;
	    	p += 257;
	    	count -=257;
		}
		*outp++ = *p;
	    	*outp++ = *p;
	    	*outp++ = count - 2;
	    	p += count;
		q = p+1;
	    }		
	} 

/* The next line is necessary just in case of a single non-repeated byte at
the end of the input buffer */

if (p == (in_end - 1)) *outp++ = *p;

/* End of the compression procedure */
	

/* Set a new value of the starting X point, if necessary  */

if (left2 != left0)
	{
	left0 = left2;
	fwrite("\035",1,1,prn_stream);
        fprintf(prn_stream,"%d",left2);
        fwrite("X",1,1,prn_stream);
        }

/* Output the data string to the printer. 
Y coordinate of the printer equals (lnum - 60)
*/        	

        fwrite("\035",1,1,prn_stream);
        fprintf(prn_stream,"%d",lnum-60);
        fwrite("Y\035",1,2,prn_stream);
        fprintf(prn_stream,"%d;",(outp - out));
	fprintf(prn_stream,"%d;",(in_end - inp) << 3); 
	fwrite("1;0bi{I",1,7,prn_stream);       
        fwrite(out,1,(outp - out),prn_stream);                 
        
        lnum++;
        
        }

/* Send the termination string */

        fwrite("\0350bcI",1,5,prn_stream);                        
	fwrite("\0351coO",1,5,prn_stream);
    	fwrite("\035rhE",1,4,prn_stream); 
        
        fwrite("\033\001@EJL \n",1,8,prn_stream);
	fwrite("@EJL SE LA=ESC/PAGE\n",1,20,prn_stream);
	fwrite("@EJL SET PU=1 PS=A4 ZO=OFF\n",1,27,prn_stream);
	fwrite("@EJL EN LA=ESC/PAGE\n",1,20,prn_stream);
	fwrite("\0350;0.24muE\0352;300;300drE",1,23,prn_stream);
	fwrite("\0350;300;300drE\0351tsE\0351mmE",1,23,prn_stream);
	fwrite("\0357isE\0355iaF\0355ipP\03514psE\0350poE",1,26,prn_stream);
	fwrite("\03560;60loE\0350X\0350Y",1,15,prn_stream);
	fwrite("\0350;0;2360;3388caE",1,17,prn_stream);
	fwrite("\0351cmE\0350alfP",1,11,prn_stream);
	fwrite("\0350affP\0350boP\0350abP",1,16,prn_stream);
	fwrite("\0354ilG\0350bcI\0350sarG",1,16,prn_stream);
	fwrite("\035rhE",1,4,prn_stream);
	fwrite("\033\001@EJL \n",1,8,prn_stream);
	fwrite("\033\001@EJL \n",1,8,prn_stream);
	
	fflush(prn_stream);
	
	gs_free(pdev->memory, (char *)buf2, in_size, 1, "lp8000_print_page(buf2)");
	gs_free(pdev->memory, (char *)buf1, in_size, 1, "lp8000_print_page(buf1)");
	return 0;
}
