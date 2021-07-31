/* example.c - an example of using libpng */

/* This is an example of how to use libpng to read and write PNG files.
 * The file libpng.txt is much more verbose then this.  If you have not
 * read it, do so first.  This was designed to be a starting point of an
 * implementation.  This is not officially part of libpng, and therefore
 * does not require a copyright notice.
 *
 * This file does not currently compile, because it is missing certain
 * parts, like allocating memory to hold an image.  You will have to
 * supply these parts to get it to compile.  For an example of a minimal
 * working PNG reader/writer, see pngtest.c, included in this distribution.
 */

#include <png.h>

/* Check to see if a file is a PNG file using png_check_sig().  Returns
 * non-zero if the image is a PNG, and 0 if it isn't a PNG.
 *
 * If this call is successful, and you are going to keep the file open,
 * you should call png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK); once
 * you have created the png_ptr, so that libpng knows your application
 * has read that many bytes from the start of the file.  Make sure you
 * don't call png_set_sig_bytes() with more than 8 bytes read or give it
 * an incorrect number of bytes read, or you will either have read too
 * many bytes (your fault), or you are telling libpng to read the wrong
 * number of magic bytes (also your fault).
 *
 * Many applications already read the first 2 or 4 bytes from the start
 * of the image to determine the file type, so it would be easiest just
 * to pass the bytes to png_check_sig() or even skip that if you know
 * you have a PNG file, and call png_set_sig_bytes().
 */
#define PNG_BYTES_TO_CHECK 4
int check_if_png(char *file_name, FILE **fp)
{
   char buf[PNG_BYTES_TO_CHECK];

   /* Open the prospective PNG file. */
   if ((*fp = fopen(file_name, "rb")) != NULL);
      return 0;

   /* Read in the signature bytes */
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
      return 0;

   /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature. */
   return(png_check_sig(buf, PNG_BYTES_TO_CHECK));
}

/* Read a PNG file.  You may want to return an error code if the read
 * fails (depending upon the failure).  There are two "prototypes" given
 * here - one where we are given the filename, and we need to open the
 * file, and the other where we are given an open file (possibly with
 * some or all of the magic bytes read - see comments above).
 */
**** prototype 1 ****
void read_png(char *file_name)  /* We need to open the file */
{
   png_structp png_ptr;
   png_infop info_ptr;
   unsigned int sig_read = 0;
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;
   FILE *fp;

   if ((fp = fopen(file_name, "rb")) == NULL)
      return;
**** prototype 2 ****
void read_png(FILE *fp, unsigned int sig_read)  /* file is already open */
{
   png_structp png_ptr;
   png_infop info_ptr;
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;
**** only use one prototype! ****

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      (void *)user_error_ptr, user_error_fn, user_warning_fn);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return;
   }

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct();
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return;
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */
   if (setjmp(png_ptr->jmpbuf))
   {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      return;
   }

   /* One of the following I/O initialization methods is REQUIRED */
**** PNG file I/O method 1 ****
   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

**** PNG file I/O method 2 ****
   /* If you are using replacement read functions, instead of calling
    * png_init_io() here you would call:
    */
   png_set_read_fn(png_ptr, (void *)user_io_ptr, user_read_fn);
   /* where user_io_ptr is a structure you want available to the callbacks */
**** Use only one I/O method! ****

   /* If we have already read some of the signature */
   png_set_sig_bytes_read(png_ptr, sig_read);

   /* The call to png_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).  REQUIRED
    */
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);

/**** Set up the data transformations you want.  Note that these are all
 **** optional.  Only call them if you want/need them.  Many of the
 **** transformations only work on specific types of images, and many
 **** are mutually exclusive.
 ****/

   /* tell libpng to strip 16 bit/color files down to 8 bits/color */
   png_set_strip_16(png_ptr);

   /* Strip alpha bytes from the input data without combining with th
    * background (not recommended).
    */
   png_set_strip_alpha(png_ptr);

   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   png_set_packing(png_ptr);

   /* Change the order of packed pixels to least significant bit first
    * (not useful if you are using png_set_packing). */
   png_set_packswap(png_ptr);

   /* Expand paletted colors into true RGB triplets */
   if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_expand(png_ptr);

   /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand(png_ptr);

   /* Expand paletted or RGB images with transparency to full alpha channels
    * so the data will be available as RGBA quartets.
    */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_expand(png_ptr);

   /* Set the background color to draw transparent and alpha images over.
    * It is possible to set the red, green, and blue components directly
    * for paletted images instead of supplying a palette index.  Note that
    * even if the PNG file supplies a background, you are not required to
    * use it - you should use the (solid) application background if it has one.
    */

   png_color_16 my_background, *image_background);

   if (png_get_bKGD(png_ptr, info_ptr, &image_background);
      png_set_background(png_ptr, image_background),
                         PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
   else
      png_set_background(png_ptr, &my_background,
                         PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

   /* Some suggestions as to how to get a screen gamma value */

   /* Note that screen gamma is (display_gamma/viewing_gamma)
   if (/* We have a user-defined screen gamma value */)
   {
      screen_gamma = user-defined screen_gamma;
   }
   /* This is one way that applications share the same screen gamma value */
   else if ((gamma_str = getenv("SCREEN_GAMMA")) != NULL)
   {
      screen_gamma = atof(gamma_str);
   }
   /* If we don't have another value */
   else
   {
      screen_gamma = 2.2;  /* A good guess for a PC monitors in a dimly
                              lit room */
      screen_gamma = 1.7 or 1.0;  /* A good guess for Mac systems */
   }

   /* Tell libpng to handle the gamma conversion for you.  The second call
    * is a good guess for PC generated images, but it should be configurable
    * by the user at run time by the user.  It is strongly suggested that
    * your application support gamma correction.
    */

   int intent;

   if (png_get_sRGB(png_ptr, info_ptr, &intent)
      png_set_sRGB(png_ptr, intent, 0);
   else 
      if (png_get_gAMA(png_ptr, info_ptr, &image_gamma)
         png_set_gamma(png_ptr, screen_gamma, image_gamma);
      else
         png_set_gamma(png_ptr, screen_gamma, 0.50);

   /* Dither RGB files down to 8 bit palette or reduce palettes
    * to the number of colors available on your screen.
    */
   if (color_type & PNG_COLOR_MASK_COLOR)
   {
      png_uint_32 num_palette;
      png_colorp palette;

      /* This reduces the image to the application supplied palette */
      if (we have our own palette)
      {
         /* An array of colors to which the image should be dithered */
         png_color std_color_cube[MAX_SCREEN_COLORS];

         png_set_dither(png_ptr, std_color_cube, MAX_SCREEN_COLORS,
            MAX_SCREEN_COLORS, NULL, 0);
      }
      /* This reduces the image to the palette supplied in the file */
      else if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)))
      {
         png_color16p histogram;

         png_get_hIST(png_ptr, info_ptr, &histogram);

         png_set_dither(png_ptr, palette, num_palette,
                        max_screen_colors, histogram, 0);
      }
   }

   /* invert monocrome files to have 0 as white and 1 as black */
   png_set_invert_mono(png_ptr);

   /* If you want to shift the pixel values from the range [0,255] or
    * [0,65535] to the original [0,7] or [0,31], or whatever range the
    * colors were originally in:
    */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT))
   {
      png_color8p sig_bit;

      png_get_sBIT(png_ptr, info_ptr, &sig_bit);
      png_set_shift(png_ptr, sig_bit);
   }

   /* flip the RGB pixels to BGR (or RGBA to BGRA) */
   png_set_bgr(png_ptr);

   /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
   png_set_swap_alpha(png_ptr);

   /* swap bytes of 16 bit files to least significant byte first */
   png_set_swap(png_ptr);

   /* Add filler (or alpha) byte (before/after each RGB triplet) */
   png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

   /* Turn on interlace handling.  REQUIRED if you are not using
    * png_read_image().  To see how to handle interlacing passes,
    * see the png_read_row() method below:
    */
   number_passes = png_set_interlace_handling(png_ptr);

   /* Optional call to gamma correct and add the background to the palette
    * and update info structure.  REQUIRED if you are expecting libpng to
    * update the palette for you (ie you selected such a transform above).
    */
   png_read_update_info(png_ptr, info_ptr);

   /* Allocate the memory to hold the image using the fields of info_ptr. */

   /* The easiest way to read the image: */
   png_bytep row_pointers[height];

   for (row = 0; row < height; row++)
   {
      row_pointers[row] = malloc(png_get_rowbytes(png_ptr, info_ptr));
   }

   /* Now it's time to read the image.  One of these methods is REQUIRED */
**** Read the entire image in one go ****
   png_read_image(png_ptr, row_pointers);

**** Read the image one or more scanlines at a time ****
   /* The other way to read images - deal with interlacing: */

   for (pass = 0; pass < number_passes; pass++)
   {
[[[[[[[ Read the image a single row at a time ]]]]]]]
      for (y = 0; y < height; y++)
      {
         png_bytep row_pointers = row[y];
         png_read_rows(png_ptr, &row_pointers, NULL, 1);
      }

[[[[[[[ Read the image several rows at a time ]]]]]]]
      for (y = 0; y < height; y += number_of_rows)
      {
<<<<<<<<<< Read the image using the "sparkle" effect. >>>>>>>>>>
         png_read_rows(png_ptr, row_pointers, NULL, number_of_rows);
        
<<<<<<<<<< Read the image using the "rectangle" effect >>>>>>>>>>
         png_read_rows(png_ptr, NULL, row_pointers, number_of_rows);
<<<<<<<<<< use only one of these two methods >>>>>>>>>>
      }
     
      /* if you want to display the image after every pass, do
         so here */
[[[[[[[ use only one of these two methods ]]]]]]]
   }
**** use only one of these two methods ****

   /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
   png_read_end(png_ptr, info_ptr);

   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

   /* close the file */
   fclose(fp);

   /* that's it */
   return;
}

/* progressively read a file */

int
initialize_png_reader(png_structp *png_ptr, png_infop *info_ptr)
{
   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible in case we are using dynamically
    * linked libraries.
    */
   *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
       (void *)user_error_ptr, user_error_fn, user_warning_fn);

   if (*png_ptr == NULL)
   {
      *info_ptr = NULL;
      return ERROR;
   }

   *info_ptr = png_create_info_struct(png_ptr);

   if (*info_ptr == NULL)
   {
      png_destroy_read_struct(png_ptr, info_ptr, (png_infopp)NULL);
      return ERROR;
   }

   if (setjmp((*png_ptr)->jmpbuf))
   {
      png_destroy_read_struct(png_ptr, info_ptr, (png_infopp)NULL);
      return ERROR;
   }

   /* this one's new.  You will need to provide all three
    * function callbacks, even if you aren't using them all.
    * These functions shouldn't be dependent on global or
    * static variables if you are decoding several images
    * simultaneously.  You should store stream specific data
    * in a separate struct, given as the second parameter,
    * and retrieve the pointer from inside the callbacks using
    * the function png_get_progressive_ptr(png_ptr).
    */
   png_set_progressive_read_fn(*png_ptr, (void *)stream_data,
      info_callback, row_callback, end_callback);

   return OK;
}

int
process_data(png_structp *png_ptr, png_infop *info_ptr,
   png_bytep buffer, png_uint_32 length)
{
   if (setjmp((*png_ptr)->jmpbuf))
   {
      /* Free the png_ptr and info_ptr memory on error */
      png_destroy_read_struct(png_ptr, info_ptr, (png_infopp)NULL);
      return ERROR;
   }

   /* This one's new also.  Simply give it chunks of data as
    * they arrive from the data stream (in order, of course).
    * On Segmented machines, don't give it any more than 64K.
    * The library seems to run fine with sizes of 4K, although
    * you can give it much less if necessary (I assume you can
    * give it chunks of 1 byte, but I haven't tried with less
    * than 256 bytes yet).  When this function returns, you may
    * want to display any rows that were generated in the row
    * callback, if you aren't already displaying them there.
    */
   png_process_data(*png_ptr, *info_ptr, buffer, length);
   return OK;
}

info_callback(png_structp png_ptr, png_infop info)
{
/* do any setup here, including setting any of the transformations
 * mentioned in the Reading PNG files section.  For now, you _must_
 * call either png_start_read_image() or png_read_update_info()
 * after all the transformations are set (even if you don't set
 * any).  You may start getting rows before png_process_data()
 * returns, so this is your last chance to prepare for that.
 */
}

row_callback(png_structp png_ptr, png_bytep new_row,
   png_uint_32 row_num, int pass)
{
/* this function is called for every row in the image.  If the
 * image is interlacing, and you turned on the interlace handler,
 * this function will be called for every row in every pass.
 * Some of these rows will not be changed from the previous pass.
 * When the row is not changed, the new_row variable will be NULL.
 * The rows and passes are called in order, so you don't really
 * need the row_num and pass, but I'm supplying them because it
 * may make your life easier.
 *
 * For the non-NULL rows of interlaced images, you must call
 * png_progressive_combine_row() passing in the row and the
 * old row.  You can call this function for NULL rows (it will
 * just return) and for non-interlaced images (it just does the
 * memcpy for you) if it will make the code easier.  Thus, you
 * can just do this for all cases:
 */

   png_progressive_combine_row(png_ptr, old_row, new_row);

/* where old_row is what was displayed for previous rows.  Note
 * that the first pass (pass == 0 really) will completely cover
 * the old row, so the rows do not have to be initialized.  After
 * the first pass (and only for interlaced images), you will have
 * to pass the current row, and the function will combine the
 * old row and the new row.
 */
}

end_callback(png_structp png_ptr, png_infop info)
{
/* this function is called when the whole image has been read,
 * including any chunks after the image (up to and including
 * the IEND).  You will usually have the same info chunk as you
 * had in the header, although some data may have been added
 * to the comments and time fields.
 *
 * Most people won't do much here, perhaps setting a flag that
 * marks the image as finished.
 */
}

/* write a png file */
void write_png(char *file_name, ... other image information ...)
{
   FILE *fp;
   png_structp png_ptr;
   png_infop info_ptr;

   /* open the file */
   fp = fopen(file_name, "wb");
   if (fp == NULL)
      return;

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      (void *)user_error_ptr, user_error_fn, user_warning_fn);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return;
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return;
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error hadnling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_ptr->jmpbuf))
   {
      /* If we get here, we had a problem reading the file */
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return;
   }

   /* One of the following I/O initialization functions is REQUIRED */
**** I/O initialization method 1 ****
   /* set up the output control if you are using standard C streams */
   png_init_io(png_ptr, fp);
**** I/O initialization method 2 ****
   /* If you are using replacement read functions, instead of calling
    * png_init_io() here you would call */
   png_set_write_fn(png_ptr, (void *)user_io_ptr, user_write_fn,
      user_IO_flush_function);
   /* where user_io_ptr is a structure you want available to the callbacks */
**** only use 1 initialization method ****

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_???,
      PNG_INTERLACE_????, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   /* set the palette if there is one.  REQUIRED for indexed-color images */
   palette = (png_colorp)png_malloc(png_ptr, 256 * sizeof (png_color));
   ... set palette colors ...
   png_set_PLTE(png_ptr, info_ptr, palette, 256);

   /* optional significant bit chunk */
   /* if we are dealing with a grayscale image then */
   sig_bit.gray = true_bit_depth;
   /* otherwise, if we are dealing with a color image then */
   sig_bit.red = true_red_bit_depth;
   sig_bit.green = true_green_bit_depth;
   sig_bit.blue = true_blue_bit_depth;
   /* if the image has an alpha channel then */
   sig_bit.alpha = true_alpha_bit_depth;
   png_set_sBIT(png_ptr, info_ptr, sig_bit);

  
   /* Optional gamma chunk is strongly suggested if you have any guess
    * as to the correct gamma of the image.
    */
   png_set_gAMA(png_ptr, info_ptr, gamma);

   /* Optionally write comments into the image */
   text_ptr[0].key = "Title";
   text_ptr[0].text = "Mona Lisa";
   text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[1].key = "Author";
   text_ptr[1].text = "Leonardo DaVinci";
   text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[2].key = "Description";
   text_ptr[2].text = "<long text>";
   text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
   png_set_text(png_ptr, info_ptr, text_ptr, 2);

   /* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
   /* note that if sRGB is present the cHRM chunk must be ignored
    * on read and must be written in accordance with the sRGB profile */

   /* Write the file header information.  REQUIRED */
   png_write_info(png_ptr, info_ptr);

   /* Once we write out the header, the compression type on the text
    * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
    * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
    * at the end.
    */

   /* set up the transformations you want.  Note that these are
    * all optional.  Only call them if you want them.
    */

   /* invert monocrome pixels */
   png_set_invert_mono(png_ptr);

   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
   png_set_shift(png_ptr, &sig_bit);

   /* pack pixels into bytes */
   png_set_packing(png_ptr);

   /* swap location of alpha bytes from ARGB to RGBA */
   png_set_swap_alpha(png_ptr);

   /* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
    * RGB (4 channels -> 3 channels). The second parameter is not used.
    */
   png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

   /* flip BGR pixels to RGB */
   png_set_bgr(png_ptr);

   /* swap bytes of 16-bit files to most significant byte first */
   png_set_swap(png_ptr);

   /* swap bits of 1, 2, 4 bit packed pixel formats */
   png_set_packswap(png_ptr);

   /* turn on interlace handling if you are not using png_write_image() */
   if (interlacing)
      number_passes = png_set_interlace_handling(png_ptr);
   else
      number_passes = 1;

   /* The easiest way to write the image (you may have a different memory
    * layout, however, so choose what fits your needs best).  You need to
    * use the first method if you aren't handling interlacing yourself.
    */
   png_byte row_pointers[height][width];

   /* One of the following output methods is REQUIRED */
**** write out the entire image data in one call ***
   png_write_image(png_ptr, row_pointers);

   /* the other way to write the image - deal with interlacing */

**** write out the image data by one or more scanlines ****
   /* The number of passes is either 1 for non-interlaced images,
    * or 7 for interlaced images.
    */
   for (pass = 0; pass < number_passes; pass++)
   {
      /* Write a few rows at a time. */
      png_write_rows(png_ptr, row_pointers, number_of_rows);

      /* If you are only writing one row at a time, this works */
      for (y = 0; y < height; y++)
      {
         png_bytep row_pointers = row[y];
         png_write_rows(png_ptr, &row_pointers, 1);
      }
   }
**** use only one output method ****

   /* You can write optional chunks like tEXt, zTXt, and tIME at the end
    * as well.
    */

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   /* if you malloced the palette, free it here */
   free(info_ptr->palette);

   /* if you allocated any text comments, free them here */

   /* clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   /* close the file */
   fclose(fp);

   /* that's it */
   return;
}

