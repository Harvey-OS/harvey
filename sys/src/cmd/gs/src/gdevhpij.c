/*
 * Hewlett-Packard Co. Inkjet Driver
 * Copyright (c) 2001, Hewlett-Packard Co.
 *
 * This ghostscript printer driver is the glue code for using the 
 * HP Inkjet Server (hpijs).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gdevprn.h"
#include "gdevpcl.h"
#include "gxdevice.h"
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "gdevhpij.h"


/* Structure for hpijs device. */
typedef struct gx_device_hpijs_s
{
   gx_device_common;
   gx_prn_device_common;
   /* Additional parameters for hpijs */
   int PrintMode;		/* qrayscale=0, normal=1, photo=2, ??? */
   SRVD sd;			/* server descriptor */
   gs_param_string DeviceName;
} gx_device_hpijs;

/* Default X and Y resolution. */
#ifndef X_DPI
#  define X_DPI 300
#endif
#ifndef Y_DPI
#  define Y_DPI 300
#endif

/* Margins are left, bottom, right, top. */
#define DESKJET_MARGINS_LETTER   0.25, 0.44, -0.25, 0.167
#define DESKJET_MARGINS_A4       0.125, 0.44, -0.125, 0.167
//#define DESKJET_MARGINS_LETTER   0.25, 0.44, 0, 0.167
//#define DESKJET_MARGINS_A4       0.125, 0.44, 0, 0.167

private int hpijs_print_page(gx_device_printer * pdev, FILE * prn_stream, const char *dname);

private dev_proc_open_device(hpijs_open);
private dev_proc_close_device(hpijs_close);
private dev_proc_print_page(dj630_print_page);
private dev_proc_print_page(dj6xx_print_page);
private dev_proc_print_page(dj6xx_photo_print_page);
private dev_proc_print_page(dj8xx_print_page);
private dev_proc_print_page(dj9xx_print_page);
private dev_proc_print_page(dj9xx_vip_print_page);
private dev_proc_print_page(ap21xx_print_page);
private dev_proc_print_page(hpijs_ext_print_page);
private dev_proc_get_params(hpijs_get_params);
private dev_proc_put_params(hpijs_put_params);

private gx_device_procs prn_hpijs_procs =
prn_color_params_procs(hpijs_open, gdev_prn_output_page, hpijs_close,
		       gx_default_rgb_map_rgb_color,
		       gx_default_rgb_map_color_rgb, hpijs_get_params,
		       hpijs_put_params);

gx_device_hpijs gs_DJ630_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "DJ630",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, dj630_print_page),
   2				/* PrintMode default */
};

gx_device_hpijs gs_DJ6xx_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "DJ6xx",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, dj6xx_print_page),
   1				/* PrintMode default */
};

gx_device_hpijs gs_DJ6xxP_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "DJ6xxP",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, dj6xx_photo_print_page),
   1				/* PrintMode default */
};

gx_device_hpijs gs_DJ8xx_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "DJ8xx",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, dj8xx_print_page),
   1				/* PrintMode default */
};

gx_device_hpijs gs_DJ9xx_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "DJ9xx",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, dj9xx_print_page),
   1				/* PrintMode default */
};

gx_device_hpijs gs_DJ9xxVIP_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "DJ9xxVIP",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, dj9xx_vip_print_page),
   1				/* PrintMode default */
};

gx_device_hpijs gs_AP21xx_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "AP21xx",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, ap21xx_print_page),
   2				/* PrintMode default */
};

gx_device_hpijs gs_hpijs_device =
    { prn_device_std_body(gx_device_hpijs, prn_hpijs_procs, "hpijs",
			  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			  X_DPI, Y_DPI,
			  0, 0, 0, 0,	/* margins (lm,bm,rm,tm)  */
			  24, hpijs_ext_print_page),
   1				/* PrintMode default */
};

static char s2c[80];
static char c2s[80];

private int bug(const char *fmt, ...)
{
   char buf[256];
   va_list args;
   int n;

   va_start(args, fmt);

   if ((n = vsnprintf(buf, 256, fmt, args)) == -1)
      buf[255] = 0;		/* output was truncated */

   fprintf(stderr, buf);
   syslog(LOG_WARNING, buf);

   fflush(stderr);
   va_end(args);
   return n;
}

/* Get packet from the server. */
private int hpijs_get_pk(PK * pk, int fd)
{
   return read(fd, pk, PIPE_BUF - 1);
}

/* Write packet to the server. */
private int hpijs_put_pk(PK * pk, int fd)
{
   return write(fd, pk, sizeof(PK));
}

/* Write single command packet to server. */
private int hpijs_put_cmd(SRVD * sd, int cmd)
{
   PK pk;

   pk.cmd = cmd;
   if (hpijs_put_pk(&pk, sd->fdc2s) < 0)
   {
      bug("unable to write fifo %d: %m\n", sd->client_to_srv);
      return -1;
   }
   if (hpijs_get_pk(&pk, sd->fds2c) < 0)	/* ack */
   {
      bug("unable to read fifo %d: %m\n", sd->client_to_srv);
      return -1;
   }

   return 0;
}

/* Init server discriptor. */
private int hpijs_init_sd(SRVD * sd)
{
   sd->fds2c = -1;
   sd->fdc2s = -1;
   sd->shmid = -1;
   sd->shmbuf = NULL;
   sd->srv_to_client = NULL;
   sd->client_to_srv = NULL;
   return 0;
}

/* Close the server. */
private int hpijs_close_srv(SRVD * sd)
{
   PK pk;

   if (sd->fdc2s >= 0)
   {
      /* Kill the server. */
      pk.cmd = KILL;
      hpijs_put_pk(&pk, sd->fdc2s);
   }
   if (sd->shmbuf != NULL)
   {
      /* Detach and release shared memory. */
      shmdt(sd->shmbuf);
      shmctl(sd->shmid, IPC_RMID, 0);
   }
   close(sd->fds2c);
   close(sd->fdc2s);

   remove(sd->srv_to_client);
   remove(sd->client_to_srv);
   return 0;
}

/* Spawn the server as a coprocess. */
private int hpijs_open_srv(SRVD * sd, FILE * prn_stream, int raster_width)
{
   PK pk;
   int fd;
   mode_t mode = 0666;
   int pid;

   /* Make sure we can find the server. */
   //   if (access(SRVPATH, F_OK) < 0)
   //   {
   //      bug("unable to locate %s: %m\n", SRVPATH);
   //      return -1;
   //   }    

   /* Assign fifo names for environment space, names must be const char. */
   sprintf(s2c, "SRV_TO_CLIENT=/tmp/hpijs_s2c_%d", getpid());
   sprintf(c2s, "CLIENT_TO_SRV=/tmp/hpijs_c2s_%d", getpid());

   /* Put fifo names in envirmoment space for child process. */
   putenv(s2c);
   putenv(c2s);

   /* Get local copy. */
   sd->srv_to_client = getenv("SRV_TO_CLIENT");
   sd->client_to_srv = getenv("CLIENT_TO_SRV");

   /* Create actual fifos in filesystem. */
   if ((mkfifo(sd->srv_to_client, mode)) < 0)
   {
      bug("unable to create fifo %s: %m\n", sd->srv_to_client);
      return -1;
   }
   if ((mkfifo(sd->client_to_srv, mode)) < 0)
   {
      bug("unable to create fifo %s: %m\n", sd->client_to_srv);
      goto BUGOUT;
   }

   /* Create shared memory for raster data. */
   if ((sd->shmid = shmget(IPC_PRIVATE, raster_width, mode)) < 0)
   {
      bug("unable to create shared memory: %m\n");
      goto BUGOUT;
   }

   /* Spawn the server. */
   if ((pid = fork()) < 0)
   {
      bug("unable to fork server: %m\n");
      goto BUGOUT;
   } else if (pid > 0)
   {				/* parent */

      /* Open fifos. */
      if ((sd->fds2c = open(sd->srv_to_client, O_RDONLY)) < 0)
      {
	 bug("unable to open fifo %d: %m\n", sd->srv_to_client);
	 goto BUGOUT;
      }
      if ((sd->fdc2s = open(sd->client_to_srv, O_WRONLY)) < 0)
      {
	 bug("unable to open fifo %d: %m\n", sd->client_to_srv);
	 goto BUGOUT;
      }

      /* Attach to shared memory. */
      if ((sd->shmbuf = shmat(sd->shmid, 0, 0)) < (char *) 0)
      {
	 bug("unable to attach shared memory: %m\n");
	 goto BUGOUT;
      }

      /* Send shmid to server. */
      pk.shm.cmd = SET_SHMID;
      pk.shm.id = sd->shmid;
      hpijs_put_pk(&pk, sd->fdc2s);
      hpijs_get_pk(&pk, sd->fds2c);

      return 0;

    BUGOUT:
      hpijs_close_srv(sd);
      return -1;

   } else
   {				/* child */
      /* Redirect print_stream to stdout. */
      fd = fileno(prn_stream);
      if (fd != STDOUT_FILENO)
      {
	 if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO)
	    bug("unable redirect stdout: %m\n");
      }
      if (execlp(SRVPATH, SNAME, (char *) 0) < 0)
      {
	 bug("unable to exec %s: %m\n", SRVPATH);
	 goto BUGOUT2;
      }
      exit(0);

    BUGOUT2:
      exit(EXIT_FAILURE);
   }

   return -1;
}

private int hpijs_spawn_srv(gx_device_hpijs *pdev, FILE * prn_stream, 
			    const char *dname)
{
   PK pk;

   hpijs_init_sd(&pdev->sd);
   if (hpijs_open_srv(&pdev->sd, prn_stream, pdev->width * 3) < 0)
   {
      bug("unable to spawn server\n");
      return -1;
   }

   pk.dev.cmd = SET_DEVICE_NAME;
   strcpy(pk.dev.name, dname);
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */

   pk.mode.cmd = SET_PRINTMODE;
   pk.mode.val = pdev->PrintMode;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */

   pk.paper.cmd = SET_PAPER;
   pk.paper.size = gdev_pcl_paper_size((gx_device *)pdev);
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */

   pk.res.cmd = SET_RESOLUTION;
   pk.res.x = pdev->x_pixels_per_inch;
   pk.res.y = pdev->y_pixels_per_inch;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */

   pk.ppr.cmd = SET_PIXELS_PER_ROW;
   pk.ppr.width = pdev->width;
   pk.ppr.outwidth = pdev->width;      /* no simple pixel replication (scaling) */
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */

#ifdef zero_and_no_warning
/* API Test code. */
   pk.ppr.cmd = GET_PRINTMODE;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("printmode=%d\n", pk.mode.val);

   pk.ppr.cmd = GET_PRINTMODE_CNT;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("number of printmodes=%d\n", pk.mode.val);

   pk.ppr.cmd = GET_PAPER;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("paper size=%d\n", pk.paper.size);

   pk.ppr.cmd = GET_EFFECTIVE_RESOLUTION;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("eff resolution x=%d, y=%d\n", pk.res.x, pk.res.y);

   pk.ppr.cmd = GET_PIXELS_PER_ROW;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("input width=%d, outwidth=%d\n", pk.ppr.width, pk.ppr.outwidth);

   pk.ppr.cmd = GET_SRV_VERSION;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("hpijs server %s\n", pk.ver.str);

   pk.ppr.cmd = GET_PRINTABLE_AREA;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("printable width=%.2f, height=%.2f (inches)\n", pk.parea.width, pk.parea.height);

   pk.ppr.cmd = GET_PHYSICAL_PAGE_SIZE;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("physical page size x=%.2f, y=%.2f (inches)\n", pk.psize.x, pk.psize.y);

   pk.ppr.cmd = GET_PRINTABLE_START;
   hpijs_put_pk(&pk, pdev->sd.fdc2s);
   hpijs_get_pk(&pk, pdev->sd.fds2c);	/* ack */
   bug("printable start x=%.2f, y=%.2f (inches)\n", pk.pstart.x, pk.pstart.y);
/* End API test code. */
#endif

   return 0;
}

private int hpijs_get_params(gx_device * pdev, gs_param_list * plist)
{
   int code = gdev_prn_get_params(pdev, plist);
   int ecode;

   if (code < 0)
      return code;

   if ((ecode = param_write_int(plist, "PrintMode",
			&((gx_device_hpijs *)pdev)->PrintMode)) < 0)
      code = ecode;

   if ((ecode = param_write_string(plist, "DeviceName", &((gx_device_hpijs *)pdev)->DeviceName)) < 0)
     code = ecode;

   return code;
}

private int hpijs_put_params(gx_device * pdev, gs_param_list * plist)
{
   int ecode = 0;
   int code;
   gs_param_name param_name;
   int pm = ((gx_device_hpijs *)pdev)->PrintMode;
   gs_param_string dname = ((gx_device_hpijs *)pdev)->DeviceName;

   switch (code = param_read_int(plist, (param_name = "PrintMode"), &pm))
   {
   case 0:
      if (pm < 0 || pm > 6)
	 ecode = gs_error_limitcheck;
      else
	 break;
      goto jqe;
   default:
      ecode = code;
    jqe:param_signal_error(plist, param_name, ecode);
   case 1:
      break;
   }

   if ((code = param_read_string(plist, (param_name = "DeviceName"), &dname)) < 0)
     ecode = code;

   if (ecode < 0)
      return ecode;
   code = gdev_prn_put_params(pdev, plist);
   if (code < 0)
      return code;

   ((gx_device_hpijs *)pdev)->PrintMode = pm;
   ((gx_device_hpijs *)pdev)->DeviceName = dname;
   return 0;
}

private int hpijs_open(gx_device * pdev)
{
   static const float dj_a4[4] = { DESKJET_MARGINS_A4 };
   static const float dj_letter[4] = { DESKJET_MARGINS_LETTER };
   const float *m = NULL;

   /* Make sure width does not exceed 8". */
   if (pdev->width > (pdev->x_pixels_per_inch * 8))
      gx_device_set_width_height(pdev, pdev->x_pixels_per_inch * 8,
				 pdev->height);

   m = (gdev_pcl_paper_size(pdev) == PAPER_SIZE_A4 ? dj_a4 : dj_letter);
   gx_device_set_margins(pdev, m, true);

   return gdev_prn_open(pdev);
}

private int hpijs_close(gx_device * pdev)
{
   hpijs_close_srv(&((gx_device_hpijs *) pdev)->sd);
   return gdev_prn_close(pdev);
}

private int white(unsigned char *data, int size)
{
   int clean = 1;
   int i;
   for (i = 0; i < size; i++)
      if (data[i] != 0xFF)
      {
	 clean = 0;
	 break;
      }
   return clean;
}

private int dj630_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "DJ630");
}

private int dj6xx_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "DJ6xx");
}

private int dj6xx_photo_print_page(gx_device_printer * pdev,
				   FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "DJ6xxPhoto");
}

private int dj8xx_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "DJ8xx");
}

private int dj9xx_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "DJ9xx");
}

private int dj9xx_vip_print_page(gx_device_printer * pdev,
				 FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "DJ9xxVIP");
}

private int ap21xx_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
   return hpijs_print_page(pdev, prn_stream, "AP21xx");
}

private int hpijs_ext_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
   char name[DEVICE_NAME_MAX];
   int len;

   len = ((gx_device_hpijs *)pdev)->DeviceName.size; 
   if ((len == 0) || (len >= DEVICE_NAME_MAX))
      return gs_note_error(gs_error_invalidaccess);
   
   strncpy(name, ((gx_device_hpijs *)pdev)->DeviceName.data, len);
   name[len]=0;
   return hpijs_print_page(pdev, prn_stream, name);
}

/* Send the page to the printer. */
private int hpijs_print_page(gx_device_printer * pdev, FILE * prn_stream, const char *dname)
{
   int line_size;
   int last_row;
   int lnum, code = 0;
   int isnew;

   isnew = gdev_prn_file_is_new(pdev);

   if (isnew)
   {
      if (hpijs_spawn_srv((gx_device_hpijs *)pdev, prn_stream, dname) != 0)
	 return gs_note_error(gs_error_invalidaccess);
   }

   line_size = gx_device_raster((gx_device *)pdev, 0);
   last_row = dev_print_scan_lines(pdev);

   for (lnum = 0; lnum < last_row; lnum++)
   {
      code = gdev_prn_copy_scan_lines(pdev, lnum,
				   ((gx_device_hpijs *) pdev)->sd.shmbuf,
				   line_size);
      if (code < 0)
	 return -1;
      if (!white(((gx_device_hpijs *)pdev)->sd.shmbuf, line_size))
      {
	if (hpijs_put_cmd(&((gx_device_hpijs *)pdev)->sd, SND_RASTER) == -1)
	  return -1;
      } else
      {
	  if (hpijs_put_cmd (&((gx_device_hpijs *)pdev)->sd, SND_NULL_RASTER) == -1)
	    return -1;
      }
   }

   hpijs_put_cmd(&((gx_device_hpijs *)pdev)->sd, NEW_PAGE);

   return code;
}
