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

/*
 * gdevhpij.h
 */

#ifndef gdevhpij_INCLUDED
#define gdevhpij_INCLUDED

/*
 * The following defines come from hpijs models.h .
 */

typedef enum { UNSUPPORTED=-1, 
                    DJ400, 
                    DJ540, 
                    DJ600, 
                    DJ6xx, 
                    DJ6xxPhoto, 
                    DJ8xx, 
                    DJ9xx,
                    DJ9xxVIP,
                    DJ630,
                    AP2100,
                    HPIJS } PRINTER_TYPE;

/*
 * The following defines come from hpijs hpijs.h .
 */

#define SNAME "hpijs"
#define SRVPATH "hpijs"	/* server path */

#define DEVICE_NAME_MAX 16

/* Packet commands. */
typedef enum
{
   ACK = 1,
   KILL,        /* kill server, no ack */
   SET_SHMID,
   SET_MODEL,
   SET_PRINTMODE,
   SET_PAPER,
   SET_PIXELS_PER_ROW,
   SND_RASTER,
   SND_NULL_RASTER,
   NEW_PAGE,
   GET_MODEL,
   GET_PRINTMODE,
   GET_PRINTMODE_CNT,
   GET_PAPER,
   GET_RESOLUTION,
   GET_EFFECTIVE_RESOLUTION,
   GET_SRV_VERSION,
   SET_RESOLUTION,
   SET_DEVICE_NAME,
   GET_PIXELS_PER_ROW,
   GET_PRINTABLE_AREA,        /* in inches */
   GET_PHYSICAL_PAGE_SIZE,    /* in inches */
   GET_PRINTABLE_START        /* in inches */
} PK_CMD;

/* Individual packets. PK_CMD must be first in all packets. */
typedef struct
{
   PK_CMD cmd;
   int id;
} PK_SHMID;

typedef struct
{
   PK_CMD cmd;
   int width;
  int outwidth;
} PK_PIXELS_PER_ROW;

typedef struct
{
  PK_CMD cmd;
  int val;
} PK_MODEL;

typedef PK_MODEL PK_MODE;

typedef struct
{
  PK_CMD cmd;
  int size;
} PK_PAPER;

typedef struct
{
  PK_CMD cmd;
  int x;
  int y;
} PK_RESOLUTION;

typedef struct
{
  PK_CMD cmd;
  char name[DEVICE_NAME_MAX];
} PK_DEVICE_NAME;

typedef struct
{
  PK_CMD cmd;
  char str[DEVICE_NAME_MAX];
} PK_VERSION;

typedef struct
{
   PK_CMD cmd;
   float width;
   float height;
} PK_PRINTABLE_AREA;

typedef struct
{
   PK_CMD cmd;
   float x;
   float y;
} PK_PHYSICAL_PAGE_SIZE;

typedef PK_PHYSICAL_PAGE_SIZE PK_PRINTABLE_START;

/* Common packet. */
typedef union
{
  PK_CMD cmd;
  PK_SHMID shm;
  PK_MODEL model;
  PK_MODE mode;
  PK_PAPER paper;
  PK_RESOLUTION res;
  PK_PIXELS_PER_ROW ppr;
  PK_DEVICE_NAME dev;
  PK_VERSION ver;
  PK_PRINTABLE_AREA parea;
  PK_PHYSICAL_PAGE_SIZE psize;
  PK_PRINTABLE_START pstart;
} PK;

/* Server descriptor. */
typedef struct
{
   int fds2c;   /* server to client */
   int fdc2s;   /* client to server */
   int shmid;
   char *shmbuf;
   char *srv_to_client;
   char *client_to_srv;
} SRVD;


#endif        /* gdevhpij_INCLUDED */


