/* 
 *  scribble.h:			User-Level API for Handwriting Recognition
 *  Author:				James Kempf
 *  Created On:			Mon Nov  2 14:01:25 1992
 *  Last Modified By:	Sape Mullender
 *  Last Modified On:	Fri Aug 25 10:24:50 EDT 2000
 *  Copyright (c) 1994 by Sun Microsystems Computer Company
 *  All rights reserved.
 *  
 *  Use and copying of this software and preparation of 
 *  derivative works based upon this software are permitted.
 *  Any distribution of this software or derivative works
 *  must comply with all applicable United States export control
 *  laws.
 *
 *  This software is made available as is, and Sun Microsystems
 *  Computer Company makes no warranty about the software, its
 *  performance, or its conformity to any specification
 */

/*
 * Opaque type for the recognizer. The toolkit must access through
 * appropriate access functions.
 */
#pragma incomplete struct _Recognizer
typedef struct _Recognizer* recognizer;

/*
 * Opaque type for recognizers to implement dictionaries.
 */

typedef struct _wordset		*wordset;
typedef struct rc		rc;
typedef struct rec_correlation	rec_correlation;
typedef struct rec_alternative	rec_alternative;
typedef struct rec_element	rec_element;
typedef struct gesture		gesture;
typedef uint			wchar_t;

/* Scalar Type Definitions */

/* For better readibility.*/

typedef int bool;

#define true 1
#define false 0

/*For pointers to extra functions on recognizer.*/

typedef void (*rec_fn)();

/*
 * rec_confidence is an integer between 0-100 giving the confidence of the
 * recognizer in a particular result.
 */

typedef uchar rec_confidence;

/**************** RECOGNIZER CONFIGURATION INFORMATION *******************/

/*
 * Recognizer information. Gives the locale, category of the character
 * set returned by the recognizer, and any subsets to which the
 * recognition can be limited. The locale and category should be
 * suitable for the setlocale(3). Those recognizers which don't do text
 * can simply report a blank locale and category, and report the
 * graphics types they recognize in the subset. 
 */

typedef struct {
    char* ri_locale;        /*The locale of the character set.*/
    char* ri_name;          /*Complete pathname to the recognizer.*/
    char** ri_subset;       /*Null terminated list of subsets supported*/
} rec_info;

/*These define a set of common character subset names.*/

#define GESTURE		"GESTURE"		/* gestures only */
#define MATHSET		"MATHSET"		/* %^*()_+={}<>,/. */
#define MONEYSET	"MONEYSET"		/* $, maybe cent, pound, and yen */
#define WHITESPACE	"WHITESPACE"	/* gaps are recognized as space */
#define KANJI_JIS1	"KANJI_JIS1"	/* the JIS1 kanji only */
#define KANJI_JIS1_PLUS	"KANJI_JIS1_PLUS" /* JIS1 plus some JIS2 */
#define KANJI_JIS2	"KANJI_JIS2"	/* the JIS1 + JIS2 kanji */
#define HIRIGANA	"HIRIGANA"		/* the hirigana */
#define KATAKANA	"KATAKANA"		/* the katakana */
#define UPPERCASE	"UPPERCASE"		/* upper case alphabetics, no digits */
#define LOWERCASE	"LOWERCASE"		/* lower case alphabetics, no digits */
#define DIGITS		"DIGITS"		/* digits 0-9 only */
#define PUNCTUATION	"PUNCTUATION"	/* \!-;'"?()&., */
#define NONALPHABETIC	"NONALPHABETIC" /* all nonalphabetics, no digits */
#define ASCII		"ASCII"			/* the ASCII character set */
#define ISO_LATIN12	"ISO_LATIN12"	/* The ISO Latin 12 characters */


/********************  RECOGNITION INPUT STRUCTURES ***********************/

/*
 * WINDOW SYSTEM INTERFACE
*/

/*Bounding box. Structurally identical to Rectangle.*/

typedef Rectangle pen_rect;    


/*
 * RECOGNITION CONTEXT
 */

/* Structure for reporting writing area geometric constraints. */

typedef struct {
	pen_rect pr_area;
	short pr_row, pr_col;
} pen_frame; 

/* 
 * Structure for describing a set of letters to constrain recognition. 
 * ls_type is the same as the re_type field for rec_element below.
*/

typedef struct _letterset {
        char ls_type;
        union _ls_set {
                char* aval;
                wchar_t* wval;
        } ls_set;
} letterset;

/********************* RECOGNITION RETURN VALUES *************************/


/*Different types in union. "Other" indicates a cast is needed.*/

#define REC_NONE    0x0             /*No return value*/
#define REC_GESTURE 0x1             /*Gesture.*/
#define REC_ASCII   0x2             /*Array of 8 bit ASCII*/
#define REC_VAR     0x4             /*Array of variable width characters. */
#define REC_WCHAR   0x8             /*Array of Unicode (wide) characters. */
#define REC_OTHER   0x10            /*Undefined type.*/
#define REC_CORR    0x20	    /*rec_correlation struct*/

/*
 * Recognition elements. A recognition element is a structure having a 
 * confidence level member, and a union, along with a flag indicating 
 * the union type. The union contains a pointer to the result. This
 * is the basic recognition return value, corresponding to one
 * recognized word, letter, or group of letters.
*/

struct rec_element {
	char			re_type;		/*Union type flag.*/
	union {
		gesture	*			gval;	/*Gesture.*/
		char*				aval;	/*ASCII and variable width.*/
		wchar_t*			wval;	/*Unicode.*/
		rec_correlation*	rcval;	/*rec_correlation*/
	} re_result;                   
	rec_confidence	re_conf;        /*Confidence (0-100).*/
};

/*
 * Recognition alternative. The recognition alternative gives
 * a translated element for a particular segmentation, and
 * a pointer to an array of alternatives for the next position
 * in the segmentation thread.
*/

struct rec_alternative {
	rec_element			ra_elem; 	/*the translated element*/
	uint				ra_nalter;	/*number of next alternatives*/
	rec_alternative*	ra_next;	/*the array of next alternatives*/
};

/**************************  GESTURES  **************************/

/*
 * Gestures. The toolkit initializes the recognizer with a
 * set of gestures having appropriate callbacks. 
 * When a gesture is recognized, it is returned as part of a
 * recognition element. The recognizer fills in the bounding
 * box and hotspots. The toolkit fills in any additional values,
 * such as the current window, and calls the callback.
*/

struct gesture {
	char*		g_name;			/*The gesture's name.*/
	uint			g_nhs;			/*Number of hotspots.*/
	pen_point*	g_hspots;			/*The hotspots.*/
	pen_rect		g_bbox;			/*The bounding box.*/
	void	  		(*g_action)(gesture*);	/*Pointer to execution function.*/
	void*		g_wsinfo;			/*For toolkit to fill in.*/
};

typedef void (*xgesture)(gesture*);

/*
 * Recognition correlation. A recognition correlation is a recognition
 * of the stroke input along with a correlation between the stroke
 * input and the recognized text. The rec_correlation struct contains
 * a pointer to an arrray of pointers to strokes, and
 * two arrays of integers, giving the starting point and
 * stopping point of each corresponding recogition element returned
 * in the strokes. 
 */

struct rec_correlation {
	rec_element	ro_elem;			/*The recognized alternative.*/
	uint		ro_nstrokes;		/*Number of strokes.*/
	Stroke*	ro_strokes;			/*Array of strokes.*/
	uint*		ro_start;			/*Starting index of points.*/
	uint*		ro_stop;			/*Stopping index of points.*/
};

/*
 * ADMINISTRATION
 */

/*
 * recognizer_load - If directory is not NULL, then use it as a pathname
 * to find the recognizer. Otherwise, use the default naming conventions
 * to find the recognizer having file name name. The subset argument
 * contains a null-terminated array of names for character subsets which
 * the recognizer should translate.
 */

recognizer	recognizer_load(char*, char*, char**);

/*
 * recognizer_unload - Unload the recognizer.
 */

int			recognizer_unload(recognizer);

/*
 * recognizer_get_info-Get a pointer to a rec_info 
 * giving the locale and subsets supported by the recognizer, and shared
 * library pathname.
 */

const rec_info*	recognizer_get_info(recognizer);

/*
 * recognizer_manager_version-Return the version number string of the
 * recognition manager.
 */

const char*	recognizer_manager_version(recognizer);

/*
 * recognizer_load_state-Get any recognizer state associated with name
 * in dir. Note that name may not be simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
 */

int			recognizer_load_state(recognizer, char*, char*);

/*
 * recognizer_save_state-Save any recognizer state to name
 * in dir. Note that name may not be a simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
 */

int			recognizer_save_state(recognizer, char*, char*);

/*
 * recognizer_error-Return the last error message, or NULL if none.
 */

char*		recognizer_error(recognizer);

/*
 * DICTIONARIES
 */

/* recognizer_load_dictionary-Load a dictionary from the directory
 * dir and file name. Return the dictionary pointer if successful,
 * otherwise NULL.
 */

wordset		recognizer_load_dictionary(recognizer, char*, char*);

/* recoginzer_save_dictionary-Save the dictionary to the file. Return 0
 * successful, -1 if error occurs.
 */

int			recognizer_save_dictionary(recognizer, char*, char*, wordset);

/*
 * recognizer_free_dictionary-Free the dictionary. Return 0 if successful,
 * -1 if error occurs.
 */

int			recognizer_free_dictionary(recognizer, wordset);

/*
 * recognizer_add_to_dictionary-Add the word to the dictionary. Return 0
 * if successful, -1 if error occurs.
 */

int			recognizer_add_to_dictionary(recognizer, letterset*, wordset);

/*
 * recognizer_delete_from_dictionary-Delete the word from the dictionary.
 * Return 0 if successful, -1 if error occurs.
 */

int			recognizer_delete_from_dictionary(recognizer, letterset*, wordset);

/*
 * TRANSLATION
 */

/* recognizer_set/get_context - Set/get the recognition context for 
 * subsequent buffering and translation. recognizer_set_context() 
 * returns -1 if an error occurs, otherwise 0. recognizer_get_context() 
 * returns NULL if no context has been set. The context is copied to avoid 
 * potential memory deallocation problems.
 */

int			recognizer_set_context(recognizer, rc*);
rc*			recognizer_get_context(recognizer);

/* recognizer_clear - Set stroke buffer to NULL and clear the context. 
 * Returns -1 if an error occurred, otherwise 0. Both the context and the 
 * stroke buffer are deallocated. If delete_points_p is true, delete the
 * points also.
 */

int			recognizer_clear(recognizer, bool);

/* recognizer_get/set_buffer - Get/set the stroke buffer. The stroke buffer 
 * is copied to avoid potential memory allocation problems. Returns -1 if 
 * an error occurs, otherwise 0.
 */

int			recognizer_get_buffer(recognizer, uint*, Stroke**);
int			recognizer_set_buffer(recognizer, uint, Stroke*);

/* recognizer_translate - Copy the strokes argument into the stroke buffer and
 * translate the buffer. If correlate_p is true, then provide stroke 
 * correlations as well. If either nstrokes is 0 or strokes is NULL, then 
 * just translate the stroke buffer and return the translation. Return an 
 * array of alternative translation segmentations in the ret pointer and the 
 * number of alternatives in nret, or NULL and 0 if there is no translation. 
 * The direction of segmentation is as specified by the rc_direction field in 
 * the buffered recognition context. Returns -1 if an error occurred, 
 * otherwise 0. 
 */

int			recognizer_translate(recognizer, uint, Stroke*, bool,
				int*, rec_alternative**);

/*
 * recognizer_get_extension_functions-Return a null terminated array
 * of functions providing extended functionality. Their interfaces
 * will change depending on the recognizer.
 */

rec_fn*		recognizer_get_extension_functions(recognizer);

/*
 * GESTURE SUPPORT
*/

/*
 * recognizer_get_gesture_names - Return a null terminated array of
 * character strings containing the gesture names.
 */

char**		recognizer_get_gesture_names(recognizer);

/*
 * recognizer_set_gesture_action-Set the action function associated with the 
 *  name.
 */

xgesture	recognizer_set_gesture_action(recognizer, char*, xgesture, void*);

/*
 * The following functions are for deleting data structures returned
 *   by the API functions.
 */

void		delete_rec_alternative_array(uint, rec_alternative*, bool);
void		delete_rec_correlation(rec_correlation*, bool);

/*
 * These are used by clients to create arrays for passing to API
 *  functions.
 */

Stroke*	make_Stroke_array(uint);
void		delete_Stroke_array(uint, Stroke*, bool);

pen_point* 	make_pen_point_array(uint);
void 		delete_pen_point_array(pen_point*);

Stroke*	copy_Stroke_array(uint, Stroke*);

/*Extension function interfaces and indices.*/

#define LI_ISA_LI		0	/*Is this a li recognizer?.*/
#define LI_TRAIN		1	/*Train recognizer*/
#define LI_CLEAR		2	/* ari's clear-state extension fn. */
#define LI_GET_CLASSES	3	/* ari's get-classes extension fn. */
#define LI_NUM_EX_FNS	4	/*Number of extension functions*/

typedef bool	(*li_isa_li)(recognizer r);
typedef int		(*li_recognizer_train)(recognizer, rc*, uint,
					Stroke*, rec_element*, bool);
typedef int		(*li_recognizer_clearState)(recognizer);
typedef int		(*li_recognizer_getClasses)(recognizer, char ***, int *);
