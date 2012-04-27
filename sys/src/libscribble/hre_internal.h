/* 
 *  hre_internal.h:   Internal Interface for Recognizer.
 *  Author:           James Kempf
 *  Created On:       Thu Nov  5 10:54:18 1992
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:51:15 1994
 *  Update Count:     99
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

/*Avoids forward reference problem.*/

/*
 * Internal view of wordset. The recognition engine uses this view to
 * maintain information about which recognizer object this wordset
 * belongs to, which file (in case it needs to be saved), and internal
 * data structures.
*/

struct _wordset {
	char* ws_pathname;		/*Path name to word set file.*/
	recognizer ws_recognizer;	/*To whom it belongs.*/
	void* ws_internal;		/*Internal data structures.*/
};

/*
 * Internal view of the recognizer struct. This view is only available
 * to OEM clients who implement a recognizer shared library. Clients
 * of the recognizer itself see it as an opaque data type. The struct
 * contains a function pointer for each function in the client API.
*/

struct _Recognizer {
	uint		recognizer_magic;
	char		*recognizer_version; 

	rec_info	*recognizer_info;
	void		*recognizer_specific;
	int		(*recognizer_load_state)(struct _Recognizer*, char*, char*);
	int		(*recognizer_save_state)(struct _Recognizer*, char*, char*);
	char*		(*recognizer_error)(struct _Recognizer*);
	wordset		(*recognizer_load_dictionary)(struct _Recognizer*, char*, char*);
	int		(*recognizer_save_dictionary)(struct _Recognizer*, char*, char*, wordset);

	int		(*recognizer_free_dictionary)(struct _Recognizer*, wordset);
	int		(*recognizer_add_to_dictionary)(struct _Recognizer*, letterset*, wordset);
	int		(*recognizer_delete_from_dictionary)(struct _Recognizer*, letterset*, wordset);
	int		(*recognizer_set_context)(struct _Recognizer*,rc*);
	rc*		(*recognizer_get_context)(struct _Recognizer*);
				   
	int		(*recognizer_clear)(struct _Recognizer*, bool);
	int		(*recognizer_get_buffer)(struct _Recognizer*, uint*, Stroke**);

	int		(*recognizer_set_buffer)(struct _Recognizer*, uint, Stroke*);
	int		(*recognizer_translate)(struct _Recognizer*, uint, Stroke*, bool, int*, rec_alternative**);
	rec_fn*		(*recognizer_get_extension_functions)(struct _Recognizer*);
	char**		(*recognizer_get_gesture_names)(struct _Recognizer*);
	xgesture	(*recognizer_set_gesture_action)(struct _Recognizer*, char*, xgesture, void*);
	uint recognizer_end_magic; 
};

/*
 * recognizer_internal_initialize - Allocate and initialize the recognizer 
 * object. The recognition shared library has the responsibility for filling
 * in all the function pointers for the recognition functions. This
 * function must be defined as a global function within the shared
 * library, so it can be accessed using dlsym() when the recognizer
 * shared library is loaded. It returns NULL if an error occured and
 * sets errno to indicate what.
*/

typedef recognizer (*recognizer_internal_initialize)(rec_info* ri);

/*Function header definition for recognizer internal initializer.*/

#define RECOGNIZER_INITIALIZE(_a) \
        recognizer __recognizer_internal_initialize(rec_info* _a)

/*
 * recognizer_internal_finalize - Deallocate and deinitialize the recognizer
 * object. If the recognizer has allocated any additional storage, it should
 * be deallocated as well. Returns 0 if successful, -1 if the argument
 * wasn't a recognizer or wasn't a recognizer handled by this library.
*/

typedef int (*recognizer_internal_finalize)(recognizer r);

#define RECOGNIZER_FINALIZE(_a) \
       int __recognizer_internal_finalize(recognizer _a)

/*
 * The following are for creating HRE structures.
 */

recognizer			make_recognizer(rec_info* ri);
void 				delete_recognizer(recognizer rec);

RECOGNIZER_FINALIZE(_a);
rec_alternative*	make_rec_alternative_array(uint size);
rec_correlation*	make_rec_correlation(char type, uint size, void* trans, rec_confidence conf, uint ps_size);

rec_fn* 
make_rec_fn_array(uint size);
void 
delete_rec_fn_array(rec_fn* rf);

gesture* 
initialize_gesture(gesture* g,
		   char* name,
		   uint nhs,
		   pen_point* hspots,
		   pen_rect bbox,
		   xgesture cback,
		   void* wsinfo);
gesture* 
make_gesture_array(uint size);
void 
delete_gesture_array(uint size,gesture* ga,bool delete_points_p);

Stroke*
concatenate_Strokes(int nstrokes1,
			Stroke* strokes1,
			int nstrokes2,
			Stroke* strokes2,
			int* nstrokes3,
			Stroke** strokes3);

rec_alternative* initialize_rec_alternative(rec_alternative* ra, uint);

rec_element* initialize_rec_element(rec_element*, char, uint, void*, rec_confidence);

/*
 * Pathnames, etc.
*/

#define REC_DEFAULT_LOCALE  	"C"
#define RECHOME			"RECHOME"
#define LANG			"LANG"
