/* dip.c
 * Digital Image Processing routines
 * (c) 2000-2002 Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 * This does various utilities for digital image processing including font
 * rendering.
 */

#include "cfg.h"

#ifdef G

#include "links.h"

#ifdef HAVE_MATH_H
#include <math.h>
#endif /* HAVE_MATH_H */

#endif
int dither_letters=1;

double user_gamma=1.0; /* 1.0 for 64 lx. This is the number user directly changes in the menu */
			  
double display_red_gamma=2.2; /* Red gamma exponent of the display */
double display_green_gamma=2.2; /* Green gamma exponent of the display */
double display_blue_gamma=2.2; /* Blue gamma exponent of the display */
#ifdef G

/* #define this if you want to report missing letters to stderr.
 * Leave it commented up for normal operation and releases! */
/* #define REPORT_UNKNOWN 1 */

double sRGB_gamma=0.45455;      /* For HTML, which runs
				 * according to sRGB standard. Number
				 * in HTML tag is linear to photons raised
				 * to this power.
				 */

unsigned long aspect=65536; /* aspect=65536 for 320x240
		        * aspect=157286 for 640x200
                        * Defines aspect ratio of screen pixels. 
		        * aspect=(196608*xsize+ysize<<1)/(ysize<<1);
		        * Default is 65536 because we assume square pixel
			* when not specified otherwise. */
unsigned long aspect_native=65536; /* Like aspect, but not influenced by
			* user, just determined by graphics driver.
			*/

/* Limitation: No input image's dimension may exceed 2^(32-1-8) pixels.
 */

/* Each input byte represents 1 byte (gray). The question whether 0 is
 * black or 255 is black doesn't matter.
 */

/* These constants represent contrast-enhancement and sharpen filter (which is one filter
 * together) that is applied onto the letters to enhance their appearance at low height.
 * They were determined by experiment for several values, interpolated, checked and tuned.
 * If you change them, don't wonder if the letters start to look strange.
 * The numers in the comments denote which height the line applies for.
 */
float fancy_constants[64]={
	0,3,		/*  1 */
	.1,3,   	/*  2 */
	.2,3,   	/*  3 */
	.3,2.9,   	/*  4 */
	.4,2.7,   	/*  5 */
	.4,2.5,   	/*  6 */
	.4,2,   	/*  7 */
	.5,2, 		/*  8 */
	.4,2, 		/*  9 */
	.38,1.9, 	/* 10 */
	.36,1.8, 	/* 11 */
	.33,1.7, 	/* 12 */
	.30,1.6, 	/* 13 */
	.25,1.5, 	/* 14 */
	.20,1.5, 	/* 15 */
	.15,1.5, 	/* 16 */
	.14,1.5, 	/* 17 */
	.13,1.5, 	/* 18 */
	.12,1.5, 	/* 19 */
	.12,1.5, 	/* 20 */
	.12,1.5, 	/* 21 */
	.12,1.5, 	/* 22 */
	.11,1.5, 	/* 23 */
	.10,1.4, 	/* 24 */
	.09,1.3, 	/* 25 */
	.08,1.3, 	/* 26 */
	.04,1.2, 	/* 27 */
	.04,1.2, 	/* 28 */
	.02,1.1, 	/* 29 */
	.02,1.1, 	/* 30 */
	.01,1, 		/* 31 */
	.01,1  		/* 32 */
};

/* This shall be hopefully reasonably fast and portable
 * We assume ix is <65536. If not, the letters will be smaller in
 * horizontal dimension (because of overflow) but this will not cause
 * a segfault. 65536 pixels wide bitmaps are not normal and will anyway
 * exhaust the memory.
 */
static inline int compute_width (int ix, int iy, int required_height)
{
	int width;
	unsigned long reg;
	
	reg=(unsigned long)aspect*(unsigned long)required_height;
	
	if (reg>=0x1000000UL){
		/* It's big */
		reg=(reg+32768)>>16;
		width=(reg*ix+(iy>>1))/iy;
	}else{
		/* It's small */
		reg=(reg+128)>>8;
		iy<<=8;
		width=(reg*ix+(iy>>1))/iy;
	}
	if (width<1) width=1;
	return width;
}

struct lru font_cache; /* This is a cache for screen-ready colored bitmaps
                        * of lettrs and/or alpha channels for these (are the
			* same size, only one byte per pixel and are used
			* for letters with an image under them )
			*/

/* Each input byte represents 1 byte (gray). The question whether 0 is
 * black or 255 is black doesn't matter.
 */

inline static void add_col_gray(unsigned *col_buf, unsigned char *ptr, int
		line_skip, int n, unsigned weight)
{
	for (;n;n--){
		*col_buf+=weight*(*ptr);
		ptr+=line_skip;
		col_buf++;
	}
}

/* line_skip is in pixels. The column contains the whole pixels (R G B)
 * We assume unsigned short holds at least 16 bits. */
inline static void add_col_color(unsigned *col_buf, unsigned short *ptr
	, int line_skip, int n, unsigned weight)
{
	for (;n;n--){
		*col_buf+=weight*(*ptr);
		col_buf[1]+=weight*ptr[1];
		col_buf[2]+=weight*ptr[2];
		ptr+=line_skip;
		col_buf+=3;
	}
}

 /* We assume unsigned short holds at least 16 bits. */
inline static void add_row_gray(unsigned *row_buf, unsigned char *ptr, int n,
	unsigned weight)
{
	for (;n;n--){
		*row_buf+=weight**ptr;
		ptr++;
		row_buf++;
	}
}

/* n is in pixels. pixel is 3 unsigned shorts in series */
 /* We assume unsigned short holds at least 16 bits. */
inline static void add_row_color(unsigned *row_buf, unsigned short *ptr, int n, unsigned weight)
{
	for (;n;n--){
		*row_buf+=weight**ptr;
		row_buf[1]+=weight*ptr[1];
		row_buf[2]+=weight*ptr[2];
		ptr+=3;
		row_buf+=3;
	}
}

/* We assume unsigned holds at least 32 bits */
inline static void emit_and_bias_col_gray(unsigned *col_buf, unsigned char *out, int
		line_skip, int n, unsigned weight)
{
	unsigned half=weight>>1;

	for (;n;n--){
		*out=*col_buf/weight;
		out+=line_skip;
		*col_buf++=half;
	}
}

/* We assume unsigned holds at least 32 bits */
static inline void bias_buf_gray(unsigned *col_buf, int n, unsigned half)
{
	for (;n;n--) *col_buf++=half;
}

/* We assume unsigned holds at least 32 bits */
inline static void bias_buf_color(unsigned *col_buf, int n, unsigned half)
{
	for (;n;n--){
		*col_buf=half;
		col_buf[1]=half;
		col_buf[2]=half;
		col_buf+=3;
	}
	/* System activated */
}
		
/* line skip is in pixels. Pixel is 3*unsigned short */
/* We assume unsigned holds at least 32 bits */
/* We assume unsigned short holds at least 16 bits. */
inline static void emit_and_bias_col_color(unsigned *col_buf
	, unsigned short *out, int line_skip, int n, unsigned weight)
{
	unsigned half=weight>>1;

	for (;n;n--){
		*out=(*col_buf)/weight;
		*col_buf=half;
		out[1]=col_buf[1]/weight;
		col_buf[1]=half;
		/* The following line is an enemy of the State and will be
		 * prosecuted according to the Constitution of The United States
		 * Cap. 20/3 ix. Sel. Bill 12/1920
		 * Moses 12/20 Erizea farizea 2:2:1:14
		 */
		out[2]=col_buf[2]/weight;
		col_buf[2]=half;
		out+=line_skip;
		col_buf+=3;
	}
}
		
/* We assume unsigned holds at least 32 bits */
inline static void emit_and_bias_row_gray(unsigned *row_buf, unsigned char *out, int n
		,unsigned weight)
{
	unsigned half=weight>>1;
	
	for (;n;n--){
		*out++=*row_buf/weight;
		*row_buf++=half;
	}
}

/* n is in pixels. pixel is 3 unsigned shorts in series. */
/* We assume unsigned holds at least 32 bits */
/* We assume unsigned short holds at least 16 bits. */
inline static void emit_and_bias_row_color(unsigned *row_buf, unsigned short
		*out, int n, unsigned weight)
{
	unsigned half=weight>>1;

	for (;n;n--){
		*out=*row_buf/weight;
		*row_buf=half;
		out[1]=row_buf[1]/weight;
		row_buf[1]=half;
		out[2]=row_buf[2]/weight;
		row_buf[2]=half;
		out+=3;
		row_buf+=3;
	}
}
		
/* For enlargement only -- does linear filtering.
 * Allocates output and frees input.
 * We assume unsigned holds at least 32 bits */
inline static void enlarge_gray_horizontal(unsigned char *in, int ix, int y
	,unsigned char ** out, int ox)
{
	unsigned *col_buf;
	int total;
	int out_pos,in_pos,in_begin,in_end;
	unsigned half=(ox-1)>>1;
	unsigned char *outptr;
	unsigned char *inptr;

	outptr=mem_alloc(ox*y);
	inptr=in;
	*out=outptr;
	if (ix==1){
		/* Dull copying */
		for (;y;y--){
			memset(outptr,*inptr,ox);
			outptr+=ox;
			inptr++;
		}
		mem_free(in);
	}else{
		total=(ix-1)*(ox-1);
		col_buf=mem_alloc(y*sizeof(*col_buf));
		bias_buf_gray(col_buf, y, half);
		out_pos=0;
		in_pos=0;
		again:
		in_begin=in_pos;
		in_end=in_pos+ox-1;
		add_col_gray(col_buf,inptr, ix, y, in_end-out_pos);
		add_col_gray(col_buf,inptr+1, ix, y, out_pos-in_begin);
		emit_and_bias_col_gray(col_buf,outptr,ox,y,ox-1);
		outptr++;
		out_pos+=ix-1;
		if (out_pos>in_end){
			in_pos=in_end;
			inptr++;
		}
		if (out_pos>total){
			mem_free(in);
			mem_free(col_buf);
			return;
		}
		goto again;
	}
	/* Rohan, oh Rohan... */
	/* ztracena zeme :) */
}

/* For enlargement only -- does linear filtering
 * Frees input and allocates output.
 * We assume unsigned holds at least 32 bits
 */
static inline void enlarge_color_horizontal(unsigned short *ina, int ix, int y,
	unsigned short ** outa, int ox)
{
	unsigned *col_buf;
	int total,a,out_pos,in_pos,in_begin,in_end;
	unsigned half=(ox-1)>>1;
	unsigned skip=3*ix;
	unsigned oskip=3*ox;
	unsigned short *out, *in;

	if (ix==ox){
		*outa=ina;
		return;
	}
	out=mem_alloc(sizeof(*out)*3*ox*y);
	*outa=out;
	in=ina;
	if (ix==1){
		for (;y;y--,in+=3) for (a=ox;a;a--,out+=3){
			*out=*in;
			out[1]=in[1];
			out[2]=in[2];
		}
		mem_free(ina);
		return;
	}
	total=(ix-1)*(ox-1);
	col_buf=mem_alloc(y*3*sizeof(*col_buf));
	bias_buf_color(col_buf,y,half);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+ox-1;
	add_col_color(col_buf,in,skip,y
		,in_end-out_pos);
	add_col_color(col_buf,in+3,skip,y
		,out_pos-in_begin);
	emit_and_bias_col_color(col_buf,out,oskip,y,ox-1);
	out+=3;
	out_pos+=ix-1;
	if (out_pos>in_end){
		in_pos=in_end;
		in+=3;
	}
	if (out_pos>total){
		mem_free(col_buf);
		mem_free(ina);
		return;
	}
	goto again;
}

/* Works for both enlarging and diminishing. Linear resample, no low pass.
 * Automatically mem_frees the "in" and allocates "out". */
/* We assume unsigned holds at least 32 bits */
inline static void scale_gray_horizontal(unsigned char *in, int ix, int y
	,unsigned char ** out, int ox)
{
	unsigned *col_buf;
	int total=ix*ox;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned char *outptr;
	unsigned char *inptr;

	if (ix<ox){
		enlarge_gray_horizontal(in,ix,y,out,ox);
		return;
	}else if (ix==ox){
		*out=in;
		return;
	}
	outptr=mem_alloc(ox*y);
	inptr=in;
	*out=outptr;
	col_buf=mem_alloc(y*sizeof(*col_buf));
	bias_buf_gray(col_buf, y, ix>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+ox;
	out_end=out_pos+ix;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_col_gray(col_buf,inptr,ix,y,in_end-in_begin);
	in_end=in_pos+ox;
	if (out_end>=in_end){
		in_pos=in_end;
		inptr++;
	}
	if (out_end<=in_end){
			emit_and_bias_col_gray(col_buf,outptr,ox,y,ix);
			out_pos=out_pos+ix;
			outptr++;
	}
	if (out_pos==total) {
		mem_free(in);
		mem_free(col_buf);
		return;
	}
	goto again;
}

/* Works for both enlarging and diminishing. Linear resample, no low pass.
 * Does only one color component.
 * Frees ina and allocates outa.
 * If ox*3<=ix, and display_optimize, performs optimization for LCD.
 */
inline static void scale_color_horizontal(unsigned short *ina, int ix, int y,
		unsigned short **outa, int ox)
{
	unsigned *col_buf;
	int total=ix*ox;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned skip=3*ix;
	unsigned oskip=3*ox;
	unsigned short *in, *out;

	if (ix==ox){
		*outa=ina;
		return;
	}
	if (ix<ox){
		enlarge_color_horizontal(ina,ix,y,outa,ox);
		return;
	}else if (ix==ox){
		*outa=ina;
		return;
	}
	out=mem_alloc(sizeof(*out)*3*ox*y);
	*outa=out;
	in=ina;
	col_buf=mem_alloc(y*3*sizeof(*col_buf));
	bias_buf_color(col_buf,y,ix>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+ox;
	out_end=out_pos+ix;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_col_color(col_buf,in,skip,y,in_end-in_begin);
	in_end=in_pos+ox;
	if (out_end>=in_end){
		in_pos=in_end;
		in+=3;
	}
	if (out_end<=in_end){
			emit_and_bias_col_color(col_buf,out,oskip,y,ix);
			out_pos=out_pos+ix;
			out+=3;
	}
	if (out_pos==total) {
		mem_free(ina);
		mem_free(col_buf);
		return;
	}
	goto again;
}

/* For magnification only. Does linear filtering. */
/* We assume unsigned holds at least 32 bits */
inline static void enlarge_gray_vertical(unsigned char *in, int x, int iy,
	unsigned char ** out ,int oy)
{
	unsigned *row_buf;
	int total;
	int out_pos,in_pos,in_begin,in_end;
	int half=(oy-1)>>1;
	unsigned char *outptr;
	unsigned char *inptr;

	if (iy==1){
		outptr=mem_alloc(oy*x);
		*out=outptr;
		for(;oy;oy--,outptr+=x)
			memcpy(outptr,in,x);
		mem_free(in);
	}
	else if (iy==oy){
		*out=in;
	}else{
		outptr=mem_alloc(oy*x);
		inptr=in;
		*out=outptr;
		total=(iy-1)*(oy-1);
		row_buf=mem_alloc(x*sizeof(*row_buf));
		bias_buf_gray(row_buf, x, half);
		out_pos=0;
		in_pos=0;
		again:
		in_begin=in_pos;
		in_end=in_pos+oy-1;
		add_row_gray(row_buf, inptr, x, in_end-out_pos);
		add_row_gray(row_buf, inptr+x, x, out_pos-in_begin);
		emit_and_bias_row_gray(row_buf, outptr, x, oy-1);
		outptr+=x;
		out_pos+=iy-1;
		if (out_pos>in_end){
			in_pos=in_end;
			inptr+=x;
		}
		if (out_pos>total){
			mem_free(in);
			mem_free(row_buf);
			return;
		}
		goto again;
	}	
}

/* For magnification only. Does linear filtering */
/* We assume unsigned holds at least 32 bits */
inline static void enlarge_color_vertical(unsigned short *ina, int x, int iy,
	unsigned short **outa ,int oy)
{
	unsigned *row_buf;
	int total,out_pos,in_pos,in_begin,in_end;
	int half=(oy-1)>>1;
	unsigned short *out, *in;

	if (iy==oy){
		*outa=ina;
		return;
	}
	/* Rivendell */
	out=mem_alloc(sizeof(*out)*3*oy*x);
	*outa=out;
	in=ina;
	if (iy==1){
		for (;oy;oy--){
	       		memcpy(out,in,3*x*sizeof(*out));
	       		out+=3*x;
		}
		mem_free(ina);
		return;
	}
	total=(iy-1)*(oy-1);
	row_buf=mem_alloc(x*3*sizeof(*row_buf));
	bias_buf_color(row_buf,x,half);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+oy-1;
	add_row_color(row_buf,in,x
		,in_end-out_pos);
	add_row_color(row_buf,in+3*x,x
		,out_pos-in_begin);
	emit_and_bias_row_color(row_buf,out,x,oy-1);
	out+=3*x;
	out_pos+=iy-1;
	if (out_pos>in_end){
		in_pos=in_end;
		in+=3*x;
	}
	if (out_pos>total){
		mem_free(ina);
		mem_free(row_buf);
		return;
	}
	goto again;
	
}	

/* Both enlarges and diminishes. Linear filtering.
 * Automatically allocates output and frees input.
 * We assume unsigned holds at least 32 bits */
inline static void scale_gray_vertical(unsigned char *in, int x, int iy,
	unsigned char ** out ,int oy)
{
	unsigned *row_buf;
	int total=iy*oy;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned char *outptr;
	unsigned char *inptr;

	/* Snow White, Snow White... */
	if (iy<oy){
		enlarge_gray_vertical(in,x,iy,out,oy);
		return;
	}
	if (iy==oy){
		*out=in;
		return;
	}
	outptr=mem_alloc(x*oy);
	inptr=in;
	*out=outptr;
	row_buf=mem_calloc(x*sizeof(*row_buf));
	bias_buf_gray(row_buf, x, iy>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+oy;
	out_end=out_pos+iy;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_row_gray(row_buf,inptr,x,in_end-in_begin);
	in_end=in_pos+oy;
	if (out_end>=in_end){
		in_pos=in_end;
		inptr+=x;
	}
	if (out_end<=in_end){
			emit_and_bias_row_gray(row_buf,outptr,x,iy);
			out_pos=out_pos+iy;
			outptr+=x;
	}
	if (out_pos==total){
		mem_free(in);
		mem_free(row_buf);
		return;
	}
	goto again;
}

/* Both enlarges and diminishes. Linear filtering. Sizes are
   in pixels. Sizes are not in bytes. 1 pixel=3 unsigned shorts.
   We assume unsigned short can hold at least 16 bits.
   We assume unsigned holds at least 32 bits.
 */
inline static void scale_color_vertical(unsigned short *ina, int x, int iy
	,unsigned short **outa, int oy)
{
	unsigned *row_buf;
	int total=iy*oy;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned short *in, *out;

	if (iy==oy){
		*outa=ina;
		return;
	}
	if (iy<oy){
		enlarge_color_vertical(ina,x,iy,outa,oy);
		return;
	}
	out=mem_alloc(sizeof(*out)*3*oy*x);
	*outa=out;
	in=ina;
	row_buf=mem_alloc(x*3*sizeof(*row_buf));
	bias_buf_color(row_buf,x,iy>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+oy;
	out_end=out_pos+iy;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_row_color(row_buf,in,x,in_end-in_begin);
	in_end=in_pos+oy;
	if (out_end>=in_end){
		in_pos=in_end;
		in+=3*x;
	}
	if (out_end<=in_end){
			emit_and_bias_row_color(row_buf,out,x,iy);
			out_pos=out_pos+iy;
			out+=3*x;
	}
	if (out_pos==total){
		mem_free(ina);
		mem_free(row_buf);
		return;
	}
	goto again;
}


/* Scales grayscale 8-bit map. Both enlarges and diminishes. Uses either low
 * pass or bilinear filtering. Automatically mem_frees the "in".
 * Automatically allocates "out".
 */
inline static void scale_gray(unsigned char *in, int ix, int iy, unsigned char **out
	,int ox, int oy)
{
	unsigned char *intermediate_buffer;

	if (!ix||!iy){
		if (in) mem_free(in);
		*out=mem_calloc(ox*oy);
		return;
	}
	if (ix*oy<ox*iy){
		scale_gray_vertical(in,ix,iy,&intermediate_buffer,oy);
		scale_gray_horizontal(intermediate_buffer,ix,oy,out,ox);
	}else{
		scale_gray_horizontal(in,ix,iy,&intermediate_buffer,ox);
		scale_gray_vertical(intermediate_buffer,ox,iy,out,oy);
	}
}

/* To be called only when global variable display_optimize is 1 or 2.
 * Performs a decimation according to this variable. Data shrink to 1/3
 * and x is the smaller width.
 * There must be 9*x*y unsigned shorts of data.
 * x must be >=1.
 * Performs realloc onto the buffer after decimation to save memory.
 */
void decimate_3(unsigned short **data0, int x, int y)
{
	unsigned short *data=*data0;
	unsigned short *ahead=data;
	int i, futuresize=x*y*3*sizeof(**data0);
	
#ifdef DEBUG
	if (!(x>0&&y>0)) internal((unsigned char *)"zero width or height in decimate_3");
#endif /* #Ifdef DEBUG */
	if (display_optimize==1){
		if (x==1){
			for (;y;y--,ahead+=9,data+=3){
				data[0]=(ahead[0]+ahead[0]+ahead[3])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[5]+ahead[8]+ahead[8])/3;
			}
		}else{
			for (;y;y--){
				data[0]=(ahead[0]+ahead[0]+ahead[3])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[5]+ahead[8]+ahead[11])/3;
				for (ahead+=9,data+=3,i=x-2;i;i--,ahead+=9,data+=3){
					data[0]=(ahead[-3]+ahead[0]+ahead[3])/3;
					data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
					data[2]=(ahead[5]+ahead[8]+ahead[11])/3;
				}
				data[0]=(ahead[-3]+ahead[0]+ahead[3])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[5]+ahead[8]+ahead[8])/3;
				ahead+=9,data+=3;
			}
		}
	}else{
		/* display_optimize==2 */
		if (x==1){
			for (;y;y--,ahead+=9,data+=3){
				data[0]=(ahead[3]+ahead[6]+ahead[6])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[2]+ahead[2]+ahead[5])/3;
			}
		}else{
			for (;y;y--){
				data[0]=(ahead[3]+ahead[6]+ahead[9])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[2]+ahead[2]+ahead[5])/3;
				for (ahead+=9,data+=3,i=x-2;i;i--,ahead+=9,data+=3){
					data[0]=(ahead[3]+ahead[6]+ahead[9])/3;
					data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
					data[2]=(ahead[-1]+ahead[2]+ahead[5])/3;
				}
				data[0]=(ahead[3]+ahead[6]+ahead[6])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[-1]+ahead[2]+ahead[5])/3;
				ahead+=9,data+=3;
			}
		}
	}	
	*data0=mem_realloc(*data0,futuresize);
}

/* Scales color 48-bits-per-pixel bitmap. Both enlarges and diminishes. Uses
 * either low pass or bilinear filtering. The memory organization for both
 * input and output are red, green, blue. All three of them are unsigned shorts 0-65535.
 * Allocates output and frees input
 * We assume unsigned short holds at least 16 bits.
 */
void scale_color(unsigned short *in, int ix, int iy, unsigned short **out,
	int ox, int oy)
{
	unsigned short *intermediate_buffer;
	int do_optimize;
	int ox0=ox;

	if (!ix||!iy){
		if (in) mem_free(in);
		*out=mem_calloc(ox*oy*sizeof(**out)*3);
		return;
	}
	if (display_optimize&&ox*3<=ix){
		do_optimize=1;
		ox0=ox;
		ox*=3;
	}else do_optimize=0;
	if (ix*oy<ox*iy){
		scale_color_vertical(in,ix,iy,&intermediate_buffer,oy);
		scale_color_horizontal(intermediate_buffer,ix,oy,out,ox);
	}else{
		scale_color_horizontal(in,ix,iy,&intermediate_buffer,ox);
		scale_color_vertical(intermediate_buffer,ox,iy,out,oy);
	}
	if (do_optimize) decimate_3(out, ox0, oy);
}

/* Fills a block with given color. length is number of pixels. pixel is a
 * tribyte. 24 bits per pixel.
 */
void mix_one_color_24(unsigned char *dest, int length,
		   unsigned char r, unsigned char g, unsigned char b)
{
	for (;length;length--){
		dest[0]=r;
		dest[1]=g;
		dest[2]=b;
		dest+=3;
	}
}

/* Fills a block with given color. length is number of pixels. pixel is a
 * tribyte. 48 bits per pixel.
 * We assume unsigned short holds at least 16 bits.
 */
void mix_one_color_48(unsigned short *dest, int length,
		   unsigned short r, unsigned short g, unsigned short b)
{
	for (;length;length--){
		dest[0]=r;
		dest[1]=g;
		dest[2]=b;
		dest+=3;
	}
}

/* Mixes ink and paper of a letter, using alpha as alpha mask.
 * Only mixing in photon space makes physical sense so that the input values
 * must always be equivalent to photons and not to electrons!
 * length is number of pixels. pixel is a tribyte
 * alpha is 8-bit, rgb are all 16-bit
 * We assume unsigned short holds at least 16 bits.
 */
inline static void mix_two_colors(unsigned short *dest, unsigned char *alpha, int length
	,unsigned short r0, unsigned short g0, unsigned short b0,
	unsigned short r255, unsigned short g255, unsigned short b255)
{
	unsigned mask,cmask;
	
	for (;length;length--){
		mask=*alpha++;
		if (((unsigned char)(mask+1))>=2){
			cmask=255-mask;
			dest[0]=(mask*r255+cmask*r0+127)/255;
			dest[1]=(mask*g255+cmask*g0+127)/255;
			dest[2]=(mask*b255+cmask*b0+127)/255;
		}else{
			if (mask){
				dest[0]=r255;
				dest[1]=g255;
				dest[2]=b255;
			}else{
				dest[0]=r0;
				dest[1]=g0;
				dest[2]=b0;
			}
		}
		dest+=3;
	}
}

/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_32_to_48_table(unsigned short *dest,
		unsigned char *src, int lenght, unsigned short *table
		,unsigned short rb, unsigned short gb, unsigned short bb)
{
	unsigned alpha, ri, gi, bi, calpha;

	for (;lenght;lenght--)
	{
		ri=table[src[0]];
		gi=table[src[1]+256];
		bi=table[src[2]+512];
		alpha=src[3];
		src+=4;
		if (((unsigned char)(alpha+1))>=2){
			calpha=255U-alpha;
			dest[0]=(ri*alpha+calpha*rb+127)/255;
			dest[1]=(gi*alpha+calpha*gb+127)/255;
			dest[2]=(bi*alpha+calpha*bb+127)/255;
		}else{
			if (alpha){
				dest[0]=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				dest[0]=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of four-bytes RGBA. All bytes are gamma corrected. length is
 * number of pixels. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work. rb, gb, bb are 0-65535
 * in linear monitor output photon space
 */
/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_32_to_48(unsigned short *dest,
		unsigned char *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma, unsigned short rb, unsigned
		short gb, unsigned short bb)
{
	float r,g,b;
	unsigned alpha, calpha;
	unsigned ri,gi,bi;
	const float inv_255=1/255.0;

	for (;lenght;lenght--)
	{
		r=src[0];
		g=src[1];
		b=src[2];
		alpha=src[3];
		src+=4;
		r*=inv_255;
		g*=inv_255;
		b*=inv_255;
		r=pow(r,red_gamma);
		g=pow(g,green_gamma);
		b=pow(b,blue_gamma);
		ri=(r*65535)+0.5;
		gi=(g*65535)+0.5;
		bi=(b*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		/* To prevent segfaults in case of crappy floating arithmetics
		 */
		if (ri>=65536) ri=65535;
		if (gi>=65536) gi=65535;
		if (bi>=65536) bi=65535;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		if (((alpha+1U)&0xffU)>=2U){
			calpha=255U-alpha;
			*dest=(ri*alpha+calpha*rb+127U)/255U;
			dest[1]=(gi*alpha+calpha*gb+127U)/255U;
			dest[2]=(bi*alpha+calpha*bb+127U)/255U;
		}else{
			if (alpha){
				*dest=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				*dest=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of four-bytes RGBA. All bytes are gamma corrected. length is
 * number of pixels. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work. rb, gb, bb are 0-65535
 * in linear monitor output photon space. alpha 255 means full image no background.
 */
/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_64_to_48(unsigned short *dest,
		unsigned short *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma, unsigned short rb, unsigned
		short gb, unsigned short bb)
{
	float r,g,b;
	unsigned alpha, calpha;
	unsigned short ri,gi,bi;
	const float inv_65535=1/((float)65535);

	for (;lenght;lenght--)
	{
		r=src[0];
		g=src[1];
		b=src[2];
		alpha=src[3];
		src+=4;
		r*=inv_65535;
		g*=inv_65535;
		b*=inv_65535;
		r=pow(r,red_gamma);
		g=pow(g,green_gamma);
		b=pow(b,blue_gamma);
		ri=r*65535+0.5;
		gi=g*65535+0.5;
		bi=b*65535+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		/* To prevent segfaults in case of crappy floating arithmetics
		 */
		if (ri>=65536) ri=65535;
		if (gi>=65536) gi=65535;
		if (bi>=65536) bi=65535;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		if (((alpha+1U)&255U)>=2U){
			calpha=65535U-alpha;
			*dest=(ri*alpha+calpha*rb+32767U)/65535U;
			dest[1]=(gi*alpha+calpha*gb+32767U)/65535U;
			dest[2]=(bi*alpha+calpha*bb+32767U)/65535U;
		}else{
			if (alpha){
				*dest=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				*dest=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of four-bytes RGBA. All bytes are gamma corrected. length is
 * number of pixels. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work. rb, gb, bb are 0-65535
 * in linear monitor output photon space. alpha 255 means full image no background.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_64_to_48_table(unsigned short *dest
		,unsigned short *src, int lenght, unsigned short *gamma_table
		,unsigned short rb, unsigned short gb, unsigned short bb)
{
	unsigned alpha, calpha;
	unsigned short ri,gi,bi;

	for (;lenght;lenght--)
	{
		ri=gamma_table[*src];
		gi=gamma_table[src[1]+65536];
		bi=gamma_table[src[2]+131072];
		alpha=src[3];
		src+=4;
		if (((alpha+1)&0xffff)>=2){
			calpha=65535-alpha;
			*dest=(ri*alpha+calpha*rb+32767)/65535;
			dest[1]=(gi*alpha+calpha*gb+32767)/65535;
			dest[2]=(bi*alpha+calpha*bb+32767)/65535;
		}else{
			if (alpha){
				*dest=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				*dest=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of three-bytes. All bytes are gamma corrected. length is
 * number of triplets. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_48_to_48(unsigned short *dest,
		unsigned short *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma)
{
	float a, inv_65535=1/((float)65535);

	for (;lenght;lenght--,src+=3,dest+=3)
	{
		a=*src;
		a*=inv_65535;
		a=pow(a,red_gamma);
		*dest=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (*dest>=0x10000) *dest=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[1];
		a*=inv_65535;
		a=pow(a,green_gamma);
		dest[1]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[1]>=0x10000) dest[1]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[2];
		a*=inv_65535;
		a=pow(a,blue_gamma);
		dest[2]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[2]>=0x10000) dest[2]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	}
}

/* src is a block of three-bytes. All bytes are gamma corrected. length is
 * number of triples. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_48_to_48_table(unsigned short *dest,
		unsigned short *src, int lenght, unsigned short *table)
{
	for (;lenght;lenght--,src+=3,dest+=3)
	{
		*dest=table[*src];
		dest[1]=table[src[1]+65536];
		dest[2]=table[src[2]+131072];
	}
}

/* src is a block of three-bytes. All bytes are gamma corrected. length is
 * number of triples. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_24_to_48(unsigned short *dest, unsigned char *src, int
			  lenght, float red_gamma, float green_gamma, float
			  blue_gamma)
{
	float a;
	float inv_255=1/((float)255);
	
	for (;lenght;lenght--,src+=3,dest+=3)
	{
		a=*src;
		a*=inv_255;
		a=pow(a,red_gamma);
		*dest=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (*dest>=0x10000) *dest=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[1];
		a*=inv_255;
		a=pow(a,green_gamma);
		dest[1]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[1]>=0x10000) dest[1]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[2];
		a*=inv_255;
		a=pow(a,blue_gamma);
		dest[2]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[2]>=0x10000) dest[2]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	}
}

/* Allocates new gamma_table and fills it with mapping 8 bits ->
 * power to user_gamma/cimg->*_gamma -> 16 bits 
 * We assume unsigned short holds at least 16 bits. */
void make_gamma_table(struct cached_image *cimg)
{
	double rg=user_gamma/cimg->red_gamma;
	double gg=user_gamma/cimg->green_gamma;
	double bg=user_gamma/cimg->blue_gamma;
	double inv;
	int a;
	unsigned short *ptr_16;

	if (cimg->buffer_bytes_per_pixel<=4){
		/* 8-bit */
		inv=1/((double)255);
		ptr_16=mem_alloc(768*sizeof(*(cimg->gamma_table)));
		cimg->gamma_table=ptr_16;
		for (a=0;a<256;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,rg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		/* To test against crappy arithmetics */
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<256;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,gg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<256;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,bg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
	}else{
		/* 16-bit */
		inv=1/((double)65535);
		ptr_16=mem_alloc(196608*sizeof(*(cimg->gamma_table)));
		cimg->gamma_table=ptr_16;
		for (a=0;a<0x10000;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,rg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<0x10000;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,gg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<0x10000;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,bg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
	}
}

/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_24_to_48_table(unsigned short *dest, unsigned char *src, int
			  lenght, unsigned short *table)
{
	for (;lenght;lenght--,src+=3,dest+=3)
	{
		dest[0]=table[src[0]];
		dest[1]=table[src[1]+256];
		dest[2]=table[src[2]+512];
	}
}

/* Input is 0-255 (8-bit). Output is 0-255 (8-bit)*/
unsigned char apply_gamma_single_8_to_8(unsigned char input, float gamma)
{
	return 255*pow(((float) input)/255,gamma)+0.5;
}

/* Input is 0-255 (8-bit). Output is 0-65535 (16-bit)*/
/* We assume unsigned short holds at least 16 bits. */
unsigned short apply_gamma_single_8_to_16(unsigned char input, float gamma)
{
	float a=input;
	unsigned short retval;

	a/=255;
	a=pow(a,gamma);
	a*=65535;
	retval = a+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (retval>=0x10000) retval=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	return retval;
}

/* Input is 0-65535 (16-bit). Output is 0-255 (8-bit)*/
/* We assume unsigned short holds at least 16 bits. */
unsigned char apply_gamma_single_16_to_8(unsigned short input, float gamma)
{
	return pow(((float)input)/65535,gamma)*255+0.5;
}

/* Input is 0-65535 (16-bit). Output is 0-255 (8-bit)*/
unsigned short apply_gamma_single_16_to_16(unsigned short input, float gamma)
{
	unsigned short retval;
	
	retval = 65535*pow(((float)input)/65535,gamma)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (retval>=0x10000) retval=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	return retval;
}

/* Points to the next unread byte from png data block */
//extern unsigned char font_data[];
extern struct letter letter_data[];
extern struct font font_table[];
extern int n_fonts; /* Number of fonts. font number 0 is system_font (it's
		     * images are in system_font/ directory) and is used
		     * for special purpose.
		     */

/* Returns a pointer to a structure describing the letter found or NULL
 * if the letter is not found. Tries all possibilities in the style table
 * before returning NULL.
 */
struct letter *find_stored_letter(int *style_table, int letter_number)
{
	int first, last, half, diff, font_index, font_number;

	for (font_index=n_fonts-1;font_index;font_index--)
	{
		font_number=*style_table++;

		first=font_table[font_number].begin;
		last=font_table[font_number].length+first-1;

		while(first<=last){
			half=(first+last)>>1;
			diff=letter_data[half].code-letter_number;
			if (diff>=0){
				if (diff==0){
					return letter_data+half;
				}else{
					/* Value in table is bigger */
					last=half-1;
				}
			}else{
				/* Value in the table is smaller */
				first=half+1;
			}
		}
	}
	
	/* 0 is system font, 0 is blotch char. This must be present
	 * or we segfault :) */
#ifdef REPORT_UNKNOWN
	fprintf(stderr,"letter 0x%04x not found\n",letter_number);
#endif /* #ifdef REPORT_UNKNOWN */
	return letter_data+font_table[0].begin;
}

void read_stored_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	struct read_work *work;

	work=png_get_io_ptr(png_ptr);
	if (length>work->length) png_error(png_ptr,"Ran out of input data");
	memcpy(data,work->pointer,length);
	work->length-=length;
	work->pointer+=length;
}

void my_png_warning(png_structp a, png_const_charp b)
{
}

void my_png_error(png_structp a, png_const_charp error_string)
{
	error((unsigned char *)"Error when loading compiled-in font: %s.\n",error_string);
}

/* Loads width and height of the PNG (nothing is scaled). Style table is
 * already incremented.
 */
inline static void load_metric(int *x, int *y, int char_number, int *style_table)
{
	struct letter *l;

	l=find_stored_letter(style_table,char_number);
	if (!l){
		*x=0;
		*y=0;
	}else{
		*x=l->xsize;
		*y=l->ysize;
	}
	return;
}

/* The data tha fall out of this function express this: 0 is paper. 255 is ink. 34
 * is 34/255ink+(255-34)paper. No gamma is involved in this formula, as you can see.
 * The multiplications and additions take place in photon space.
 */
static void load_char(unsigned char **dest, int *x, int *y, 
unsigned char *png_data, int png_length, struct style *style)
{
	png_structp png_ptr;
	png_infop info_ptr;
	double gamma;
	int y1,number_of_passes;
	unsigned char **ptrs;
	struct read_work work;

	work.pointer = png_data;
	work.length = png_length;
	
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, my_png_error, my_png_warning);
	info_ptr=png_create_info_struct(png_ptr);
	png_set_read_fn(png_ptr,&work,(png_rw_ptr)&read_stored_data);
	png_read_info(png_ptr, info_ptr);
	*x=png_get_image_width(png_ptr,info_ptr);
	*y=png_get_image_height(png_ptr,info_ptr);
	if (png_get_gAMA(png_ptr,info_ptr, &gamma))
		png_set_gamma(png_ptr, 1.0, gamma);
	else
		png_set_gamma(png_ptr, 1.0, sRGB_gamma);
	{
		int bit_depth;
		int color_type;

		color_type=png_get_color_type(png_ptr, info_ptr);
		bit_depth=png_get_bit_depth(png_ptr, info_ptr);
		if (color_type==PNG_COLOR_TYPE_GRAY){
			if (bit_depth<8){
				 png_set_expand(png_ptr);
			}
			if (bit_depth==16){
				 png_set_strip_16(png_ptr);
			}
		}
		if (color_type==PNG_COLOR_TYPE_PALETTE){
			png_set_expand(png_ptr);
#ifdef HAVE_PNG_SET_RGB_TO_GRAY
			png_set_rgb_to_gray(png_ptr,1,54.0/256,183.0/256);
#else
			goto end;
#endif
		}
		if (color_type & PNG_COLOR_MASK_ALPHA){
			png_set_strip_alpha(png_ptr);
		}
		if (color_type==PNG_COLOR_TYPE_RGB ||
			color_type==PNG_COLOR_TYPE_RGB_ALPHA){
#ifdef HAVE_PNG_SET_RGB_TO_GRAY
			png_set_rgb_to_gray(png_ptr, 1, 54.0/256, 183.0/256);
#else
			goto end;
#endif
		}
		
	}
	/* If the depth is different from 8 bits/gray, make the libpng expand
	 * it to 8 bit gray.
	 */
	number_of_passes=png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr,info_ptr);
	*dest=mem_alloc(*x*(*y));
	ptrs=mem_alloc(*y*sizeof(*ptrs));
	for (y1=0;y1<*y;y1++) ptrs[y1]=*dest+*x*y1;
	for (;number_of_passes;number_of_passes--){
		png_read_rows(png_ptr, ptrs, NULL, *y);
	}
	png_read_end(png_ptr, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	mem_free(ptrs);
	return;
#ifndef HAVE_PNG_SET_RGB_TO_GRAY
	end:
	*dest=mem_calloc(*x*(*y));
	return;
#endif
}

/* Like load_char, but we dictate the y.
 */
static void load_scaled_char(void **dest, int *x, int y,
unsigned char *png_data, int png_length, struct style *style)
{
	unsigned char *interm;
	unsigned char *interm2;
	unsigned char *i2ptr,*dptr;
	int ix, iy, y0, x0, c;
	float conv0, conv1,sharpness,contrast;

	load_char(&interm, &ix,&iy,png_data, png_length,style);
	if (style->mono_space>=0)
		*x=compute_width(style->mono_space, style->mono_height, y);
	else
		*x=compute_width(ix,iy,y);
	if (display_optimize) *x*=3;
	scale_gray(interm, ix,iy, (unsigned char **)dest, *x, y);
	if (y>32||y<=0) return ; /* No convolution */
	ix=*x+2; /* There is one-pixel border around */
	iy=y+2;
	interm2=mem_alloc(ix*iy);
	i2ptr=interm2+ix+1;
	dptr=*dest;
	memset(interm2,0,ix);
	memset(interm2+(iy-1)*ix,0,ix);
	for (y0=y;y0;y0--){
		i2ptr[-1]=0;
		memcpy(i2ptr,dptr,*x);
		i2ptr[ix-1]=0;
		i2ptr+=ix;
		dptr+=*x;
	}
	i2ptr=interm2+ix+1;
	dptr=*dest;

	/* Determine the sharpness and contrast */
	sharpness=fancy_constants[2*y-2];
	contrast=fancy_constants[2*y-1];

	/* Compute the matrix constants from contrast and sharpness */
	conv0=(1+sharpness)*contrast;
	conv1=-sharpness*0.25*contrast;
	
	for (y0=y;y0;y0--){
		for (x0=*x;x0;x0--){
			/* Convolution */
			c=((*i2ptr)*conv0)+i2ptr[-ix]*conv1+i2ptr[-1]*conv1+i2ptr[1]*conv1+i2ptr[ix]*conv1+0.5;
			if (((unsigned)c)>=256) c=c<0?0:255;
			*dptr=c;
			dptr++;
			i2ptr++;
		}
		i2ptr+=2;
	}
	mem_free(interm2);
}

/* Adds required entry into font_cache and returns pointer to the entry.
 * We assume the entry is FC_COLOR.
 */
struct font_cache_entry *supply_color_cache_entry (struct graphics_driver
*gd, struct style *style, struct letter *letter)
{
	struct font_cache_entry *found, *new;
	unsigned short *primary_data;
	int do_free=0;
	struct font_cache_entry template;
	unsigned short red, green, blue;
	unsigned bytes_consumed;

	template.bitmap.y=style->height;
	template.type=FC_BW;
	/* The BW entries will be added only for background images which
	 * are not yet implemented, but will be in the future.
	 */

	found=lru_lookup(&font_cache, &template, letter->bw_list);
	if (!found){
		found=mem_alloc(sizeof(*found));
		found->bitmap.y=style->height;
//		load_scaled_char(&(found->bitmap.data),&(found->bitmap.x),
//			found->bitmap.y, font_data+letter->begin,
//			letter->length, style);
//		do_free=1;
	}
	new=mem_alloc(sizeof(*new));
	new->type=FC_COLOR;
	new->bitmap=found->bitmap;
	new->r0=style->r0;
	new->g0=style->g0;
	new->b0=style->b0;
	new->r1=style->r1;
	new->g1=style->g1;
	new->b1=style->b1;
	new->mono_space=style->mono_space;
	new->mono_height=style->mono_height;

	primary_data=mem_alloc(3
			*new->bitmap.x*new->bitmap.y*sizeof(*primary_data));

	/* We assume the gamma of HTML styles is in sRGB space */
	round_color_sRGB_to_48(&red, &green, &blue,
		((style->r0)<<16)|((style->g0)<<8)|(style->b0));
	mix_two_colors(primary_data, found->bitmap.data,
		found->bitmap.x*found->bitmap.y,
		red,green,blue,
		apply_gamma_single_8_to_16(style->r1,user_gamma/sRGB_gamma),
		apply_gamma_single_8_to_16(style->g1,user_gamma/sRGB_gamma),
		apply_gamma_single_8_to_16(style->b1,user_gamma/sRGB_gamma)
	);
	if (display_optimize){
		/* A correction for LCD */
		new->bitmap.x/=3;
		decimate_3(&primary_data,new->bitmap.x,new->bitmap.y);
	}
	/* We have a buffer with photons */
	gd->get_empty_bitmap(&(new->bitmap));
	if (dither_letters)
		dither(primary_data, &(new->bitmap));
	else
		(*round_fn)(primary_data,&(new->bitmap));
	mem_free(primary_data);
	gd->register_bitmap(&(new->bitmap));
	if (do_free){
		mem_free(found->bitmap.data);
		mem_free(found);
	}
	bytes_consumed=new->bitmap.x*new->bitmap.y*(gd->depth&7);
	/* Number of bytes per pixel in passed bitmaps */
	bytes_consumed+=sizeof(*new);
	bytes_consumed+=sizeof(struct lru_entry);
	lru_insert(&font_cache, new, &(letter->color_list),
		bytes_consumed);
	return new;
}

/* Prunes the cache to comply with maximum size */
static inline void prune_font_cache(struct graphics_driver *gd)
{
	struct font_cache_entry *bottom;

	while(font_cache.bytes>font_cache.max_bytes){
		/* Prune bottom entry of font cache */
		bottom=lru_get_bottom(&font_cache);
		if (!bottom){
#ifdef DEBUG
			if (font_cache.bytes||font_cache.items){
				internal((unsigned char *)"font cache is empty and contains\
some items or bytes at the same time.\n");
			}
#endif
			break;
		}
		if (bottom->type==FC_COLOR){
			gd->unregister_bitmap(&(bottom->bitmap));
		}else{
			mem_free(bottom->bitmap.data);
		}
		mem_free(bottom);
		lru_destroy_bottom(&font_cache);
	}
}

/* Prints a letter to the specified position and
 * returns the width of the printed letter */
inline static int print_letter(struct graphics_driver *gd, struct
	graphics_device *device, int x, int y, struct style *style,
	int char_number)

{	
	struct font_cache_entry *found;
	struct font_cache_entry template;
	struct letter *letter;

	/* Find a suitable letter */
	letter=find_stored_letter(style->table,char_number);
#ifdef DEBUG
	if (!letter) internal((unsigned char *)"print_letter could not find a letter - even not the blotch!");
#endif /* #ifdef DEBUG */
	template.type=FC_COLOR;
	template.r0=style->r0;
	template.r1=style->r1;
	template.g0=style->g0;
	template.g1=style->g1;
	template.b0=style->b0;
	template.b1=style->b1;
	template.bitmap.y=style->height;
	template.mono_space=style->mono_space;
	template.mono_height=style->mono_height;

	found=lru_lookup(&font_cache, &template, letter->color_list);
	if (!found) found=supply_color_cache_entry(gd, style, letter);
	gd->draw_bitmap(device, &(found->bitmap), x, y);
	prune_font_cache(gd);
	return found->bitmap.x;
}

/* Must return values that are:
 * >=0
 * <=height
 * at least 1 apart
 * Otherwise g_print_text will print nonsense (but won't segfault)
 */
void get_underline_pos(int height, int *top, int *bottom)
{
	int thickness, baseline;
	thickness=(height+15)/16;
	baseline=height/7;
	if (baseline<=0) baseline=1;
	if (thickness>baseline) thickness=baseline;
	*top=height-baseline;
	*bottom=*top+thickness;
}

/* *width will be advanced by the width of the text */
void g_print_text(struct graphics_driver *gd, struct graphics_device *device,
int x, int y, struct style *style, unsigned char *text, int *width)
{
	int original_flags, top_underline, bottom_underline, original_width,
		my_width;
	struct rect saved_clip;
	int p;

	if (y+style->height<=device->clip.y1||y>=device->clip.y2) goto o;
	if (style -> flags){
		/* Underline */
		if (!width){
		       width=&my_width;
		       *width=0;
		}
		original_flags=style->flags;
		original_width=*width;
		style -> flags=0;
		get_underline_pos(style->height, &top_underline, &bottom_underline);
		restrict_clip_area(device, &saved_clip, 0, 0, device->size.x2, y+
			top_underline);
		g_print_text(gd, device, x, y, style, text, width);
		gd->set_clip_area(device, &saved_clip);
		if (bottom_underline-top_underline==1){
			/* Line */
			drv->draw_hline(device, x, y+top_underline
				, x+*width-original_width
				, style->underline_color);
		}else{
			/* Area */
			drv->fill_area(device, x, y+top_underline,
					x+*width-original_width
					,y+bottom_underline,
					style->underline_color);
		}
		if (bottom_underline<style->height){
			/* Do the bottom half only if the underline is above
			 * the bottom of the letters.
			 */
			*width=original_width;
			restrict_clip_area(device, &saved_clip, 0,
				y+bottom_underline, device->size.x2,
				device->size.y2);
			g_print_text(gd, device, x, y, style, text, width);
			gd->set_clip_area(device, &saved_clip);
		}
		style -> flags=original_flags;
		return;
	}
#ifdef PLAN9
 	p = plan9_print_text(gd,device,x,y,style, text);
	if (width) 
 		*width += p;
#else
	while (*text){
		int u;
		GET_UTF_8(text, u);
		if (!u || u == 0xad) continue;
		if (u == 0x01 || u == 0xa0) u = ' ';
		p=print_letter(gd,device,x,y,style, u);
		x += p;
		if (width) {
			*width += p;
			continue;
		}
		if (x>=device->clip.x2) return;
	}
#endif
	return;
	o:
	if (width) *width += g_text_width(style, text);
}

/* 0=equality 1=inequality */
int compare_font_entries(void *entry, void *template)
{
	struct font_cache_entry*e1=entry;
	struct font_cache_entry*e2=template;

	if (e1->type==FC_COLOR){
		return(
		 (e1->r0!=e2->r0)||
		 (e1->g0!=e2->g0)||
		 (e1->b0!=e2->b0)||
		 (e1->r1!=e2->r1)||
		 (e1->g1!=e2->g1)||
		 (e1->b1!=e2->b1)||
		 (e1->bitmap.y!=e2->bitmap.y)||
		 (e1->mono_space!=e2->mono_space)||
		 (e1->mono_space>=0&&e1->mono_height!=e2->mono_height));
	}else{
		return e1->bitmap.y!=e2->bitmap.y;
	}
	
}

/* If the cache already exists, it is destroyed and reallocated. If you call it with the same
 * size argument, only a cache flush will yield.
 */
void init_font_cache(int bytes)
{
	lru_init(&font_cache, &compare_font_entries, bytes);
}

/* Ensures there are no lru_entry objects allocated - destroys them.
 * Also destroys the bitmaps asociated with them. Does not destruct the
 font_cache per se.
 */
void destroy_font_cache(void)
{
	struct font_cache_entry *bottom;
	
	while((bottom=lru_get_bottom(&font_cache))){
		if (bottom->type==FC_COLOR){
			drv->unregister_bitmap(&(bottom->bitmap));
		}else{
			/* Then it must be FC_BW. */
			mem_free(bottom->bitmap.data);
		}
		mem_free(bottom);
		lru_destroy_bottom(&font_cache);
	}
}

/* Returns 0 in case the char is not found. */
static inline int g_get_width(struct style *style, int charcode)
{
	int x, y, width;

	if (style->mono_space>=0){
		x=style->mono_space;
		y=style->mono_height;
	}else load_metric(&x,&y,charcode,style->table);
	if (!(x&&y)) width=0;
	else width=compute_width(x,y,style->height);
	return width;
}

int g_text_width(struct style *style, unsigned char *text)
{
	int w = 0;
#ifdef PLAN9
	w = plan9_stringsize(style, text);
#else
	while (*text) {
		int u;
		GET_UTF_8(text, u);
		if (!u || u == 0xad) continue;
		if (u == 0x01 || u == 0xa0) u = ' ';
		w += g_get_width(style, u);
	}
#endif
	return w;
}

/* this is called with normal characters only */
int g_char_width(struct style *style, int charcode)
{
#ifdef PLAN9
	return plan9_charsize(style, (char *)charcode);
#else
	return g_get_width(style, charcode);
#endif
}

int g_wrap_text(struct wrap_struct *w)
{
	while (*w->text) {
		int u;
		int s;
		if (*w->text == ' ') w->last_wrap = w->text,
				     w->last_wrap_obj = w->obj;
		GET_UTF_8(w->text, u);
		if (!u) continue;
		if (u == 0x01 || u == 0xa0) u = ' ';
#ifdef PLAN9
		s = plan9_charsize(w->style, u);	
#else
		s = g_get_width(w->style, u);
#endif
		if (u == 0xad) s = 0;
		if ((w->pos += s) <= w->width) {
			c:
			if (u != 0xad || *w->text == ' ') continue;
#ifdef PLAN9
			s = plan9_charsize(w->style, '-');	
#else
			s = g_char_width(w->style, '-');
#endif
			if (w->pos + s <= w->width || (!w->last_wrap && !w->last_wrap_obj)) {
				w->last_wrap = w->text;
				w->last_wrap_obj = w->obj;
				continue;
			}
		}
		if (!w->last_wrap && !w->last_wrap_obj) goto c;
		return 0;
	}
	return 1;
}

void update_aspect(void)
{
	aspect=aspect_on?(aspect_native*bfu_aspect+0.5):65536UL;
}

void init_dip(void)
{
	update_aspect();
	/* Initializes to 2 MByte */
	init_font_cache(2000000);
}

void shutdown_dip(void)
{
	destroy_font_cache();
}

void recode_font_name(unsigned char **name)
{
	int dashes=0;
	unsigned char *p;

	if (!strcmp((char *)*name,"monospaced")) *name=(unsigned char *)"courier-medium-roman-serif-mono";
	if (!strcmp((char *)*name,"monospace")) *name=(unsigned char *)"courier-medium-roman-serif-mono";
	else if (!strcmp((char *)*name,"")) *name=(unsigned char *)"century_school-medium-roman-serif-vari";
	p=*name;
	while(*p){
		if (*p=='-')dashes++;
		p++;
	}
	if (dashes!=4) *name=(unsigned char *)"century_school-medium-roman-serif-vari";
}

/* Compares single=a multi=b-c-a as matching.
 * 0 matches
 * 1 doesn't match
 */
int compare_family(unsigned char *single, unsigned char *multi)
{
	unsigned char *p,*r;
	int single_length=strlen((char *)single);

	r=multi;
	while(1){
		p=r;
		while (*r&&*r!='-')r++;
		if ((r-p==single_length)&&!strncmp((char *)single,(char *)p,r-p)) return 0;
		if (!*r) return 1;
		r++;
	}
	return 1;
}

/* Input name must contain exactly 4 dashes, otherwise the
 * result is undefined (parsing into weight, slant, adstyl, spacing
 * will result deterministically random results).
 * Returns 1 if the font is monospaced or 0 if not.
 */
int fill_style_table(int * table, unsigned char *name)
{
	unsigned char *p;
	unsigned char *family, *weight, *slant, *adstyl, *spacing;
	int pass,result,f;
	int masks[6]={0x1f,0x1f,0xf,0x7,0x3,0x1};
	int xors[6]={0,0x10,0x8,0x4,0x2,0x1};
	/* Top bit of the values belongs to family, bottom to spacing */
	int monospaced;
	
	/* Parse the name */
	recode_font_name(&name);
	family=stracpy(name);
	p=family;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	weight=p;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	slant=p;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	adstyl=p;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	spacing=p;
	monospaced=!strcmp((char*)spacing,"mono");
	
	for (pass=0;pass<6;pass++){
		for (f=1;f<n_fonts;f++){
			/* Font 0 must not be int style_table */
			result=compare_family(family,font_table[f].family);
			result<<=1;
			result|=!!strcmp((char *)weight,(char *)font_table[f].weight);
			result<<=1;
			result|=!!strcmp((char*)slant,(char *)font_table[f].slant);
			result<<=1;
			result|=!!strcmp((char *)adstyl,(char *)font_table[f].adstyl);
			result<<=1;
			result|=!!strcmp((char *)spacing,(char *)font_table[f].spacing);
			result^=xors[pass];
			result&=masks[pass];
			if (!result) /* Fot complies */
				*table++=f;
		}
	}
	mem_free(family);
	return monospaced;
}

struct style *g_invert_style(struct style *old)
{
	int length;

	struct style *st;
	st = mem_alloc(sizeof(struct style));
	st->refcount=1;
	st->r0=old->r1;
	st->g0=old->g1;
	st->b0=old->b1;
	st->r1=old->r0;
	st->g1=old->g0;
	st->b1=old->b0;
	st->height=old->height;
	st->flags=old->flags;
	if (st->flags)
	{
		/* We have to get a foreground color for underlining */
		st->underline_color=dip_get_color_sRGB(
			(st->r1<<16)|(st->g1<<8)|(st->b1));
	}
	length=sizeof(*st->table)*(n_fonts-1);
	st->table=mem_alloc(length);
	memcpy(st->table,old->table,length);
	st->mono_space=old->mono_space;
	st->mono_height=old->mono_height;
	return st;
}

/* Never returns NULL. */
struct style *g_get_style(int fg, int bg, int size, unsigned char *font, int flags)
{
	struct style *st;

	st = mem_alloc(sizeof(struct style));
	/* strcpy(st->font, font); */
	st->refcount = 1;
	st->r0 = bg >> 16;
	st->g0 = (bg >> 8) & 255;
	st->b0 = bg & 255;
	st->r1 = fg >> 16;
	st->g1 = (fg >> 8) & 255;
	st->b1 = fg & 255;
	if (size<=0) size=1;
	st->height = size;
	st->flags=flags&FF_UNDERLINE;
	if (st->flags)
	{
		/* We have to get a foreground color for underlining */
		st->underline_color=dip_get_color_sRGB(fg);
	}
	st->table=mem_alloc(sizeof(*st->table)*(n_fonts-1));
	if(fill_style_table(st->table, font))
		load_metric(&(st->mono_space), &(st->mono_height),' ',st->table);
	else
		st->mono_space=-1;
	return st;
}

struct style *g_clone_style(struct style *st)
{
	st->refcount++;
	return st;
}

void g_free_style(struct style *st)
{
	if (--st->refcount) return;
	mem_free(st->table);
	mem_free(st);
}

long gamma_cache_color;
int gamma_cache_rgb = -2;

/* IEC 61966-2-1 
 * Input gamma: sRGB space (directly from HTML, i. e. unrounded)
 * Output: color index for graphics driver that is closest to the
 * given sRGB value.
 * We assume unsigned short holds at least 16 bits. */
long real_dip_get_color_sRGB(int rgb)
{
	unsigned short r,g,b;
	int new_rgb;

	round_color_sRGB_to_48(&r,&g,&b,rgb);
	r=apply_gamma_single_16_to_8(r,1/display_red_gamma);
	g=apply_gamma_single_16_to_8(g,1/display_green_gamma);
	b=apply_gamma_single_16_to_8(b,1/display_blue_gamma);
	new_rgb=b|(g<<8)|(r<<16);	
	gamma_cache_rgb = rgb;
	/* The get_color takes values with gamma of display_*_gamma */
	return gamma_cache_color = drv->get_color(new_rgb);
}

/* ATTENTION!!! allocates using malloc. Due to braindead Xlib, which
 * frees it using free and thus it is not possible to use mem_alloc. */
void get_links_icon(unsigned char **data, int *width, int* height, int depth)
{
	struct bitmap b;
	unsigned short *tmp1;
	double g=user_gamma/sRGB_gamma;

	b.x=48;
	b.y=48;
	*width=b.x;
	*height=b.y;
	b.skip=b.x*(depth&7);
	b.data=*data=malloc(b.skip*b.y);
	tmp1=mem_alloc(6*b.y*b.x);
        apply_gamma_exponent_24_to_48(tmp1,links_icon,b.x*b.y,g,g,g);
	dither(tmp1, &b);
	mem_free(tmp1);
}

#endif /* G */
