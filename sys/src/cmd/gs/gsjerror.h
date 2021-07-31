/*
 * jerror.h
 *
 * Copyright (C) 1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the error and message codes for the JPEG library.
 * Edit this file to add new codes, or to translate the message strings to
 * some other language.
 * A set of error-reporting macros are defined too.  Some applications using
 * the JPEG library may wish to include this file to get the error codes
 * and/or the macros.
 */


/* To define the enum list of message codes, include this file without
 * defining JMAKE_MSG_TABLE.  To create the message string table, include it
 * again with JMAKE_MSG_STRINGS defined, and then yet again with
 * JMAKE_MSG_TABLE defined (this should be done in just one module).
 */

#ifdef JMAKE_MSG_TABLE

/*
 * The following doesn't work, because jpeg_message_table is also
 * the name a member of the jpeg_error_mgr structure.
 */
#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_message_table	jMsgTable
#endif

const char far_data * const far_data jpeg_message_table[] = {

#define JMESSAGE(code,strvar,string)	strvar ,

#else /* not JMAKE_MSG_TABLE */

#ifdef JMAKE_MSG_STRINGS

#define JMESSAGE(code,strvar,string)	const char far_data strvar[] = string;

#else /* not JMAKE_MSG_STRINGS, not JMAKE_MSG_TABLE */

typedef enum {

#define JMESSAGE(code,strvar,string)	code ,

#endif /* JMAKE_MSG_STRINGS */

#endif /* JMAKE_MSG_TABLE */

JMESSAGE(JMSG_NOMESSAGE, str_JMSG_NOMESSAGE,
	 "Bogus message code %d") /* Must be first entry! */

/* For maintenance convenience, list is alphabetical by message code name */
JMESSAGE(JERR_ARITH_NOTIMPL, str_JERR_ARITH_NOTIMPL,
	 "Sorry, there are legal restrictions on arithmetic coding")
JMESSAGE(JERR_BAD_ALIGN_TYPE, str_JERR_BAD_ALIGN_TYPE,
	 "ALIGN_TYPE is wrong, please fix")
JMESSAGE(JERR_BAD_ALLOC_CHUNK, str_JERR_BAD_ALLOC_CHUNK,
	 "MAX_ALLOC_CHUNK is wrong, please fix")
JMESSAGE(JERR_BAD_BUFFER_MODE, str_JERR_BAD_BUFFER_MODE,
	 "Bogus buffer control mode")
JMESSAGE(JERR_BAD_COMPONENT_ID, str_JERR_BAD_COMPONENT_ID,
	 "Invalid component ID %d in SOS")
JMESSAGE(JERR_BAD_DCTSIZE, str_JERR_BAD_DCTSIZE,
	 "IDCT output block size %d not supported")
JMESSAGE(JERR_BAD_IN_COLORSPACE, str_JERR_BAD_IN_COLORSPACE,
	 "Bogus input colorspace")
JMESSAGE(JERR_BAD_J_COLORSPACE, str_JERR_BAD_J_COLORSPACE,
	 "Bogus JPEG colorspace")
JMESSAGE(JERR_BAD_LENGTH, str_JERR_BAD_LENGTH,
	 "Bogus marker length")
JMESSAGE(JERR_BAD_MCU_SIZE, str_JERR_BAD_MCU_SIZE,
	 "Sampling factors too large for interleaved scan")
JMESSAGE(JERR_BAD_POOL_ID, str_JERR_BAD_POOL_ID,
	 "Invalid memory pool code %d")
JMESSAGE(JERR_BAD_PRECISION, str_JERR_BAD_PRECISION,
	 "Unsupported JPEG data precision %d")
JMESSAGE(JERR_BAD_SAMPLING, str_JERR_BAD_SAMPLING,
	 "Bogus sampling factors")
JMESSAGE(JERR_BAD_STATE, str_JERR_BAD_STATE,
	 "Improper call to JPEG library in state %d")
JMESSAGE(JERR_BAD_VIRTUAL_ACCESS, str_JERR_BAD_VIRTUAL_ACCESS,
	 "Bogus virtual array access")
JMESSAGE(JERR_BUFFER_SIZE, str_JERR_BUFFER_SIZE,
	 "Buffer passed to JPEG library is too small")
JMESSAGE(JERR_CANT_SUSPEND, str_JERR_CANT_SUSPEND,
	 "Suspension not allowed here")
JMESSAGE(JERR_CCIR601_NOTIMPL, str_JERR_CCIR601_NOTIMPL,
	 "CCIR601 sampling not implemented yet")
JMESSAGE(JERR_COMPONENT_COUNT, str_JERR_COMPONENT_COUNT,
	 "Too many color components: %d, max %d")
JMESSAGE(JERR_CONVERSION_NOTIMPL, str_JERR_CONVERSION_NOTIMPL,
	 "Unsupported color conversion request")
JMESSAGE(JERR_DAC_INDEX, str_JERR_DAC_INDEX,
	 "Bogus DAC index %d")
JMESSAGE(JERR_DAC_VALUE, str_JERR_DAC_VALUE,
	 "Bogus DAC value 0x%x")
JMESSAGE(JERR_DHT_COUNTS, str_JERR_DHT_COUNTS,
	 "Bogus DHT counts")
JMESSAGE(JERR_DHT_INDEX, str_JERR_DHT_INDEX,
	 "Bogus DHT index %d")
JMESSAGE(JERR_DQT_INDEX, str_JERR_DQT_INDEX,
	 "Bogus DQT index %d")
JMESSAGE(JERR_EMPTY_IMAGE, str_JERR_EMPTY_IMAGE,
	 "Empty JPEG image (DNL not supported)")
JMESSAGE(JERR_EMS_READ, str_JERR_EMS_READ,
	 "Read from EMS failed")
JMESSAGE(JERR_EMS_WRITE, str_JERR_EMS_WRITE,
	 "Write to EMS failed")
JMESSAGE(JERR_EOI_EXPECTED, str_JERR_EOI_EXPECTED,
	 "Didn't expect more than one scan")
JMESSAGE(JERR_FILE_READ, str_JERR_FILE_READ,
	 "Input file read error")
JMESSAGE(JERR_FILE_WRITE, str_JERR_FILE_WRITE,
	 "Output file write error --- out of disk space?")
JMESSAGE(JERR_FRACT_SAMPLE_NOTIMPL, str_JERR_FRACT_SAMPLE_NOTIMPL,
	 "Fractional sampling not implemented yet")
JMESSAGE(JERR_HUFF_CLEN_OVERFLOW, str_JERR_HUFF_CLEN_OVERFLOW,
	 "Huffman code size table overflow")
JMESSAGE(JERR_HUFF_MISSING_CODE, str_JERR_HUFF_MISSING_CODE,
	 "Missing Huffman code table entry")
JMESSAGE(JERR_IMAGE_TOO_BIG, str_JERR_IMAGE_TOO_BIG,
	 "Maximum supported image dimension is %u pixels")
JMESSAGE(JERR_INPUT_EMPTY, str_JERR_INPUT_EMPTY,
	 "Empty input file")
JMESSAGE(JERR_INPUT_EOF, str_JERR_INPUT_EOF,
	 "Premature end of input file")
JMESSAGE(JERR_JFIF_MAJOR, str_JERR_JFIF_MAJOR,
	 "Unsupported JFIF revision number %d.%02d")
JMESSAGE(JERR_NOTIMPL, str_JERR_NOTIMPL,
	 "Not implemented yet")
JMESSAGE(JERR_NOT_COMPILED, str_JERR_NOT_COMPILED,
	 "Requested feature was omitted at compile time")
JMESSAGE(JERR_NO_BACKING_STORE, str_JERR_NO_BACKING_STORE,
	 "Backing store not supported")
JMESSAGE(JERR_NO_HUFF_TABLE, str_JERR_NO_HUFF_TABLE,
	 "Huffman table 0x%02x was not defined")
JMESSAGE(JERR_NO_IMAGE, str_JERR_NO_IMAGE,
	 "JPEG datastream contains no image")
JMESSAGE(JERR_NO_QUANT_TABLE, str_JERR_NO_QUANT_TABLE,
	 "Quantization table 0x%02x was not defined")
JMESSAGE(JERR_NO_SOI, str_JERR_NO_SOI,
	 "Not a JPEG file: starts with 0x%02x 0x%02x")
JMESSAGE(JERR_OUT_OF_MEMORY, str_JERR_OUT_OF_MEMORY,
	 "Insufficient memory (case %d)")
JMESSAGE(JERR_QUANT_COMPONENTS, str_JERR_QUANT_COMPONENTS,
	 "Cannot quantize more than %d color components")
JMESSAGE(JERR_QUANT_FEW_COLORS, str_JERR_QUANT_FEW_COLORS,
	 "Cannot quantize to fewer than %d colors")
JMESSAGE(JERR_QUANT_MANY_COLORS, str_JERR_QUANT_MANY_COLORS,
	 "Cannot quantize to more than %d colors")
JMESSAGE(JERR_SOF_DUPLICATE, str_JERR_SOF_DUPLICATE,
	 "Invalid JPEG file structure: two SOF markers")
JMESSAGE(JERR_SOF_NO_SOS, str_JERR_SOF_NO_SOS,
	 "Invalid JPEG file structure: missing SOS marker")
JMESSAGE(JERR_SOF_UNSUPPORTED, str_JERR_SOF_UNSUPPORTED,
	 "Unsupported JPEG process: SOF type 0x%02x")
JMESSAGE(JERR_SOI_DUPLICATE, str_JERR_SOI_DUPLICATE,
	 "Invalid JPEG file structure: two SOI markers")
JMESSAGE(JERR_SOS_NO_SOF, str_JERR_SOS_NO_SOF,
	 "Invalid JPEG file structure: SOS before SOF")
JMESSAGE(JERR_TFILE_CREATE, str_JERR_TFILE_CREATE,
	 "Failed to create temporary file %s")
JMESSAGE(JERR_TFILE_READ, str_JERR_TFILE_READ,
	 "Read failed on temporary file")
JMESSAGE(JERR_TFILE_SEEK, str_JERR_TFILE_SEEK,
	 "Seek failed on temporary file")
JMESSAGE(JERR_TFILE_WRITE, str_JERR_TFILE_WRITE,
	 "Write failed on temporary file --- out of disk space?")
JMESSAGE(JERR_TOO_LITTLE_DATA, str_JERR_TOO_LITTLE_DATA,
	 "Application transferred too few scanlines")
JMESSAGE(JERR_UNKNOWN_MARKER, str_JERR_UNKNOWN_MARKER,
	 "Unsupported marker type 0x%02x")
JMESSAGE(JERR_VIRTUAL_BUG, str_JERR_VIRTUAL_BUG,
	 "Virtual array controller messed up")
JMESSAGE(JERR_WIDTH_OVERFLOW, str_JERR_WIDTH_OVERFLOW,
	 "Image too wide for this implementation")
JMESSAGE(JERR_XMS_READ, str_JERR_XMS_READ,
	 "Read from XMS failed")
JMESSAGE(JERR_XMS_WRITE, str_JERR_XMS_WRITE,
	 "Write to XMS failed")
JMESSAGE(JMSG_COPYRIGHT, str_JMSG_COPYRIGHT,
	 JCOPYRIGHT)
JMESSAGE(JMSG_VERSION, str_JMSG_VERSION,
	 JVERSION)
JMESSAGE(JTRC_16BIT_TABLES, str_JTRC_16BIT_TABLES,
	 "Caution: quantization tables are too coarse for baseline JPEG")
JMESSAGE(JTRC_ADOBE, str_JTRC_ADOBE,
	 "Adobe APP14 marker: version %d, flags 0x%04x 0x%04x, transform %d")
JMESSAGE(JTRC_APP0, str_JTRC_APP0,
	 "Unknown APP0 marker (not JFIF), length %u")
JMESSAGE(JTRC_APP14, str_JTRC_APP14,
	 "Unknown APP14 marker (not Adobe), length %u")
JMESSAGE(JTRC_DAC, str_JTRC_DAC,
	 "Define Arithmetic Table 0x%02x: 0x%02x")
JMESSAGE(JTRC_DHT, str_JTRC_DHT,
	 "Define Huffman Table 0x%02x")
JMESSAGE(JTRC_DQT, str_JTRC_DQT,
	 "Define Quantization Table %d  precision %d")
JMESSAGE(JTRC_DRI, str_JTRC_DRI,
	 "Define Restart Interval %u")
JMESSAGE(JTRC_EMS_CLOSE, str_JTRC_EMS_CLOSE,
	 "Freed EMS handle %u")
JMESSAGE(JTRC_EMS_OPEN, str_JTRC_EMS_OPEN,
	 "Obtained EMS handle %u")
JMESSAGE(JTRC_EOI, str_JTRC_EOI,
	 "End Of Image")
JMESSAGE(JTRC_HUFFBITS, str_JTRC_HUFFBITS,
	 "        %3d %3d %3d %3d %3d %3d %3d %3d")
JMESSAGE(JTRC_JFIF, str_JTRC_JFIF,
	 "JFIF APP0 marker, density %dx%d  %d")
JMESSAGE(JTRC_JFIF_BADTHUMBNAILSIZE, str_JTRC_JFIF_BADTHUMBNAILSIZE,
	 "Warning: thumbnail image size does not match data length %u")
JMESSAGE(JTRC_JFIF_MINOR, str_JTRC_JFIF_MINOR,
	 "Warning: unknown JFIF revision number %d.%02d")
JMESSAGE(JTRC_JFIF_THUMBNAIL, str_JTRC_JFIF_THUMBNAIL,
	 "    with %d x %d thumbnail image")
JMESSAGE(JTRC_MISC_MARKER, str_JTRC_MISC_MARKER,
	 "Skipping marker 0x%02x, length %u")
JMESSAGE(JTRC_PARMLESS_MARKER, str_JTRC_PARMLESS_MARKER,
	 "Unexpected marker 0x%02x")
JMESSAGE(JTRC_QUANTVALS, str_JTRC_QUANTVALS,
	 "        %4u %4u %4u %4u %4u %4u %4u %4u")
JMESSAGE(JTRC_QUANT_3_NCOLORS, str_JTRC_QUANT_3_NCOLORS,
	 "Quantizing to %d = %d*%d*%d colors")
JMESSAGE(JTRC_QUANT_NCOLORS, str_JTRC_QUANT_NCOLORS,
	 "Quantizing to %d colors")
JMESSAGE(JTRC_QUANT_SELECTED, str_JTRC_QUANT_SELECTED,
	 "Selected %d colors for quantization")
JMESSAGE(JTRC_RECOVERY_ACTION, str_JTRC_RECOVERY_ACTION,
	 "At marker 0x%02x, recovery action %d")
JMESSAGE(JTRC_RST, str_JTRC_RST,
	 "RST%d")
JMESSAGE(JTRC_SMOOTH_NOTIMPL, str_JTRC_SMOOTH_NOTIMPL,
	 "Smoothing not supported with nonstandard sampling ratios")
JMESSAGE(JTRC_SOF, str_JTRC_SOF,
	 "Start Of Frame 0x%02x: width=%u, height=%u, components=%d")
JMESSAGE(JTRC_SOF_COMPONENT, str_JTRC_SOF_COMPONENT,
	 "    Component %d: %dhx%dv q=%d")
JMESSAGE(JTRC_SOI, str_JTRC_SOI,
	 "Start of Image")
JMESSAGE(JTRC_SOS, str_JTRC_SOS,
	 "Start Of Scan: %d components")
JMESSAGE(JTRC_SOS_COMPONENT, str_JTRC_SOS_COMPONENT,
	 "    Component %d: dc=%d ac=%d")
JMESSAGE(JTRC_TFILE_CLOSE, str_JTRC_TFILE_CLOSE,
	 "Closed temporary file %s")
JMESSAGE(JTRC_TFILE_OPEN, str_JTRC_TFILE_OPEN,
	 "Opened temporary file %s")
JMESSAGE(JTRC_UNKNOWN_IDS, str_JTRC_UNKNOWN_IDS,
	 "Unrecognized component IDs %d %d %d, assuming YCbCr")
JMESSAGE(JTRC_XMS_CLOSE, str_JTRC_XMS_CLOSE,
	 "Freed XMS handle %u")
JMESSAGE(JTRC_XMS_OPEN, str_JTRC_XMS_OPEN,
	 "Obtained XMS handle %u")
JMESSAGE(JWRN_ADOBE_XFORM, str_JWRN_ADOBE_XFORM,
	 "Unknown Adobe color transform code %d")
JMESSAGE(JWRN_EXTRANEOUS_DATA, str_JWRN_EXTRANEOUS_DATA,
	 "Corrupt JPEG data: %u extraneous bytes before marker 0x%02x")
JMESSAGE(JWRN_HIT_MARKER, str_JWRN_HIT_MARKER,
	 "Corrupt JPEG data: premature end of data segment")
JMESSAGE(JWRN_HUFF_BAD_CODE, str_JWRN_HUFF_BAD_CODE,
	 "Corrupt JPEG data: bad Huffman code")
JMESSAGE(JWRN_JPEG_EOF, str_JWRN_JPEG_EOF,
	 "Premature end of JPEG file")
JMESSAGE(JWRN_MUST_RESYNC, str_JWRN_MUST_RESYNC,
	 "Corrupt JPEG data: found marker 0x%02x instead of RST%d")
JMESSAGE(JWRN_NOT_SEQUENTIAL, str_JWRN_NOT_SEQUENTIAL,
	 "Invalid SOS parameters for sequential JPEG")
JMESSAGE(JWRN_TOO_MUCH_DATA, str_JWRN_TOO_MUCH_DATA,
	 "Application transferred too many scanlines")

#ifdef JMAKE_MSG_TABLE

  NULL
};

#else /* not JMAKE_MSG_TABLE */

#ifndef JMAKE_MSG_STRINGS

  JMSG_LASTMSGCODE
} J_MESSAGE_CODE;

#endif /* JMAKE_MSG_STRINGS */

#endif /* JMAKE_MSG_TABLE */

#undef JMESSAGE


#ifndef JMAKE_MSG_TABLE
#ifndef JMAKE_MSG_STRINGS

/* Macros to simplify using the error and trace message stuff */
/* The first parameter is either type of cinfo pointer */

/* Fatal errors (print message and exit) */
#define ERREXIT(cinfo,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))
#define ERREXIT1(cinfo,code,p1)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))
#define ERREXIT2(cinfo,code,p1,p2)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (cinfo)->err->msg_parm.i[1] = (p2), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))
#define ERREXIT3(cinfo,code,p1,p2,p3)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (cinfo)->err->msg_parm.i[1] = (p2), \
   (cinfo)->err->msg_parm.i[2] = (p3), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))
#define ERREXIT4(cinfo,code,p1,p2,p3,p4)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (cinfo)->err->msg_parm.i[1] = (p2), \
   (cinfo)->err->msg_parm.i[2] = (p3), \
   (cinfo)->err->msg_parm.i[3] = (p4), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))
#define ERREXITS(cinfo,code,str)  \
  ((cinfo)->err->msg_code = (code), \
   strncpy((cinfo)->err->msg_parm.s, (str), JMSG_STR_PARM_MAX), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))

#define MAKESTMT(stuff)		do { stuff } while (0)

/* Nonfatal errors (we can keep going, but the data is probably corrupt) */
#define WARNMS(cinfo,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), -1))
#define WARNMS1(cinfo,code,p1)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), -1))
#define WARNMS2(cinfo,code,p1,p2)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (cinfo)->err->msg_parm.i[1] = (p2), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), -1))

/* Informational/debugging messages */
#define TRACEMS(cinfo,lvl,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)))
#define TRACEMS1(cinfo,lvl,code,p1)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)))
#define TRACEMS2(cinfo,lvl,code,p1,p2)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (cinfo)->err->msg_parm.i[1] = (p2), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)))
#define TRACEMS3(cinfo,lvl,code,p1,p2,p3)  \
  MAKESTMT(int * _mp = (cinfo)->err->msg_parm.i; \
	   _mp[0] = (p1); _mp[1] = (p2); _mp[2] = (p3); \
	   (cinfo)->err->msg_code = (code); \
	   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)); )
#define TRACEMS4(cinfo,lvl,code,p1,p2,p3,p4)  \
  MAKESTMT(int * _mp = (cinfo)->err->msg_parm.i; \
	   _mp[0] = (p1); _mp[1] = (p2); _mp[2] = (p3); _mp[3] = (p4); \
	   (cinfo)->err->msg_code = (code); \
	   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)); )
#define TRACEMS8(cinfo,lvl,code,p1,p2,p3,p4,p5,p6,p7,p8)  \
  MAKESTMT(int * _mp = (cinfo)->err->msg_parm.i; \
	   _mp[0] = (p1); _mp[1] = (p2); _mp[2] = (p3); _mp[3] = (p4); \
	   _mp[4] = (p5); _mp[5] = (p6); _mp[6] = (p7); _mp[7] = (p8); \
	   (cinfo)->err->msg_code = (code); \
	   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)); )
#define TRACEMSS(cinfo,lvl,code,str)  \
  ((cinfo)->err->msg_code = (code), \
   strncpy((cinfo)->err->msg_parm.s, (str), JMSG_STR_PARM_MAX), \
   (*(cinfo)->err->emit_message) ((j_common_ptr) (cinfo), (lvl)))

#endif /* JMAKE_MSG_STRINGS */
#endif /* JMAKE_MSG_TABLE */
