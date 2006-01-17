/* Copyright (C) 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevcgmx.h,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Internal definitions for CGM-writing library */

#ifndef gdevcgmx_INCLUDED
#  define gdevcgmx_INCLUDED

#include "gdevcgml.h"

/* Define the internal representations of the CGM opcodes. */
#define cgm_op_class_shift 7
#define cgm_op_id_shift 5
typedef enum {
    /* Class 0 */
    BEGIN_METAFILE = (0 << cgm_op_class_shift) + 1,
    END_METAFILE,
    BEGIN_PICTURE,
    BEGIN_PICTURE_BODY,
    END_PICTURE,
    /* Class 1 */
    METAFILE_VERSION = (1 << cgm_op_class_shift) + 1,
    METAFILE_DESCRIPTION,
    VDC_TYPE,
    INTEGER_PRECISION,
    REAL_PRECISION,
    INDEX_PRECISION,
    COLOR_PRECISION,
    COLOR_INDEX_PRECISION,
    MAXIMUM_COLOR_INDEX,
    COLOR_VALUE_EXTENT,
    METAFILE_ELEMENT_LIST,
    METAFILE_DEFAULTS_REPLACEMENT,
    FONT_LIST,
    CHARACTER_SET_LIST,
    CHARACTER_CODING_ANNOUNCER,
    /* Class 2 */
    SCALING_MODE = (2 << cgm_op_class_shift) + 1,
    COLOR_SELECTION_MODE,
    LINE_WIDTH_SPECIFICATION_MODE,
    MARKER_SIZE_SPECIFICATION_MODE,
    EDGE_WIDTH_SPECIFICATION_MODE,
    VDC_EXTENT,
    BACKGROUND_COLOR,
    /* Class 3 */
    VDC_INTEGER_PRECISION = (3 << cgm_op_class_shift) + 1,
    VDC_REAL_PRECISION,
    AUXILIARY_COLOR,
    TRANSPARENCY,
    CLIP_RECTANGLE,
    CLIP_INDICATOR,
    /* Class 4 */
    POLYLINE = (4 << cgm_op_class_shift) + 1,
    DISJOINT_POLYLINE,
    POLYMARKER,
    TEXT,
    RESTRICTED_TEXT,
    APPEND_TEXT,
    POLYGON,
    POLYGON_SET,
    CELL_ARRAY,
    GENERALIZED_DRAWING_PRIMITIVE,
    RECTANGLE,
    CIRCLE,
    CIRCULAR_ARC_3_POINT,
    CIRCULAR_ARC_3_POINT_CLOSE,
    CIRCULAR_ARC_CENTER,
    CIRCULAR_ARC_CENTER_CLOSE,
    ELLIPSE,
    ELLIPTICAL_ARC,
    ELLIPTICAL_ARC_CLOSE,
    /* Class 5 */
    LINE_BUNDLE_INDEX = (5 << cgm_op_class_shift) + 1,
    LINE_TYPE,
    LINE_WIDTH,
    LINE_COLOR,
    MARKER_BUNDLE_INDEX,
    MARKER_TYPE,
    MARKER_SIZE,
    MARKER_COLOR,
    TEXT_BUNDLE_INDEX,
    TEXT_FONT_INDEX,
    TEXT_PRECISION,
    CHARACTER_EXPANSION_FACTOR,
    CHARACTER_SPACING,
    TEXT_COLOR,
    CHARACTER_HEIGHT,
    CHARACTER_ORIENTATION,
    TEXT_PATH,
    TEXT_ALIGNMENT,
    CHARACTER_SET_INDEX,
    ALTERNATE_CHARACTER_SET_INDEX,
    FILL_BUNDLE_INDEX,
    INTERIOR_STYLE,
    FILL_COLOR,
    HATCH_INDEX,
    PATTERN_INDEX,
    EDGE_BUNDLE_INDEX,
    EDGE_TYPE,
    EDGE_WIDTH,
    EDGE_COLOR,
    EDGE_VISIBILITY,
    FILL_REFERENCE_POINT,
    PATTERN_TABLE,
    PATTERN_SIZE,
    COLOR_TABLE,
    ASPECT_SOURCE_FLAGS,
    /* Class 6 */
    ESCAPE = (6 << cgm_op_class_shift) + 1,
    /* Class 7 */
    MESSAGE = (7 << cgm_op_class_shift) + 1,
    APPLICATION_DATA
} cgm_op_index;

/* Define the state of the CGM writer. */
						/*typedef struct cgm_state_s cgm_state; *//* in gdevcgml.h */
struct cgm_state_s {
    /* The following are set at initialization time. */
    FILE *file;
    cgm_allocator allocator;
    /* The following are set by specific calls. */
    cgm_metafile_elements metafile;
    cgm_picture_elements picture;
    int vdc_integer_precision;
    cgm_precision vdc_real_precision;
    cgm_color auxiliary_color;
    cgm_transparency transparency;
    cgm_point clip_rectangle[2];
    cgm_clip_indicator clip_indicator;
    int line_bundle_index;
    cgm_line_type line_type;
    cgm_line_width line_width;
    cgm_color line_color;
    int marker_bundle_index;
    cgm_marker_type marker_type;
    cgm_marker_size marker_size;
    cgm_color marker_color;
    int text_bundle_index;
    int text_font_index;
    cgm_text_precision text_precision;
    cgm_real character_expansion_factor;
    cgm_real character_spacing;
    cgm_color text_color;
    cgm_vdc character_height;
    cgm_vdc character_orientation[4];
    cgm_text_path text_path;
/****** text_alignment ******/
    int character_set_index;
    int alternate_character_set_index;
    int fill_bundle_index;
    cgm_interior_style interior_style;
    cgm_color fill_color;
    cgm_hatch_index hatch_index;
    int pattern_index;
    int edge_bundle_index;
    cgm_edge_type edge_type;
    cgm_edge_width edge_width;
    bool edge_visibility;
    cgm_point fill_reference_point;
/****** pattern_table ******/
    cgm_vdc pattern_size[4];
/****** color_table ******/
            byte /*cgm_aspect_source */ source_flags[18];
    /* The following change dynamically. */
#define command_max_count 400	/* (must be even) */
    byte command[command_max_count];
    int command_count;
    bool command_first;
    cgm_result result;
};

#endif /* gdevcgmx_INCLUDED */
