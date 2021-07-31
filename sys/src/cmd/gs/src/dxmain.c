/* Copyright (C) 2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/* $Id: dxmain.c,v 1.7 2001/10/20 08:22:05 ghostgum Exp $ */

/* dxmain.c */
/* 
 * Ghostscript frontend which provides a graphical window 
 * using Gtk+.  Load time linking to libgs.so 
 * Compile using
 *    gcc `gtk-config --cflags` -o gs dxmain.c -lgs `gtk-config --libs`
 *
 * The ghostscript library needs to be compiled with
 *  gcc -fPIC -g -c -Wall file.c
 *  gcc -shared -Wl,-soname,libgs.so.6 -o libgs.so.6.60 file.o -lc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#define __PROTOTYPES__
#include "errors.h"
#include "iapi.h"
#include "gdevdsp.h"

const char start_string[] = "systemdict /start get exec\n";

static void read_stdin_handler(gpointer data, gint fd, 
	GdkInputCondition condition);
static int gsdll_stdin(void *instance, char *buf, int len);
static int gsdll_stdout(void *instance, const char *str, int len);
static int gsdll_stdout(void *instance, const char *str, int len);
static int display_open(void *handle, void *device);
static int display_preclose(void *handle, void *device);
static int display_close(void *handle, void *device);
static int display_presize(void *handle, void *device, int width, int height, 
	int raster, unsigned int format);
static int display_size(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage);
static int display_sync(void *handle, void *device);
static int display_page(void *handle, void *device, int copies, int flush);
static int display_update(void *handle, void *device, int x, int y, 
	int w, int h);

enum SEPARATIONS {
    SEP_CYAN = 8,
    SEP_MAGENTA = 4,
    SEP_YELLOW = 2,
    SEP_BLACK = 1
};

/*********************************************************************/
/* stdio functions */

struct stdin_buf {
   char *buf;
   int len;	/* length of buffer */
   int count;	/* number of characters returned */
};

/* handler for reading non-blocking stdin */
static void 
read_stdin_handler(gpointer data, gint fd, GdkInputCondition condition)
{
    struct stdin_buf *input = (struct stdin_buf *)data;

    if (condition & GDK_INPUT_EXCEPTION) {
	g_print("input exception");
	input->count = 0;	/* EOF */
    }
    else if (condition & GDK_INPUT_READ) {
	/* read returns -1 for error, 0 for EOF and +ve for OK */
	input->count = read(fd, input->buf, input->len);
	if (input->count < 0)
	    input->count = 0;
    }
    else {
	g_print("input condition unknown");
	input->count = 0;	/* EOF */
    }
}

/* callback for reading stdin */
static int 
gsdll_stdin(void *instance, char *buf, int len)
{
    struct stdin_buf input;
    gint input_tag;

    input.len = len;
    input.buf = buf;
    input.count = -1;

    input_tag = gdk_input_add(fileno(stdin), 
	(GdkInputCondition)(GDK_INPUT_READ | GDK_INPUT_EXCEPTION),
	read_stdin_handler, &input);
    while (input.count < 0)
	gtk_main_iteration_do(TRUE);
    gdk_input_remove(input_tag);

    return input.count;
}

static int 
gsdll_stdout(void *instance, const char *str, int len)
{
    gtk_main_iteration_do(FALSE);
    fwrite(str, 1, len, stdout);
    fflush(stdout);
    return len;
}

static int 
gsdll_stderr(void *instance, const char *str, int len)
{
    gtk_main_iteration_do(FALSE);
    fwrite(str, 1, len, stderr);
    fflush(stderr);
    return len;
}

/*********************************************************************/
/* dll display device */

typedef struct IMAGE_S IMAGE;
struct IMAGE_S {
    void *handle;
    void *device;
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *cmyk_bar;
    GtkWidget *scroll;
    GtkWidget *darea;
    guchar *buf;
    gint width;
    gint height;
    gint rowstride;
    unsigned int format;
    GdkRgbCmap *cmap;
    int separation;	/* for displaying C or M or Y or K */
    guchar *rgbbuf;	/* used when we need to convert raster format */
    IMAGE *next;
};

IMAGE *first_image;
static IMAGE *image_find(void *handle, void *device);
static void window_destroy(GtkWidget *w, gpointer data);
static void window_create(IMAGE *img);
static gboolean window_draw(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);

static IMAGE *
image_find(void *handle, void *device)
{
    IMAGE *img;
    for (img = first_image; img!=0; img=img->next) {
	if ((img->handle == handle) && (img->device == device))
	    return img;
    }
    return NULL;
}



static gboolean
window_draw(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
    IMAGE *img = (IMAGE *)user_data;
    if (img && img->window && img->buf) {
        int color = img->format & DISPLAY_COLORS_MASK;
	int depth = img->format & DISPLAY_DEPTH_MASK;
	switch (color) {
	    case DISPLAY_COLORS_NATIVE:
		if (depth == DISPLAY_DEPTH_8)
		    gdk_draw_indexed_image(widget->window, 
			widget->style->fg_gc[GTK_STATE_NORMAL],
			0, 0, img->width, img->height,
			GDK_RGB_DITHER_MAX, img->buf, img->rowstride,
			img->cmap);
	  	else if ((depth == DISPLAY_DEPTH_16) && img->rgbbuf)
		    gdk_draw_rgb_image(widget->window, 
			widget->style->fg_gc[GTK_STATE_NORMAL],
			0, 0, img->width, img->height,
			GDK_RGB_DITHER_MAX, img->rgbbuf, img->width * 3);
		break;
	    case DISPLAY_COLORS_GRAY:
		if (depth == DISPLAY_DEPTH_8)
		    gdk_draw_gray_image(widget->window, 
			widget->style->fg_gc[GTK_STATE_NORMAL],
			0, 0, img->width, img->height,
			GDK_RGB_DITHER_MAX, img->buf, img->rowstride);
		break;
	    case DISPLAY_COLORS_RGB:
		if (depth == DISPLAY_DEPTH_8) {
		    if (img->rgbbuf)
			gdk_draw_rgb_image(widget->window, 
			    widget->style->fg_gc[GTK_STATE_NORMAL],
			    0, 0, img->width, img->height,
			    GDK_RGB_DITHER_MAX, img->rgbbuf, img->width * 3);
		    else
			gdk_draw_rgb_image(widget->window, 
			    widget->style->fg_gc[GTK_STATE_NORMAL],
			    0, 0, img->width, img->height,
			    GDK_RGB_DITHER_MAX, img->buf, img->rowstride);
		}
		break;
	    case DISPLAY_COLORS_CMYK:
		if ((depth == DISPLAY_DEPTH_8) && img->rgbbuf)
		    gdk_draw_rgb_image(widget->window, 
			widget->style->fg_gc[GTK_STATE_NORMAL],
			0, 0, img->width, img->height,
			GDK_RGB_DITHER_MAX, img->rgbbuf, img->width * 3);
		break;
	}
    }
    return TRUE;
}

static void window_destroy(GtkWidget *w, gpointer data)
{
    IMAGE *img = (IMAGE *)data;
    img->window = NULL;
    img->scroll = NULL;
    img->darea = NULL;
}

static void window_create(IMAGE *img)
{
    /* Create a gtk window */
    img->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(img->window), "gs");
    gtk_window_set_default_size(GTK_WINDOW(img->window), 500, 400);
    img->vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(img->window), img->vbox);
    gtk_widget_show(img->vbox);

    img->darea = gtk_drawing_area_new();
    gtk_widget_show(img->darea);
    img->scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(img->scroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(img->scroll),
	GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(img->scroll),
	img->darea);
    gtk_box_pack_start(GTK_BOX(img->vbox), img->scroll, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (img->darea), "expose-event",
                        GTK_SIGNAL_FUNC (window_draw), img);
    gtk_signal_connect(GTK_OBJECT (img->window), "destroy", 
			GTK_SIGNAL_FUNC (window_destroy), img);
    /* do not show img->window until we know the image size */
}

static void window_separation(IMAGE *img, int layer)
{
    img->separation ^= layer;
    display_sync(img->handle, img->device);
}

static void cmyk_cyan(GtkWidget *w, gpointer data)
{
    window_separation((IMAGE *)data, SEP_CYAN);
}

static void cmyk_magenta(GtkWidget *w, gpointer data)
{
    window_separation((IMAGE *)data, SEP_MAGENTA);
}

static void cmyk_yellow(GtkWidget *w, gpointer data)
{
    window_separation((IMAGE *)data, SEP_YELLOW);
}

static void cmyk_black(GtkWidget *w, gpointer data)
{
    window_separation((IMAGE *)data, SEP_BLACK);
}

static void
window_add_button(IMAGE *img, const char *label, GtkSignalFunc fn)
{
    GtkWidget *w;
    w = gtk_check_button_new_with_label(label);
    gtk_box_pack_start(GTK_BOX(img->cmyk_bar), w, TRUE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
    gtk_signal_connect(GTK_OBJECT(w), "clicked", fn, img);
    gtk_widget_show(w);
}

/* New device has been opened */
static int display_open(void *handle, void *device)
{

    IMAGE *img = (IMAGE *)malloc(sizeof(IMAGE));
    if (img == NULL)
	return -1;
    memset(img, 0, sizeof(IMAGE));

    if (first_image == NULL) {
	gdk_rgb_init();
	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
	gtk_widget_set_default_visual(gdk_rgb_get_visual());
    }

    /* add to list */
    if (first_image)
	img->next = first_image;
    first_image = img;

    /* remember device and handle */
    img->handle = handle;
    img->device = device;

    /* create window */
    window_create(img);

    gtk_main_iteration_do(FALSE);
    return 0;
}

static int display_preclose(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
	return -1;

    gtk_main_iteration_do(FALSE);

    img->buf = NULL;
    img->width = 0;
    img->height = 0;
    img->rowstride = 0;
    img->format = 0;

    gtk_widget_destroy(img->window);
    img->window = NULL;
    img->scroll = NULL;
    img->darea = NULL;
    if (img->cmap)
	gdk_rgb_cmap_free(img->cmap);
    img->cmap = NULL;
    if (img->rgbbuf)
	free(img->rgbbuf);
    img->rgbbuf = NULL;

    gtk_main_iteration_do(FALSE);

    return 0;
}

static int display_close(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
	return -1;

    /* remove from list */
    if (img == first_image) {
	first_image = img->next;
    }
    else {
	IMAGE *tmp;
	for (tmp = first_image; tmp!=0; tmp=tmp->next) {
	    if (img == tmp->next)
		tmp->next = img->next;
	}
    }

    return 0;
}

static int display_presize(void *handle, void *device, int width, int height, 
	int raster, unsigned int format)
{
    /* Assume everything is OK.
     * It would be better to return e_rangecheck if we can't 
     * support the format.
     */
    return 0;
}
   
static int display_size(void *handle, void *device, int width, int height, 
	int raster, unsigned int format, unsigned char *pimage)
{
    IMAGE *img = image_find(handle, device);
    int color;
    int depth;
    if (img == NULL)
	return -1;

    if (img->cmap)
	gdk_rgb_cmap_free(img->cmap);
    img->cmap = NULL;
    if (img->rgbbuf)
	free(img->rgbbuf);
    img->rgbbuf = NULL;

    img->width = width;
    img->height = height;
    img->rowstride = raster;
    img->buf = pimage;
    img->format = format;
    gtk_drawing_area_size(GTK_DRAWING_AREA (img->darea), width, height);

    color = img->format & DISPLAY_COLORS_MASK;
    depth = img->format & DISPLAY_DEPTH_MASK;
    switch (color) {
	case DISPLAY_COLORS_NATIVE:
	    if (depth == DISPLAY_DEPTH_8) {
		/* palette of 96 colors */
		guint32 color[96];
		int i;
		int one = 255 / 3;
		for (i=0; i<96; i++) {
		    /* 0->63 = 00RRGGBB, 64->95 = 010YYYYY */
		    if (i < 64) {
			color[i] = 
			    (((i & 0x30) >> 4) * one << 16) + 	/* r */
			    (((i & 0x0c) >> 2) * one << 8) + 	/* g */
			    (i & 0x03) * one;		        /* b */
		    }
		    else {
			int val = i & 0x1f;
			val = (val << 3) + (val >> 2);
			color[i] = (val << 16) + (val << 8) + val;
		    }
		}
		img->cmap = gdk_rgb_cmap_new(color, 96);
		break;
	    }
	    else if (depth == DISPLAY_DEPTH_16) {
		/* need to convert to 24RGB */
		img->rgbbuf = (guchar *)malloc(width * height * 3);
		if (img->rgbbuf == NULL)
		    return -1;
	    }
	    else
		return e_rangecheck;	/* not supported */
	case DISPLAY_COLORS_GRAY:
	    if (depth == DISPLAY_DEPTH_8)
		break;
	    else
		return -1;	/* not supported */
	case DISPLAY_COLORS_RGB:
	    if (depth == DISPLAY_DEPTH_8) {
		if (((img->format & DISPLAY_ALPHA_MASK) == DISPLAY_ALPHA_NONE)
		    && ((img->format & DISPLAY_ENDIAN_MASK) 
			== DISPLAY_BIGENDIAN))
		    break;
		else {
		    /* need to convert to 24RGB */
		    img->rgbbuf = (guchar *)malloc(width * height * 3);
		    if (img->rgbbuf == NULL)
			return -1;
		}
	    }
	    else
		return -1;	/* not supported */
	    break;
	case DISPLAY_COLORS_CMYK:
	    if (depth == DISPLAY_DEPTH_8) {
		/* need to convert to 24RGB */
		img->rgbbuf = (guchar *)malloc(width * height * 3);
		if (img->rgbbuf == NULL)
		    return -1;
	    }
	    else
		return -1;	/* not supported */
	    break;
    }


    if (color == DISPLAY_COLORS_CMYK) {
	if (!img->cmyk_bar) {
	    /* add bar to select separation */
	    img->cmyk_bar = gtk_hbox_new(TRUE, 0);
	    gtk_box_pack_start(GTK_BOX(img->vbox), img->cmyk_bar, 
		FALSE, FALSE, 0);
	    img->separation = 0xf;	/* all layers */
	    window_add_button(img, "Cyan", (GtkSignalFunc)cmyk_cyan);
	    window_add_button(img, "Magenta", (GtkSignalFunc)cmyk_magenta);
	    window_add_button(img, "Yellow", (GtkSignalFunc)cmyk_yellow);
	    window_add_button(img, "Black", (GtkSignalFunc)cmyk_black);
	}
	gtk_widget_show(img->cmyk_bar);
    }
    else {
	if (img->cmyk_bar)
	    gtk_widget_hide(img->cmyk_bar);
    }


    if (!(GTK_WIDGET_FLAGS(img->window) & GTK_VISIBLE))
	gtk_widget_show(img->window);

    gtk_main_iteration_do(FALSE);
    return 0;
}
   
static int display_sync(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    int color;
    int depth;
    int endian;
    int native555;
    int alpha;
    if (img == NULL)
	return -1;

    if (img->window == NULL)
	window_create(img);
    if (!(GTK_WIDGET_FLAGS(img->window) & GTK_VISIBLE))
	gtk_widget_show_all(img->window);

    color = img->format & DISPLAY_COLORS_MASK;
    depth = img->format & DISPLAY_DEPTH_MASK;
    endian = img->format & DISPLAY_ENDIAN_MASK;
    native555 = img->format & DISPLAY_555_MASK;
    alpha = img->format & DISPLAY_ALPHA_MASK;
		
    /* some formats need to be converted for use by GdkRgb */
    switch (color) {
	case DISPLAY_COLORS_NATIVE:
	    if (depth == DISPLAY_DEPTH_16) {
	      if (endian == DISPLAY_LITTLEENDIAN) {
		if (native555 == DISPLAY_NATIVE_555) {
		    /* BGR555 */
		    int x, y;
		    unsigned short w;
		    unsigned char value;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    w = s[0] + (s[1] << 8);
			    value = (w >> 10) & 0x1f;	/* red */
			    *d++ = (value << 3) + (value >> 2);
			    value = (w >> 5) & 0x1f;	/* green */
			    *d++ = (value << 3) + (value >> 2);
			    value = w & 0x1f;		/* blue */
			    *d++ = (value << 3) + (value >> 2);
			    s += 2;
			}
		    }
		}
		else {
		    /* BGR565 */
		    int x, y;
		    unsigned short w;
		    unsigned char value;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    w = s[0] + (s[1] << 8);
			    value = (w >> 11) & 0x1f;	/* red */
			    *d++ = (value << 3) + (value >> 2);
			    value = (w >> 5) & 0x3f;	/* green */
			    *d++ = (value << 2) + (value >> 4);
			    value = w & 0x1f;		/* blue */
			    *d++ = (value << 3) + (value >> 2);
			    s += 2;
			}
		    }
		}
	      }
	      else {
		if (native555 == DISPLAY_NATIVE_555) {
		    /* RGB555 */
		    int x, y;
		    unsigned short w;
		    unsigned char value;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    w = s[1] + (s[0] << 8);
			    value = (w >> 10) & 0x1f;	/* red */
			    *d++ = (value << 3) + (value >> 2);
			    value = (w >> 5) & 0x1f;	/* green */
			    *d++ = (value << 3) + (value >> 2);
			    value = w & 0x1f;		/* blue */
			    *d++ = (value << 3) + (value >> 2);
			    s += 2;
			}
		    }
		}
		else {
		    /* RGB565 */
		    int x, y;
		    unsigned short w;
		    unsigned char value;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    w = s[1] + (s[0] << 8);
			    value = (w >> 11) & 0x1f;	/* red */
			    *d++ = (value << 3) + (value >> 2);
			    value = (w >> 5) & 0x3f;	/* green */
			    *d++ = (value << 2) + (value >> 4);
			    value = w & 0x1f;		/* blue */
			    *d++ = (value << 3) + (value >> 2);
			    s += 2;
			}
		    }
		}
	      }
	    }
	    break;
	case DISPLAY_COLORS_RGB:
	    if ( (depth == DISPLAY_DEPTH_8) && 
		 ((alpha == DISPLAY_ALPHA_FIRST) || 
	          (alpha == DISPLAY_UNUSED_FIRST)) &&
		 (endian == DISPLAY_BIGENDIAN) ) {
		/* Mac format */
		int x, y;
		unsigned char *s, *d;
		for (y = 0; y<img->height; y++) {
		    s = img->buf + y * img->rowstride;
		    d = img->rgbbuf + y * img->width * 3;
		    for (x=0; x<img->width; x++) {
			s++;		/* x = filler */
			*d++ = *s++;	/* r */
			*d++ = *s++;	/* g */
			*d++ = *s++;	/* b */
		    }
		}
	    }
	    else if ( (depth == DISPLAY_DEPTH_8) &&
		      (endian == DISPLAY_LITTLEENDIAN) ) {
	        if ((alpha == DISPLAY_UNUSED_LAST) ||
	            (alpha == DISPLAY_ALPHA_LAST)) {
		    /* Windows format + alpha = BGRx */
		    int x, y;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    *d++ = s[2];	/* r */
			    *d++ = s[1];	/* g */
			    *d++ = s[0];	/* b */
			    s += 4;
			}
		    }
		}
	        else if ((alpha == DISPLAY_UNUSED_FIRST) ||
	            (alpha == DISPLAY_ALPHA_FIRST)) {
		    /* xBGR */
		    int x, y;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    *d++ = s[3];	/* r */
			    *d++ = s[2];	/* g */
			    *d++ = s[1];	/* b */
			    s += 4;
			}
		    }
		}
		else {
		    /* Windows BGR24 */
		    int x, y;
		    unsigned char *s, *d;
		    for (y = 0; y<img->height; y++) {
			s = img->buf + y * img->rowstride;
			d = img->rgbbuf + y * img->width * 3;
			for (x=0; x<img->width; x++) {
			    *d++ = s[2];	/* r */
			    *d++ = s[1];	/* g */
			    *d++ = s[0];	/* b */
			    s += 3;
			}
		    }
		}
	    }
	    break;
	case DISPLAY_COLORS_CMYK:
	    if (depth == DISPLAY_DEPTH_8) {
	    	/* Separations */
		int x, y;
		int cyan, magenta, yellow, black;
		unsigned char *s, *d;
		for (y = 0; y<img->height; y++) {
		    s = img->buf + y * img->rowstride;
		    d = img->rgbbuf + y * img->width * 3;
		    for (x=0; x<img->width; x++) {
			cyan = *s++;
			magenta = *s++;
			yellow = *s++;
			black = *s++;
			if (!(img->separation & SEP_CYAN))
			    cyan = 0;
			if (!(img->separation & SEP_MAGENTA))
			    magenta = 0;
			if (!(img->separation & SEP_YELLOW))
			    yellow = 0;
			if (!(img->separation & SEP_BLACK))
			    black = 0;
			*d++ = (255-cyan)    * (255-black) / 255; /* r */
			*d++ = (255-magenta) * (255-black) / 255; /* g */
			*d++ = (255-yellow)  * (255-black) / 255; /* b */
		    }
		}
	    }
	    break;
    }

    gtk_widget_draw(img->darea, NULL);
    gtk_main_iteration_do(FALSE);
    return 0;
}

static int display_page(void *handle, void *device, int copies, int flush)
{
    display_sync(handle, device);
    return 0;
}

static int display_update(void *handle, void *device, 
    int x, int y, int w, int h)
{
    /* not implemented - eventually this will be used for progressive update */
    return 0;
}

/* callback structure for "display" device */
display_callback display = { 
    sizeof(display_callback),
    DISPLAY_VERSION_MAJOR,
    DISPLAY_VERSION_MINOR,
    display_open,
    display_preclose,
    display_close,
    display_presize,
    display_size,
    display_sync,
    display_page,
    display_update,
    NULL,	/* memalloc */
    NULL	/* memfree */
};

/*********************************************************************/

int main(int argc, char *argv[])
{
    int exit_status;
    int code = 1;
    gs_main_instance *instance;
    int nargc;
    char **nargv;
    char dformat[64];
    int exit_code;
    gboolean use_gui;

    /* Gtk initialisation */
    gtk_set_locale();
    use_gui = gtk_init_check(&argc, &argv);

    /* insert display device parameters as first arguments */
    sprintf(dformat, "-dDisplayFormat=%d", 
 	    DISPLAY_COLORS_RGB | DISPLAY_ALPHA_NONE | DISPLAY_DEPTH_8 | 
	    DISPLAY_BIGENDIAN | DISPLAY_TOPFIRST);
    nargc = argc + 1;
    nargv = (char **)malloc((nargc + 1) * sizeof(char *));
    nargv[0] = argv[0];
    nargv[1] = dformat;
    memcpy(&nargv[2], &argv[1], argc * sizeof(char *));

    /* run Ghostscript */
    if ((code = gsapi_new_instance(&instance, NULL)) == 0) {
        gsapi_set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
	if (use_gui)
            gsapi_set_display_callback(instance, &display);
	code = gsapi_init_with_args(instance, nargc, nargv);

	if (code == 0)
	    code = gsapi_run_string(instance, start_string, 0, &exit_code);
        gsapi_exit(instance);
	if (code == e_Quit)
	    code = 0;	/* user executed 'quit' */

	gsapi_delete_instance(instance);
    }

    exit_status = 0;
    switch (code) {
	case 0:
	case e_Info:
	case e_Quit:
	    break;
	case e_Fatal:
	    exit_status = 1;
	    break;
	default:
	    exit_status = 255;
    }

    return exit_status;
}

