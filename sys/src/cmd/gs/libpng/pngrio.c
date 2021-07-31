
/* pngrio.c - functions for data input
 *
 * libpng 0.99b
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, Glenn Randers-Pehrson
 * February 3, 1998
 *
 * This file provides a location for all input.  Users which need
 * special handling are expected to write a function which has the same
 * arguments as this, and perform a similar function, but possibly has
 * a different input method.  Note that you shouldn't change this
 * function, but rather write a replacement function and then make
 * libpng use it at run time with png_set_read_fn(...).
 */

#define PNG_INTERNAL
#include "png.h"

/* Read the data from whatever input you are using.  The default routine
   reads from a file pointer.  Note that this routine sometimes gets called
   with very small lengths, so you should implement some kind of simple
   buffering if you are using unbuffered reads.  This should never be asked
   to read more then 64K on a 16 bit machine. */
void
png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_debug1(4,"reading %d bytes\n", length);
   if (png_ptr->read_data_fn != NULL)
      (*(png_ptr->read_data_fn))(png_ptr, data, length);
   else
      png_error(png_ptr, "Call to NULL read function");
}

#if !defined(PNG_NO_STDIO)
/* This is the function which does the actual reading of data.  If you are
   not reading from a standard C stream, you should create a replacement
   read_data function and use it at run time with png_set_read_fn(), rather
   than changing the library. */
#ifndef USE_FAR_KEYWORD
static void
png_default_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_size_t check;

   /* fread() returns 0 on error, so it is OK to store this in a png_size_t
    * instead of an int, which is what fread() actually returns.
    */
   check = (png_size_t)fread(data, (png_size_t)1, length,
      (FILE *)png_ptr->io_ptr);

   if (check != length)
   {
      png_error(png_ptr, "Read Error");
   }
}
#else
/* this is the model-independent version. Since the standard I/O library
   can't handle far buffers in the medium and small models, we have to copy
   the data.
*/
 
#define NEAR_BUF_SIZE 1024
#define MIN(a,b) (a <= b ? a : b)
 
static void
png_default_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   int check;
   png_byte *n_data;
   FILE *io_ptr;

   /* Check if data really is near. If so, use usual code. */
   n_data = (png_byte *)CVT_PTR_NOCHECK(data);
   io_ptr = (FILE *)CVT_PTR(png_ptr->io_ptr);
   if ((png_bytep)n_data == data)
   {
      check = fread(n_data, 1, length, io_ptr);
   }
   else
   {
      png_byte buf[NEAR_BUF_SIZE];
      png_size_t read, remaining, err;
      check = 0;
      remaining = length;
      do
      {
         read = MIN(NEAR_BUF_SIZE, remaining);
         err = fread(buf, (png_size_t)1, read, io_ptr);
         png_memcpy(data, buf, read); /* copy far buffer to near buffer */
         if(err != read)
            break;
         else
            check += err;
         data += read;
         remaining -= read;
      }
      while (remaining != 0);
   }
   if ((png_uint_32)check != (png_uint_32)length)
   {
      png_error(png_ptr, "read Error");
   }
}
#endif
#endif

/* This function allows the application to supply a new input function
   for libpng if standard C streams aren't being used.

   This function takes as its arguments:
   png_ptr      - pointer to a png input data structure
   io_ptr       - pointer to user supplied structure containing info about
                  the input functions.  May be NULL.
   read_data_fn - pointer to a new input function which takes as it's
                  arguments a pointer to a png_struct, a pointer to
                  a location where input data can be stored, and a 32-bit
                  unsigned int which is the number of bytes to be read.
                  To exit and output any fatal error messages the new write
                  function should call png_error(png_ptr, "Error msg"). */
void
png_set_read_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr read_data_fn)
{
   png_ptr->io_ptr = io_ptr;

#if !defined(PNG_NO_STDIO)
   if (read_data_fn != NULL)
      png_ptr->read_data_fn = read_data_fn;
   else
      png_ptr->read_data_fn = png_default_read_data;
#else
   png_ptr->read_data_fn = read_data_fn;
#endif

   /* It is an error to write to a read device */
   png_ptr->write_data_fn = NULL;

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   png_ptr->output_flush_fn = NULL;
#endif /* PNG_WRITE_FLUSH_SUPPORTED */
}

