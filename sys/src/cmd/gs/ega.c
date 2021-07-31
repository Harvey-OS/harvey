/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* ega.c */
/* EGA graphics hacks */
#define interrupt			/* patch ANSI incompatibility */
#include <dos.h>
#include "math_.h"
#include "stdio_.h"
#include "gstypes.h"
#include "gsmatrix.h"			/* for gxdevice.h */
#include "gxdevice.h"

/* Stuff in gdevpcfb.c */
#define pcfb_device gs_vga_device
extern struct gx_device_s pcfb_device;
#define the_device (&pcfb_device)

/* Stubs for other GS stuff */
int
gx_device_adjust_resolution(gx_device *dev,
  int actual_width, int actual_height, int fit)
{	return 0;
}

float yDx = 35.0/48.0;			/* aspect ratio */
#define black (gx_color_index)0
#define green (gx_color_index)2
#define blue (gx_color_index)9
#define red (gx_color_index)12
#define yellow (gx_color_index)14
#define white (gx_color_index)15

main(int argc, char *argv[])
{	return real_main(argc, argv);
}
real_main(int argc, char *argv[])
{	/* Display all kinds of stuff */
	(*dev_proc(the_device, open_device))(the_device);
	if ( !kbhit() ) paint1();
	if ( !kbhit() ) paint2();
	if ( !kbhit() ) paint3();
	if ( !kbhit() ) paint4();
	if ( !kbhit() ) test2();
	while ( !kbhit() ) ;
	(*dev_proc(the_device, close_device))(the_device);
}

/* Stubs for gx procedures */
int gx_default_draw_line() { return -1; }
void gx_default_get_initial_matrix() { }
int gx_default_output_page() { return 0; }
int gx_default_sync_output() { return 0; }
int gx_default_tile_rectangle() { return -1; }
int gx_default_get_params() { return 0; }
int gx_default_put_params() { return 0; }

/* Display color samples */
paint1()
{	ushort r, g, b;
	ushort m = the_device->color_info.max_color;
	for ( r = 0; r <= m; r++ )
	 for ( g = 0; g <= m; g++ )
	  for ( b = 0; b <= m; b++ )
	{	int x = r * 30, y = (g*(m+1)+b) * 24;
		gx_color_index color =
		  (*dev_proc(the_device, map_rgb_color))(the_device,
			(ushort)(r * 0xffffL / m),
			(ushort)(g * 0xffffL / m),
			(ushort)(b * 0xffffL / m));
		static unsigned char map[16] = {
			0xaa, 0xaa, 0xaa, 0, 0x55, 0x55, 0x55, 0,
			0xaa, 0xaa, 0xaa, 0, 0x55, 0x55, 0x55, 0
		   };
		(*dev_proc(the_device, fill_rectangle))(the_device, x, y, 21, 8, color);
		(*dev_proc(the_device, copy_mono))(the_device, map, 0, 4, gx_no_bitmap_id, x, y + 8, 21, 4, white, color);
		(*dev_proc(the_device, copy_mono))(the_device, map, 0, 4, gx_no_bitmap_id, x, y + 12, 21, 4, black, color);
	}
	   {	int c;
		for ( c = 0; c <= 15; c++ )
		   {	(*dev_proc(the_device, fill_rectangle))(the_device, 320+c*20, 0, 14, 11, (gx_color_index)c);
		   }
	   }
}

/* Tile with different color combinations */
paint2()
{	static byte pattern[] = {
	  0xaa,0xcc, 0x55,0xcc, 0xaa,0x33, 0x55,0x33,
	  0xaa,0xcc, 0x55,0xcc, 0xaa,0x33, 0x55,0x33,
	  0xf0,0xcc, 0xf0,0x66, 0xf0,0x33, 0xf0,0x99,
	  0x0f,0xcc, 0x0f,0x66, 0x0f,0x33, 0x0f,0x99
	};
	static gx_color_index colors[5] = { 0, 2, 6, 12, 0xf };
	gx_bitmap tile;
	int i, j;
	tile.data = pattern;
	tile.raster = 2;
	tile.size.x = tile.rep_width = 16;
	tile.size.y = tile.rep_height = 16;
	tile.id = gx_no_bitmap_id;
	for ( i = 0; i < 5; i++ )
	 for ( j = 0; j < 5; j++ )
		(*dev_proc(the_device, tile_rectangle))(the_device, &tile, 212+i*44, 20+j*44, 32, 32, colors[i], colors[j], 0, 0);
}

/* Draw a turkey */
paint3()
{	static byte turkey[] = {
	  0x00,0x3b,0x00,0, 0x00,0x27,0x00,0, 0x00,0x24,0x80,0,
	  0x0e,0x49,0x40,0, 0x11,0x49,0x20,0, 0x14,0xb2,0x20,0,
	  0x3c,0xb6,0x50,0, 0x75,0xfe,0x88,0, 0x17,0xff,0x8c,0,
	  0x17,0x5f,0x14,0, 0x1c,0x07,0xe2,0, 0x38,0x03,0xc4,0,
	  0x70,0x31,0x82,0, 0xf8,0xed,0xfc,0, 0xb2,0xbb,0xc2,0,
	  0xbb,0x6f,0x84,0, 0x31,0xbf,0xc2,0, 0x18,0xea,0x3c,0,
	  0x0e,0x3e,0x00,0, 0x07,0xfc,0x00,0, 0x03,0xf8,0x00,0,
	  0x1e,0x18,0x00,0, 0x1f,0xf8,0x00,0
	};
	(*dev_proc(the_device, fill_rectangle))(the_device, 0, 250, 30, 30, yellow);
	(*dev_proc(the_device, copy_mono))(the_device, turkey, 0, 4, gx_no_bitmap_id, 0, 250, 24, 23, white, red);
	(*dev_proc(the_device, copy_color))(the_device, turkey, 0, 4, gx_no_bitmap_id, 0, 280, 6, 23);
}

/* Copy patterns to the screen */
paint4()
{	static unsigned char box[] =
		{ 0xff,0xff,0,0, 0xc0,0x0c,0,0, 0xc0,0x0c,0,0, 0xc0,0x0c,0,0, 0xff,0xff,0,0
		};
	int i, j;
	(*dev_proc(the_device, fill_rectangle))(the_device, 432 - 4, 20 - 4, 4 + 8 * 17 + 8, 4 + 8 * 17, green);
	for ( i = 0; i < 8; i++ )
	  for ( j = 0; j < 8; j++ )
		(*dev_proc(the_device, copy_mono))(the_device, box, i, 4,
			gx_no_bitmap_id, 432 + (j * 17) + i, 20 + (i * 17),
			j + 1, 5, black, red);
}

/* Plot a circle 2 different ways */
plot_arc(P9(floatp, floatp, floatp, floatp, floatp, floatp, int, gx_color_index, floatp));
test2()
{	float kx = 200.0;
	float ky = -kx * yDx;
	float ox = 320.0, oy = 175.0;
	float x0 = ox - kx, y0 = oy - ky;
	float x1 = ox + kx, y1 = oy + ky;
	/* Plot a real circle */
	float i;
	for ( i = 0; i <= 360; i += 5 )
	   {	float theta = i * M_PI / 180.0;
		int x = ox + kx * cos(theta);
		int y = oy + ky * sin(theta);
		ega_write_dot(the_device, x, y, red);
	   }
	/* Plot with 90 degree arcs */
	   {	static float mar[] = { 0.5, 0.45, 0.4 };
		static gx_color_index car[] = { white, green, yellow };
		int j;
		for ( j = 0; j < 3; j++ )
		   {	float magic = mar[j];
			gx_color_index color = car[j];
			plot_arc(x1, oy, x1, y1, ox, y1, 40, color, magic);
			plot_arc(ox, y1, x0, y1, x0, oy, 40, color, magic);
			plot_arc(x0, oy, x0, y0, ox, y0, 40, color, magic);
			plot_arc(ox, y0, x1, y0, x1, oy, 40, color, magic);
		   }
	   }
}

/* Approximate an arc */
plot_curve(P10(floatp, floatp, floatp, floatp, floatp, floatp, floatp, floatp, int, gx_color_index));
plot_arc(floatp x0, floatp y0, floatp xi, floatp yi, floatp x3, floatp y3,
  int nsteps, gx_color_index color, floatp magic)
{	float cmagic = 1 - magic;
	float xic = xi * cmagic, yic = yi * cmagic;
	plot_curve(x0, y0, x0 * magic + xic, y0 * magic + yic,
		x3 * magic + xic, y3 * magic + yic, x3, y3,
		nsteps, color);
}

/* Plot a Bezier curve */
plot_curve(floatp x0, floatp y0, floatp x1, floatp y1,
  floatp x2, floatp y2, floatp x3, floatp y3,
  int nsteps, gx_color_index color)
{	float dt = 1.0 / nsteps;
	float cx = 3.0 * (x1 - x0);
	float bx = 3.0 * (x2 - x1) - cx;
	float ax = x3 - (x0 + cx + bx);
	float cy = 3.0 * (y1 - y0);
	float by = 3.0 * (y2 - y1) - cy;
	float ay = y3 - (y0 + cy + by);
	float t = 0.0;
	while ( t <= 1.0 )
	   {	int x = ((ax * t + bx) * t + cx) * t + x0;
		int y = ((ay * t + by) * t + cy) * t + y0;
		ega_write_dot(the_device, x, y, color);
		t += dt;
	   }
}
