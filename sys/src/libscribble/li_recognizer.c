/*
 *  li_recognizer.c
 *
 *	Copyright 2000 Compaq Computer Corporation.
 *	Copying or modifying this code for any purpose is permitted,
 *	provided that this copyright notice is preserved in its entirety
 *	in all copies or modifications.
 *	COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR
 *	IMPLIED, AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR
 *
 *
 *  Adapted from cmu_recognizer.c by Jay Kistler.
 *  
 *  Where is the CMU copyright???? Gotta track it down - Jim Gettys
 *
 *  Credit to Dean Rubine, Jim Kempf, and Ari Rapkin.
 */

#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <scribble.h>
#include "scribbleimpl.h"

#include "hre_internal.h"
#include "li_recognizer_internal.h"

int lidebug = 0;

#define LI_MAGIC 0xACCBADDD

#define CHECK_LI_MAGIC(_a) \
  ((_a) != nil && ((li_recognizer*)(_a))->li_magic == LI_MAGIC)


static void lialg_initialize(rClassifier *);
static int lialg_read_classifier_digest(rClassifier *);
static int lialg_canonicalize_examples(rClassifier *);
static char *lialg_recognize_stroke(rClassifier *, point_list *);
static void lialg_compute_lpf_parameters(void);


char* li_err_msg = nil;

#define bcopy(s1,s2,n) memmove(s2,s1,n)

/*Freeing classifier*/

static void
free_rClassifier(rClassifier* rc);

/*
 * Point List Support
*/

static point_list*
add_example(point_list* l,int npts,pen_point* pts)
{
	pen_point* lpts = mallocz(npts*sizeof(pen_point), 1);
	point_list *p = malloc(sizeof(point_list));

	p->npts = npts;
	p->pts = lpts;
	p->next = l;       /*Order doesn't matter, so we stick on end.*/

	/*Copy points.*/

	bcopy(pts, lpts, npts * sizeof(pen_point));

	return(p);
}
	

static void
delete_examples(point_list* l)
{
	point_list* p;

	for( ; l != nil; l = p ) {
		p = l->next;
		free(l->pts);
		free(l);
	}
}

/*
 * Local functions
 */

/*
 * recognize_internal-Form Vector, use Classifier to classify, return char.
 */

static char*
recognize_internal(rClassifier* rec, Stroke* str, int*)
{
	char *res;
	point_list *stroke;

	stroke = add_example(nil, str->npts, str->pts);
	if (stroke == nil) return(nil);

	res = lialg_recognize_stroke(rec, stroke);

	delete_examples(stroke);
	return(res);
}

/*
 * file_path-Construct pathname, check for proper extension.
 */

static int
  file_path(char* dir,char* filename,char* pathname)
{
	char* dot;
	
	/*Check for proper extension on file name.*/
	
	dot = strrchr(filename,'.');
	
	if( dot == nil ) {
		return(-1);
	}

	/*Determine whether a gesture or character classifier.*/

	if( strcmp(dot,LI_CLASSIFIER_EXTENSION) != 0 ) {
		return(-1);
	}

	/*Concatenate directory and filename into pathname.*/
	
	strcpy(pathname,dir);
	strcat(pathname,"/");
	strcat(pathname,filename);
	
	return(0);
}

/*read_classifier_points-Read points so classifier can be extended.*/

static int 
read_classifier_points(FILE* fd,int nclss,point_list** ex,char** cnames)
{
	int i,j,k;
	char buf[BUFSIZ];
	int nex = 0;
	char* names[MAXSCLASSES];
	point_list* examples[MAXSCLASSES];
	pen_point* pts;
	int npts;

	/*Initialize*/

		for( i = 0; i < MAXSCLASSES; i++ ) {
				names[i] = nil;
				examples[i] = nil;
		}

		/*Go thru classes.*/

		for( k = 0; k < nclss; k++ ) {

				/*Read class name and number of examples.*/
		
				if( fscanf(fd,"%d %s",&nex,buf) != 2 )
					goto unallocate;
		
				/*Save class name*/
		
				names[k] = strdup(buf);
		
				/*Read examples.*/
				
				for( i = 0; i < nex; i++ ) {
					
					/*Read number of points.*/
					
					if( fscanf(fd,"%d",&npts) != 1 )
								goto unallocate; /*Boy would I like exceptions!*/
					
					/*Allocate array for points.*/
					
					if( (pts = mallocz(npts*sizeof(pen_point), 1)) == nil )
								goto unallocate;
					
					/*Read in points.*/
					
					for( j = 0; j < npts; j++ ) {
								int x,y;
								if( fscanf(fd,"%d %d",&x,&y) != 2 ) {
									delete_pen_point_array(pts);
									goto unallocate;
								}
								pts[j].Point = Pt(x, y);
					}
					
					/*Add example*/
					
					if( (examples[k] = add_example(examples[k],npts,pts)) == nil ) {
								delete_pen_point_array(pts);
								goto unallocate;
					}
					
					delete_pen_point_array(pts);
			
				}
		}

/* ari -- end of list of classes */
/* fprint(2, "]\n"); */

		/*Transfer to recognizer.*/

		bcopy(examples,ex,sizeof(examples));
		bcopy(names,cnames,sizeof(names));

		return(0);

		/*Error. Deallocate memory and return.*/

unallocate:
		for( ; k >= 0; k-- ) {
				delete_examples(examples[k]);
				free(names[k]);
		}

		return(-1);
}

/*read_classifier-Read a classifier file.*/

static int read_classifier(FILE* fd,rClassifier* rc)
{
	
	li_err_msg = nil;

	/*Read in classifier file.*/

	if(fscanf(fd, "%d", &rc->nclasses) != 1)
		return -1;

	/*Read in the example points, so classifier can be extended.*/

	if( read_classifier_points(fd,rc->nclasses,rc->ex,rc->cnames) != 0 )
		return -1;

	return(0);
}

/*
 * Extension Functions
*/

/* getClasses and clearState are by Ari */

static int
recognizer_getClasses (recognizer r, char ***list, int *nc)
{
	int i, nclasses;
	li_recognizer* rec;
	char **ret;

		rec = (li_recognizer*)r->recognizer_specific;

		/*Check for LI recognizer.*/

		if( !CHECK_LI_MAGIC(rec) ) {
				li_err_msg = "Not a LI recognizer";
				return(-1);
	}
	
	*nc = nclasses = rec->li_rc.nclasses;
	ret = malloc(nclasses*sizeof(char*));

	for (i = 0; i < nclasses; i++)
				ret[i] = rec->li_rc.cnames[i];   /* only the 1st char of the cname */
	*list = ret;
		return 0;
}

static int
recognizer_clearState (recognizer)
{
  /*This operation isn't supported by the LI recognizer.*/

  li_err_msg = "Clearing state is not supported by the LI recognizer";

  return(-1);
}

static bool isa_li(recognizer r)
{ return(CHECK_LI_MAGIC(r)); }

static int
recognizer_train(recognizer, rc*, uint, Stroke*, rec_element*, bool)
{
  /*This operation isn't supported by the LI recognizer.*/

  li_err_msg = "Training is not supported by the LI recognizer";

  return(-1);
}

int
li_recognizer_get_example (recognizer r,
						   int		class, 
						   int		instance,
						   char		**name, 
						   pen_point	**points,
						   int		*npts)
{
	li_recognizer   *rec = (li_recognizer*)r->recognizer_specific;
	int nclasses = rec->li_rc.nclasses;
	point_list	    *pl;
	
		if( !CHECK_LI_MAGIC(rec) ) {
				li_err_msg = "Not a LI recognizer";
				return(-1);
		}
		if (class > nclasses)
				return -1;
		pl = rec->li_rc.canonex[class];
		while (instance && pl)
		{
				pl = pl->next;
				instance--;
		}
		if (!pl)
				return -1;
		*name = rec->li_rc.cnames[class];
		*points = pl->pts;
		*npts = pl->npts;
		return pl->npts; /* I hope [sjm] */
}

/*
 * API Functions
 */


/*li_recognizer_load-Load a classifier file.*/

static int li_recognizer_load(recognizer r, char* dir, char* filename)
{
		FILE *fd;
		char* pathname;
		li_recognizer* rec;
		rClassifier* rc;
		
		rec = (li_recognizer*)r->recognizer_specific;

		/*Make sure recognizer's OK*/

		if( !CHECK_LI_MAGIC(rec) ) {
				li_err_msg = "Not a LI recognizer";
				return(-1);
		}

		rc = &(rec->li_rc);

		/*Check parameters.*/

		if( filename == nil ) {
				li_err_msg = "Invalid parameters";
				return(-1);
		}

		/*We let the directory be null.*/

		if( dir == nil || (int)strlen(dir) <= 0 ) {
				dir = ".";
		}

		if(0)fprint(2, "dir = %s, filename = %s\n",
				dir, filename);

		/*Make full pathname and check filename*/

		pathname = malloc((strlen(dir) + strlen(filename) + 2)*sizeof(char));
		if( file_path(dir, filename, pathname) == -1 ) {
				free(pathname);
				li_err_msg = "Not a LI recognizer classifier file";
				return -1;
		}

		/* Try to short-circuit the full classifier-file processing. */
		rc->file_name = pathname;
		if (lialg_read_classifier_digest(rc) == 0)
				return(0);
		rc->file_name = nil;

		/*Open the file*/

		if( (fd = fopen(pathname,"r")) == nil ) {
				li_err_msg = "Can't open classifier file";
				if(0)fprint(2, "Can't open %s.\n", pathname);
				free(pathname);
				return(-1);
		}

		/*If rClassifier is OK, then delete it first.*/

		if( rc->file_name != nil ) {
				free_rClassifier(rc);
		}

		/*Read classifier.*/
		
		if( read_classifier(fd,rc) < 0 ) {
				free(pathname);
				return(-1);
		}

		/*Close file.*/

		fclose(fd);

		/*Add classifier name.*/

		rc->file_name = pathname;

		/* Canonicalize examples. */
		if (lialg_canonicalize_examples(rc) != 0) {
				free(pathname);
				rc->file_name = nil;
				return -1;
		}

		return(0);
}

/*li_recognizer_save-Save a classifier file.*/

static int li_recognizer_save(recognizer, char*, char*)
{ 
		/*This operation isn't supported by the LI recognizer.*/

		li_err_msg = "Saving is not supported by the LI recognizer";
		return -1;
}

static wordset
li_recognizer_load_dictionary(recognizer, char*, char*)
{
		/*This operation isn't supported by the LI recognizer.*/

		li_err_msg = "Dictionaries are not supported by the LI recognizer";
		return nil;
}

static int
li_recognizer_save_dictionary(recognizer, char*, char*, wordset)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Dictionaries are not supported by the LI recognizer";
		return -1;
}

static int
li_recognizer_free_dictionary(recognizer, wordset)
{
  /*This operation isn't supported by the LI recognizer.*/

  li_err_msg = "Dictionaries are not supported by the LI recognizer";

  return -1;

}

static int
li_recognizer_add_to_dictionary(recognizer, letterset*, wordset)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Dictionaries are not supported by the LI recognizer";
		return -1;
}

static int
li_recognizer_delete_from_dictionary(recognizer, letterset*, wordset)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Dictionaries are not supported by the LI recognizer";
		return -1;
}

static char*
li_recognizer_error(recognizer rec)
{
	char* ret = li_err_msg;

	/*Check for LI recognizer.*/

	if( !CHECK_LI_MAGIC(rec->recognizer_specific) ) {
				li_err_msg = "Not a LI recognizer";
				return nil;
	}
	li_err_msg = nil;
	return ret;
}

static int 
li_recognizer_clear(recognizer r, bool)
{
		li_recognizer* rec; 

		rec = (li_recognizer*)r->recognizer_specific;
		/*Check for LI recognizer.*/
		if( !CHECK_LI_MAGIC(rec) ) {
				li_err_msg = "Not a LI recognizer";
				return 0;
		}
		return 0;
}

static int 
li_recognizer_set_context(recognizer, rc*)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Contexts are not supported by the LI recognizer";
		return -1;
}

static rc*
li_recognizer_get_context(recognizer)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Contexts are not supported by the LI recognizer";
		return nil;
}

static int 
li_recognizer_get_buffer(recognizer, uint*, Stroke**)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Buffer get/set are not supported by the LI recognizer";
		return -1;
}

static int 
li_recognizer_set_buffer(recognizer, uint, Stroke*)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Buffer get/set are not supported by the LI recognizer";
		return -1;
}

static int
li_recognizer_translate(recognizer r, uint ncs, Stroke* tps, bool, int* nret, rec_alternative** ret)
{
	char* clss;
	li_recognizer* rec; 
	int conf;
	rClassifier* rc;
	  
	rec = (li_recognizer*)r->recognizer_specific;

	*nret = 0;
	*ret = nil;

	/*Check for LI recognizer.*/

	if( !CHECK_LI_MAGIC(rec) ) {
				li_err_msg = "Not a LI recognizer";
				return(-1);
	}

		rc = &(rec->li_rc);

	/*Check for valid parameters.*/
	if (ncs < 1) {
				li_err_msg = "Invalid parameters: ncs";
				return(-1);
	}
	if( tps == nil) {
				li_err_msg = "Invalid parameters: tps";
				return(-1);
	}
	if( nret == nil) {
				li_err_msg = "Invalid parameters: nret";
				return(-1);
	}
	if( ret == nil) {
				li_err_msg = "Invalid parameters: ret";
				return(-1);
	}

	/*
	 * Go through the stroke array and recognize. Since this is a single
	 *   stroke recognizer, each stroke is treated as a separate
	 *   character or gesture. We allow only characters or gestures
	 *   to be recognized at one time, since otherwise, handling
	 *   the display of segmentation would be difficult. 
	*/
	clss = recognize_internal(rc,tps,&conf);
	if (clss == nil) {
				*nret = 1;
				return(0);
	}

	/*Return values.*/
	*nret = 1;
	return(*clss);
}


static rec_fn*
li_recognizer_get_extension_functions(recognizer rec)
{
	rec_fn* ret;

	/*Check for LI recognizer.*/

	if( !CHECK_LI_MAGIC(rec->recognizer_specific) ) {
				li_err_msg = "Not a LI recognizer";
				return(nil);
	}

	ret = make_rec_fn_array(LI_NUM_EX_FNS);

/* ari -- clearState & getClasses are mine */
	ret[LI_GET_CLASSES] =	(rec_fn)recognizer_getClasses;
	ret[LI_CLEAR] =			(rec_fn)recognizer_clearState;
	ret[LI_ISA_LI] =		(rec_fn)isa_li;
	ret[LI_TRAIN] =			(rec_fn)recognizer_train;

	return(ret);
}

static char**
li_recognizer_get_gesture_names(recognizer)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Gestures are not supported by the LI recognizer";
		return nil;
}

static xgesture
li_recognizer_set_gesture_action(recognizer, char*, xgesture, void*)
{
		/*This operation isn't supported by the LI recognizer.*/
		li_err_msg = "Gestures are not supported by the LI recognizer";
		return nil;
}

/*
 * Exported Functions
*/

/*RECOGNIZER_INITIALIZE-Initialize the recognizer.*/

/* note from ari:  this expands via pre-processor to
 *
 * recognizer __recognizer_internal_initialize(rec_info* ri)
 */

RECOGNIZER_INITIALIZE(ri)
{
	recognizer r;
	li_recognizer* rec;
	int i;

	/*Check that locale matches.*/

	if( strcmp(ri->ri_locale,LI_SUPPORTED_LOCALE) != 0 ) {
		li_err_msg = "Not a supported locale";
/* fprint(2, "Locale error.\n");*/
		return(nil);
	}

	/*
	 * Check that character sets match. Note that this is only approximate,
	 * since the classifier file will have more information.
	*/

	if( ri->ri_subset != nil ) {
	  for(i = 0; ri->ri_subset[i] != nil; i++ ) {

		if( strcmp(ri->ri_subset[i],UPPERCASE) != 0 &&
			strcmp(ri->ri_subset[i],LOWERCASE) != 0  &&
			strcmp(ri->ri_subset[i],DIGITS) != 0 &&
			strcmp(ri->ri_subset[i],GESTURE) != 0 ) {
		  li_err_msg = "Not a supported character set";
/* fprint(2, "charset error.\n"); */

		  return(nil);
		}
	  }
	}
			 
/* ari */
	r = make_recognizer(ri);
/* fprint(2, "past make_recognizer.\n"); */

	if( r == nil ) {
		li_err_msg = "Can't allocate storage";

		return(nil);
	}

	/*Make a LI recognizer structure.*/


	/* rec = (li_recognizer*)safe_malloc(sizeof(li_recognizer))) == nil ); */

	rec = malloc(sizeof(li_recognizer));

	r->recognizer_specific = rec;

	rec->li_rc.file_name = nil;
	rec->li_rc.nclasses = 0;

	/*Initialize the recognizer struct.*/

	r->recognizer_load_state = li_recognizer_load;
	r->recognizer_save_state = li_recognizer_save;
	r->recognizer_load_dictionary = li_recognizer_load_dictionary;
	r->recognizer_save_dictionary = li_recognizer_save_dictionary;
	r->recognizer_free_dictionary = li_recognizer_free_dictionary;
	r->recognizer_add_to_dictionary = li_recognizer_add_to_dictionary;
	r->recognizer_delete_from_dictionary = li_recognizer_delete_from_dictionary;
	r->recognizer_error = li_recognizer_error;
	r->recognizer_translate = li_recognizer_translate;
	r->recognizer_get_context = li_recognizer_get_context;
	r->recognizer_set_context = li_recognizer_set_context;
	r->recognizer_get_buffer = li_recognizer_get_buffer;
	r->recognizer_set_buffer = li_recognizer_set_buffer;
	r->recognizer_clear = li_recognizer_clear;
	r->recognizer_get_extension_functions = 
	  li_recognizer_get_extension_functions;
	r->recognizer_get_gesture_names = li_recognizer_get_gesture_names;
	r->recognizer_set_gesture_action = 
	  li_recognizer_set_gesture_action;

	/*Initialize LI Magic Number.*/

	rec->li_magic = LI_MAGIC;

	/*Initialize rClassifier.*/

	rec->li_rc.file_name = nil;

	for( i = 0; i < MAXSCLASSES; i++ ) {
		rec->li_rc.ex[i] = nil;
		rec->li_rc.cnames[i] = nil;
	}

	lialg_initialize(&rec->li_rc);

	/*Get rid of error message. Not needed here.*/
	li_err_msg = nil;

	return(r);
}

/*free_rClassifier-Free the rClassifier.*/

static void
free_rClassifier(rClassifier* rc)
{
	int i;

	if( rc->file_name != nil) {
		free(rc->file_name);
	}

	for( i = 0; rc->ex[i] != nil; i++) {
		delete_examples(rc->ex[i]);
		free(rc->cnames[i]);
	}

}

/*RECOGNIZER_FINALIZE-Deallocate the recognizer, finalize.*/

RECOGNIZER_FINALIZE(r)
{
		li_recognizer* rec = (li_recognizer*)r->recognizer_specific;

		/*Make sure this is a li_recognizer first*/
		if( !CHECK_LI_MAGIC(rec) ) {
				li_err_msg = "Not a LI recognizer";
				return(-1);
	}

		/*Deallocate rClassifier resources.*/

		free_rClassifier(&(rec->li_rc));

		/*Deallocate the li_recognizer struct.*/
		free(rec);

		/*Deallocate the recognizer*/
		delete_recognizer(r);

		return(0);
}


/* **************************************************

  Implementation of the Li/Yeung recognition algorithm

************************************************** */

#define	WORST_SCORE	0x7fffffff

/* Dynamic programming parameters */
#define	DP_BAND		3
#define	MIN_SIM		0
#define	MAX_DIST	0x7fffffff
#define	SIM_THLD	60	/* x 100 */
#define	DIST_THLD	3200	/* x 100 */

/* Low-pass filter parameters -- empirically derived */
#define	LP_FILTER_WIDTH	6
#define	LP_FILTER_ITERS	8
#define	LP_FILTER_THLD	250	/* x 100 */
#define	LP_FILTER_MIN	5

/* Pseudo-extrema parameters -- empirically derived */
#define	PE_AL_THLD	1500	/* x 100 */
#define	PE_ATCR_THLD	135	/* x 100 */

/* Contour-angle derivation parameters */
#define	T_ONE		1
#define	T_TWO		20

/* Pre-processing and canonicalization parameters */
#define	CANONICAL_X	108
#define	CANONICAL_Y	128
#define	DIST_SQ_THRESHOLD   (3*3)	/* copied from fv.h */
#define	NCANONICAL	50

/* Tap-handling parameters */
#define	TAP_CHAR	"."
#define	TAP_TIME_THLD	150	    /* msec */
#define	TAP_DIST_THLD	75	    /* dx * dx + dy * dy */
#define	TAP_PATHLEN	1000	    /* x 100 */


/* region types */
#define	RGN_CONVEX  0
#define	RGN_CONCAVE 1
#define	RGN_PLAIN   2
#define	RGN_PSEUDO  3


typedef struct RegionList {
	int start;
	int end;
	int type;
	struct RegionList *next;
} region_list;


/* direction-code table; indexed by dx, dy */
static int lialg_dctbl[3][3] = {{1, 0, 7}, {2, 0x7FFFFFFF, 6}, {3, 4, 5}};

/* low-pass filter weights */
static int lialg_lpfwts[2 * LP_FILTER_WIDTH + 1];
static int lialg_lpfconst = -1;


static int lialg_preprocess_stroke(point_list *);
static point_list *lialg_compute_dominant_points(point_list *);
static point_list *lialg_interpolate_points(point_list *);
static void lialg_bresline(pen_point *, pen_point *, point_list *, int *);
static void lialg_compute_chain_code(point_list *);
static void lialg_compute_unit_chain_code(point_list *);
static region_list *lialg_compute_regions(point_list *);
static point_list *lialg_compute_dompts(point_list *, region_list *);
static int *lialg_compute_contour_angle_set(point_list *, region_list *);
static void lialg_score_stroke(point_list *, point_list *, int *, int *);
static int lialg_compute_similarity(point_list *, point_list *);
static int lialg_compute_distance(point_list *, point_list *);

static int lialg_read_classifier_digest(rClassifier *);

static int lialg_canonicalize_examples(rClassifier *);
static int lialg_canonicalize_example_stroke(point_list *);
static int lialg_compute_equipoints(point_list *);

static int lialg_compute_pathlen(point_list *);
static int lialg_compute_pathlen_subset(point_list *, int, int);
static int lialg_filter_points(point_list *);
static int lialg_translate_points(point_list *, int, int, int, int);
static void lialg_get_bounding_box(point_list *, int *, int *, int *, int *);
static void lialg_compute_lpf_parameters();
static int isqrt(int);
static int likeatan(int, int);
static int quadr(int);


/*************************************************************

  Core routines for the Li/Yeung recognition algorithm

 *************************************************************/

static void lialg_initialize(rClassifier *rec) {
	int i;

	/* Initialize the dompts arrays. */
	for (i = 0; i < MAXSCLASSES; i++) {
		rec->dompts[i] = nil;
	}
}


/*
 *  Main recognition routine -- called by HRE API.
 */
static char *lialg_recognize_stroke(rClassifier *rec, point_list *stroke) {
		int i;
		char *name = nil;
		point_list *input_dompts = nil;
		char *best_name = nil;
		int best_score = WORST_SCORE;
		char *curr_name;
		point_list *curr_dompts;

		/*    (void)gettimeofday(&stv, nil);*/

		if (stroke->npts < 1) goto done;

		/* Check for tap. */

		/* First thing is to filter out ``close points.'' */
		if (lialg_filter_points(stroke) != 0) return(nil);

		/* Unfortunately, we don't have the actual time that each point */
		/* was recorded (i.e., dt is invalid).  Hence, we have to use a */
		/* heuristic based on total distance and the number of points. */
		if (stroke->npts == 1 || lialg_compute_pathlen(stroke) < TAP_PATHLEN)
			return(TAP_CHAR);

		/* Pre-process input stroke. */
		if (lialg_preprocess_stroke(stroke) != 0) goto done;

		/* Compute its dominant points. */
		input_dompts = lialg_compute_dominant_points(stroke);
		if (input_dompts == nil) goto done;

		/* Score input stroke against every class in classifier. */
		for (i = 0, curr_name = rec->cnames[i], curr_dompts = rec->dompts[i];
				i < MAXSCLASSES && curr_name != nil && curr_dompts != nil;
				i++, curr_name = rec->cnames[i], curr_dompts = rec->dompts[i]) {
				int sim;
				int dist;
				int curr_score;

				lialg_score_stroke(input_dompts, curr_dompts, &sim, &dist);
				curr_score = dist;

				if (lidebug && curr_score < DIST_THLD)
				fprint(2, "(%s, %d, %d) ", curr_name, sim, dist);

				/* Is it the best so far? */
				if (curr_score < best_score && curr_score <= DIST_THLD) {
				best_score = curr_score;
				best_name = curr_name;
				}
		}

		if (lidebug)
				fprint(2, "\n");

		/* No errors. */
		name = best_name;

done:
		delete_examples(input_dompts);
		return(name);
}


static int lialg_preprocess_stroke(point_list *points) {
	int minx, miny, maxx, maxy, xrange, yrange, scale, xoff, yoff;

	/* Filter out points that are too close. */
	/* We did this earlier, when we checked for a tap. */
/*
	if (lialg_filter_points(points) != 0) return(-1);
*/

/*    assert(points->npts > 0);*/

	/* Scale up to avoid conversion errors. */
	lialg_get_bounding_box(points, &minx, &miny, &maxx, &maxy);
	xrange = maxx - minx;
	yrange = maxy - miny;
	scale = ( ((100 * xrange + CANONICAL_X / 2) / CANONICAL_X) > 
			  ((100 * yrange + CANONICAL_Y / 2) / CANONICAL_Y))
	  ? (100 * CANONICAL_X + xrange / 2) / xrange
	  : (100 * CANONICAL_Y + yrange / 2) / yrange;
	if (lialg_translate_points(points, minx, miny, scale, scale) != 0)
		return(-1);

	/* Center the stroke. */
	lialg_get_bounding_box(points, &minx, &miny, &maxx, &maxy);
	xrange = maxx - minx;
	yrange = maxy - miny;
	xoff = -((CANONICAL_X - xrange + 1) / 2);
	yoff = -((CANONICAL_Y - yrange + 1) / 2);
	if (lialg_translate_points(points, xoff, yoff, 100, 100) != 0) return(-1);

	/* Store the x and y ranges in the point list. */
	xrange = maxx - minx;
	yrange = maxy - miny;
	points->xrange = xrange;
	points->yrange = yrange;

	if (lidebug) {
		int i;
		fprint(2, "After pre-processing:   %d %d %d %d\n",
				minx, miny, maxx, maxy);
		for (i = 0; i < points->npts; i++)
			fprint(2, "      (%P)\n", points->pts[i].Point);
		fflush(stderr);
	}

	return(0);
}


static point_list *lialg_compute_dominant_points(point_list *points) {
	point_list *ipts;
	region_list *regions;
	point_list *dpts;

	/* Interpolate points. */
	ipts = lialg_interpolate_points(points);
	if (ipts == nil) return(nil);
	if (lidebug) {
				int j;
				fprint(2, "After interpolation:  %d ipts\n", ipts->npts);
				for (j = 0; j < ipts->npts; j++) {
					fprint(2, "  (%P), %lud\n", ipts->pts[j].Point, ipts->pts[j].chaincode);
				}
				fflush(stderr);
	}

	/* Compute regions. */
	regions = lialg_compute_regions(ipts);
/*    assert(regions != nil);*/

	/* Compute dominant points. */
	dpts = lialg_compute_dompts(ipts, regions);
	if (lidebug) {
				int j;
				fprint(2, "Dominant points:  ");
				for (j = 0; j < dpts->npts; j++) {
					fprint(2, "%P (%lud)  ", dpts->pts[j].Point, dpts->pts[j].chaincode);
				}
				fprint(2, "\n");
				fflush(stderr);
	}

	/* Delete region data structure. */
	{
				region_list *curr, *next;
				for (curr = regions; curr != nil; ) {
					next = curr->next;
					free(curr);
					curr = next;
				}
	}
	delete_examples(ipts);
	return(dpts);
}

/* Input points are assumed to be integer-valued! */
static point_list *lialg_interpolate_points(point_list *points) {
	int i, j;
	int maxpts;
	point_list *newpts;

	/* Compute an upper-bound on the number of interpolated points. */
	maxpts = 0;
	for (i = 0; i < (points->npts - 1); i++) {
				pen_point *pta = &(points->pts[i]);
				pen_point *ptb = &(points->pts[i+1]);
				maxpts += abs(pta->x - ptb->x) + abs(pta->y - ptb->y);
	}

	/* Allocate an array of the requisite size. */
	maxpts += points->npts;
	/* newpts = (point_list *)safe_malloc(sizeof(point_list)); */
	newpts = malloc(sizeof(point_list));
	newpts->pts = mallocz(maxpts*sizeof(pen_point), 1);
	if (newpts->pts == nil) {
				free(newpts);
				return(nil);
	}
	newpts->npts = maxpts;
	newpts->next = nil;

	/* Interpolate each of the segments. */
	j = 0;
	for (i = 0; i < (points->npts - 1); i++) {
				pen_point *startpt = &(points->pts[i]);
				pen_point *endpt = &(points->pts[i+1]);

				lialg_bresline(startpt, endpt, newpts, &j);

				j--;	/* end point gets recorded as start point of next segment! */
	}

	/* Add-in last point. */
	newpts->pts[j++] = points->pts[points->npts - 1];
	newpts->npts = j;

	/* Compute the chain code for P (the list of points). */
	lialg_compute_unit_chain_code(newpts);

	return(newpts);
}


/* This implementation is due to Kenny Hoff. */
static void lialg_bresline(pen_point *startpt, pen_point *endpt,
							point_list *newpts, int *j) {
	int Ax, Ay, Bx, By, dX, dY, Xincr, Yincr;

	Ax = startpt->x;
	Ay = startpt->y;
	Bx = endpt->x;
	By = endpt->y;

	/* INITIALIZE THE COMPONENTS OF THE ALGORITHM THAT ARE NOT AFFECTED */
	/* BY THE SLOPE OR DIRECTION OF THE	LINE */
	dX = abs(Bx-Ax);	/* store the change in X and Y of the line endpoints */
	dY = abs(By-Ay);

	/* DETERMINE "DIRECTIONS" TO INCREMENT X AND Y (REGARDLESS OF DECISION) */
	if (Ax > Bx) { Xincr=-1; } else { Xincr=1; }    /* which direction in X? */
	if (Ay > By) { Yincr=-1; } else { Yincr=1; }    /* which direction in Y? */

	/* DETERMINE INDEPENDENT VARIABLE (ONE THAT ALWAYS INCREMENTS BY 1 (OR -1) ) */
	/* AND INITIATE APPROPRIATE LINE DRAWING ROUTINE (BASED ON FIRST OCTANT */
	/* ALWAYS). THE X AND Y'S MAY BE FLIPPED IF Y IS THE INDEPENDENT VARIABLE. */
	if (dX >= dY) {	    /* if X is the independent variable */
				int dPr	= dY<<1;	    /* amount to increment decision if right is chosen (always) */
				int dPru = dPr - (dX<<1);   /* amount to increment decision if up is chosen */
				int P =	dPr - dX;	    /* decision variable start value */
		
				/* process each point in the line one at a time (just use dX) */
				for (; dX>=0; dX--) {
					newpts->pts[*j].x = Ax;
					newpts->pts[*j].y = Ay;
					(*j)++;
		
					if (P > 0) {	/* is the pixel	going right AND	up? */
								Ax+=Xincr;	/* increment independent variable */
								Ay+=Yincr;	/* increment dependent variable */
								P+=dPru;	/* increment decision (for up) */
					} else {		/* is the pixel just going right? */
								Ax+=Xincr;	/* increment independent variable */
								P+=dPr;		/* increment decision (for right) */
					}
				}
	} else {		    /* if Y is the independent variable */
				int dPr	= dX<<1;	    /* amount to increment decision if right is chosen (always) */
				int dPru = dPr - (dY<<1);   /* amount to increment decision if up is chosen */
				int P  = dPr - dY;	    /* decision variable start value */
		
				/* process each point in the line one at a time (just use dY) */
				for (; dY>=0; dY--) {
					newpts->pts[*j].x = Ax;
					newpts->pts[*j].y = Ay;
					(*j)++;
		
					if (P > 0) {	/* is the pixel going up AND right? */
								Ax+=Xincr;	/* increment dependent variable */
								Ay+=Yincr;	/* increment independent variable */
								P+=dPru;	/* increment decision (for up) */
					} else {		/* is the pixel just going up? */
								Ay+=Yincr;	/* increment independent variable */
								P+=dPr;		/* increment decision (for right) */
					}
				}
	}
}

static void lialg_compute_chain_code(point_list *pts) {
	int i;

	for (i = 0; i < (pts->npts - 1); i++) {
				pen_point *startpt = &(pts->pts[i]);
				pen_point *endpt = &(pts->pts[i+1]);
				int dx = endpt->x - startpt->x;
				int dy = endpt->y - startpt->y;
				int tmp = quadr(likeatan(dy, dx));
				int dircode = (12 - tmp) % 8;
		
				startpt->chaincode = dircode;
	}
}


static void lialg_compute_unit_chain_code(point_list *pts) {
	int i;

	for (i = 0; i < (pts->npts - 1); i++) {
				pen_point *startpt = &(pts->pts[i]);
				pen_point *endpt = &(pts->pts[i+1]);
				int dx = endpt->x - startpt->x;
				int dy = endpt->y - startpt->y;
				int dircode = lialg_dctbl[dx+1][dy+1];

				startpt->chaincode = dircode;
	}
}


static region_list *lialg_compute_regions(point_list *pts) {
		region_list *regions;
		region_list *curr_reg;
		int *R[2 + LP_FILTER_ITERS];
		int *junk;
		int *curr, *next;
		int i, j;

		/* Initialize low-pass filter parameters if necessary. */
		if (lialg_lpfconst == -1)
				lialg_compute_lpf_parameters();

	/* Allocate a 2 x pts->npts array for use in computing the (filtered) Angle set, A_n. */
	junk = malloc((2 + LP_FILTER_ITERS) * pts->npts*sizeof(int));
	for (i = 0; i < (2 + LP_FILTER_ITERS); i++)
		R[i] = junk + (i * pts->npts);
	curr = R[0];

	/* Compute the Angle set, A, in the first element of array R. */
	/* Values in R are in degrees, x 100. */
	curr[0] = 18000;				/* a_0 */
	for (i = 1; i < (pts->npts - 1); i++) {
				int d_i = pts->pts[i].chaincode;
				int d_iminusone = pts->pts[i-1].chaincode;
				int a_i;
		
				if (d_iminusone < d_i)
					d_iminusone += 8;
		
				a_i = (d_iminusone - d_i) % 8;
		
				/* convert to degrees, x 100 */
				curr[i] = ((12 - a_i) % 8) * 45 * 100;
	}
	curr[pts->npts - 1]	= 18000;		/* a_L-1 */

	/* Perform a number of filtering iterations. */
	next = R[1];
	for (j = 0; j < LP_FILTER_ITERS; j++, curr = R[j], next = R[j+1]) {
				for (i = 0; i < pts->npts; i++) {
					int k;
		
					next[i] = 0;
		
					for (k = i - LP_FILTER_WIDTH; k <= i + LP_FILTER_WIDTH; k++) {
						int oldval = (k < 0 || k >= pts->npts) ? 18000 : curr[k];
						next[i]	+= oldval * lialg_lpfwts[k - (i	- LP_FILTER_WIDTH)];	/* overflow? */
					}
		
					next[i] /= lialg_lpfconst;
				}
	}

	/* Do final thresholding around PI. */
	/* curr and next are set-up correctly at end of previous loop! */
	for (i = 0; i < pts->npts; i++)
				next[i] = (abs(curr[i] - 18000) < LP_FILTER_THLD) ? 18000 : curr[i];
	curr = next;

	/* Debugging. */
	if (lidebug > 1) {
				for (i = 0; i < pts->npts; i++) {
					fprint(2, "%3d:  (%P)  %lud  ",
								i, pts->pts[i].Point, pts->pts[i].chaincode);
					for (j = 0; j < 2 + LP_FILTER_ITERS; j++)
						fprint(2, "%d  ", R[j][i]);
					fprint(2, "\n");
				}
	}

	/* Do the region segmentation. */
	{
				int start, end;
				int currtype;
		
#define	RGN_TYPE(val) (((val)==18000)?RGN_PLAIN:((val)<18000?RGN_CONCAVE:RGN_CONVEX))
		
				start = 0;
				currtype = RGN_TYPE(curr[0]);
				regions = malloc(sizeof(region_list));
				curr_reg = regions;
				curr_reg->start = start;
				curr_reg->end = 0;
				curr_reg->type = currtype;
				curr_reg->next = nil;
				for (i = 1; i < pts->npts; i++) {
					int nexttype = RGN_TYPE(curr[i]);
		
					if (nexttype != currtype) {
								region_list *next_reg;
				
								end = i - 1;
								curr_reg->end = end;
								if (lidebug > 1)
									fprint(2, "  (%d, %d), %d\n", start, end, currtype);
				
								start = i;
								currtype = nexttype;
								next_reg = malloc(sizeof(region_list));
								next_reg->start = start;
								next_reg->end = 0;
								next_reg->type = nexttype;
								next_reg->next = nil;
				
								curr_reg->next = next_reg;
								curr_reg = next_reg;
					}
				}
				end = i - 1;
				curr_reg->end = end;
				if (lidebug > 1)
					fprint(2, "  (%d, %d), %d\n", start, end, currtype);

				/* Filter out convex/concave regions that are too short. */
				for (curr_reg = regions; curr_reg; curr_reg = curr_reg->next)
					if (curr_reg->type == RGN_PLAIN) {
								region_list *next_reg;
				
								for (next_reg = curr_reg->next;
									 next_reg != nil &&
									   (next_reg->end - next_reg->start) < LP_FILTER_MIN;
									 next_reg = curr_reg->next) {
									/* next_reg must not be plain, and it must be followed by a plain */
									/* assert(next_reg->type != RGN_PLAIN); */
									/* assert(next_reg->next != nil && (next_reg->next)->type == RGN_PLAIN); */
				
									curr_reg->next = (next_reg->next)->next;
									curr_reg->end = (next_reg->next)->end;
				
									free(next_reg->next);
									free(next_reg);
								}
					}
		
				/* Add-in pseudo-extremes. */
				{
					region_list *tmp, *prev_reg;
		
					tmp = regions;
					regions = nil;
					prev_reg = nil;
					for (curr_reg = tmp; curr_reg; curr_reg = curr_reg->next) {
						if (curr_reg->type == RGN_PLAIN) {
							int arclen = lialg_compute_pathlen_subset(pts,
																		curr_reg->start,
																		curr_reg->end);
							int dx = pts->pts[curr_reg->end].x -
							  pts->pts[curr_reg->start].x;
							int dy = pts->pts[curr_reg->end].y -
							  pts->pts[curr_reg->start].y;
							int chordlen = isqrt(10000 * (dx * dx + dy * dy));
							int atcr = (chordlen == 0) ? 0 : (100 * arclen + chordlen / 2) / chordlen;

							if (lidebug)
								fprint(2, "%d, %d, %d\n", arclen, chordlen, atcr);
		
							/* Split region if necessary. */
							if (arclen >= PE_AL_THLD && atcr >= PE_ATCR_THLD) {
								int mid = curr_reg->start + (curr_reg->end - curr_reg->start) / 2;
								int end = curr_reg->end;
								region_list *saved_next = curr_reg->next;
		
								curr_reg->end = mid - 1;
								if (prev_reg == nil)
									regions = curr_reg;
								else
									prev_reg->next = curr_reg;
								prev_reg = curr_reg;
		
								/* curr_reg = (region_list *)safe_malloc(sizeof(region_list));*/
								curr_reg = malloc(sizeof(region_list));
								curr_reg->start = mid;
								curr_reg->end = mid;
								curr_reg->type = RGN_PSEUDO;
								curr_reg->next = nil;
								prev_reg->next = curr_reg;
								prev_reg = curr_reg;
		
								/* curr_reg = (region_list *)malloc(sizeof(region_list)); */
								curr_reg = malloc(sizeof(region_list));
								curr_reg->start = mid + 1;
								curr_reg->end = end;
								curr_reg->type = RGN_PLAIN;
								curr_reg->next = nil;
								prev_reg->next = curr_reg;
								prev_reg = curr_reg;
		
								curr_reg->next = saved_next;
								continue;
							}
						}
		
						if (prev_reg == nil)
							regions = curr_reg;
						else
							prev_reg->next = curr_reg;
						prev_reg = curr_reg;
					}
				}
		}

	free(junk);
	return(regions);
}


static point_list *lialg_compute_dompts(point_list *pts, region_list *regions) {
	point_list *dpts;
	int ndpts;
	int *cas;
	int nonplain;
	region_list *r;
		region_list *curr;
		int dp;
		int previx;
		int currix;

	/* Compute contour angle set. */
	cas = lialg_compute_contour_angle_set(pts, regions);

	/* Dominant points include:  start_pt, end_pt, extrema_of_non_plain_regions, midpts of the preceding. */
	nonplain = 0;
	for (r = regions; r != nil; r = r->next)
				if (r->type != RGN_PLAIN)
						nonplain++;
	ndpts = 2 * (2 + nonplain) - 1;
	/* dpts = (point_list *)safe_malloc(sizeof(point_list)); */
	dpts = malloc(sizeof(point_list));
	dpts->pts = mallocz(ndpts*sizeof(pen_point), 1);
	if (dpts->pts == nil) {
				free(dpts);
				return(nil);
	}
	dpts->npts = ndpts;
	dpts->next = nil;

	/* Pick out dominant points. */

		/* Record start point. */
		dp = 0;
		previx = 0;
		dpts->pts[dp++] = pts->pts[previx];

		for (curr = regions; curr != nil; curr = curr->next)
			if (curr->type != RGN_PLAIN) {
						int max_v = 0;
						int min_v = 0x7fffffff;	/* maxint */
						int max_ix = -1;
						int min_ix = -1;
						int i;
		
						for (i = curr->start; i <= curr->end; i++) {
							int v = cas[i];
							if (v > max_v) { max_v = v; max_ix = i; }
							if (v < min_v) { min_v = v; min_ix = i; }
							if (lidebug > 1)
								fprint(2, "  %d\n", v);
						}
		
						currix = (curr->type == RGN_CONVEX ? max_ix : min_ix);
		
						/* Record midpoint. */
						dpts->pts[dp++] = pts->pts[previx + (currix - previx) / 2];
		
						/* Record extreme point. */
						dpts->pts[dp++] = pts->pts[currix];
		
						previx = currix;
			}

		/* Record last mid-point and end point. */
		currix = pts->npts - 1;
		dpts->pts[dp++] = pts->pts[previx + (currix - previx) / 2];
		dpts->pts[dp] = pts->pts[currix];

	/* Compute chain-code. */
	lialg_compute_chain_code(dpts);

	free(cas);
	return(dpts);
}


static int *lialg_compute_contour_angle_set(point_list *pts,
											   region_list *regions) {
		int *V;
		region_list *curr_reg;
		int i;

		V = malloc(pts->npts*sizeof(int));

		V[0] = 18000;
		for (curr_reg = regions; curr_reg != nil; curr_reg = curr_reg->next) {
				for (i = curr_reg->start; i <= curr_reg->end; i++) {
					if (curr_reg->type == RGN_PLAIN) {
								V[i] = 18000;
					} else {
								/* For now, simply choose the mid-point. */
								int isMidPt = i == (curr_reg->start +
													 (curr_reg->end - curr_reg->start) / 2);
								V[i] = (curr_reg->type == RGN_CONVEX)
								  ? (isMidPt ? 18000 : 0)
								  : (isMidPt ? 0 : 18000);
					}
				}
	}
	V[pts->npts - 1] = 18000;

	return(V);
}


/*
 *  First compute the similarity between the two strings.
 *  If it's above a threshold, compute the distance between
 *  the two and return it as the ``score.''
 *  Otherwise, return the constant WORST_SCORE.
 *
 */
static void lialg_score_stroke(point_list *input_dompts, point_list *curr_dompts, int *sim, int *dist) {
	*sim = MIN_SIM;
	*dist = MAX_DIST;

	*sim = lialg_compute_similarity(input_dompts, curr_dompts);
	if (*sim < SIM_THLD) goto done;

	*dist = lialg_compute_distance(input_dompts, curr_dompts);

done:
	if (lidebug)
		fprint(2, "%d, %d\n", *sim, *dist);
}


static int lialg_compute_similarity(point_list *input_dompts, point_list *curr_dompts) {
	int sim;
	point_list *A, *B;
	int N, M;
	int **G;
	int *junk;
	int i, j;

	/* A is the	longer sequence, length	N. */
	/* B is the shorter sequence, length M. */
		if (input_dompts->npts >= curr_dompts->npts) {
				A = input_dompts;
				N = input_dompts->npts;
				B = curr_dompts;
				M = curr_dompts->npts;
	} else {
				A = curr_dompts;
				N = curr_dompts->npts;
				B = input_dompts;
				M = input_dompts->npts;
		}

		/* Allocate and initialize the Gain matrix, G. */
		/* The size of G is M x (N + 1). */
		/* Note that row 0 is unused. */
		/* Similarities are x 10. */
		G = malloc(M*sizeof(int *));
		junk = malloc(M * (N + 1) * sizeof(int));
		for (i = 0; i < M; i++)
			G[i] = junk + (i * (N + 1));

		for (i = 1; i < M; i++) {
			int bval = B->pts[i-1].chaincode;

			/* Source column. */
			G[i][0] = 0;

			for (j = 1; j < N; j++) {
				int aval = A->pts[j-1].chaincode;
				int diff = abs(bval - aval);
				if (diff > 4) diff = 8 - diff;

				G[i][j] = (diff == 0)
				  ? 10
				  : (diff == 1)
				  ? 6
				  : 0;
			}

			/* Sink column. */
			G[i][N] = 0;
		}

	/* Do the DP algorithm. */
	/* Proceed in column order, from highest column to the lowest. */
	/* Within each column, proceed from the highest row to the lowest. */
	/* Skip the highest column. */
		for (j = N - 1; j >= 0; j--)
			for (i = M - 1; i > 0; i--) {
				int max = G[i][j + 1];

				if (i < (M - 1)) {
					int tmp = G[i + 1][j + 1];
					if (tmp > max) max = tmp;
				}

				G[i][j] += max;
		}

		sim = (10 * G[1][0] + (N - 1) / 2) / (N - 1);

		if (G != nil)
				free(G);
		if (junk != nil)
				free(junk);
		return(sim);
}


static int lialg_compute_distance(point_list *input_dompts,
								   point_list *curr_dompts) {
		int dist;
		point_list *A, *B;
		int N, M;
		int **C;
		int *junk;
		int *BE;
		int *TE;
		int i, j;

		/* A is the	longer sequence, length	N. */
		/* B is the shorter sequence, length M. */
		if (input_dompts->npts >= curr_dompts->npts) {
				A = input_dompts;
				N = input_dompts->npts;
				B = curr_dompts;
				M = curr_dompts->npts;
		}
		else {
				A = curr_dompts;
				N = curr_dompts->npts;
				B = input_dompts;
				M = input_dompts->npts;
		}

		/* Construct the helper vectors, BE and TE, which say for each column */
		/* what are the ``bottom'' and ``top'' rows of interest. */
		BE = malloc((N + 1)*sizeof(int));
		TE = malloc((N + 1)*sizeof(int));

		for (j = 1; j <= N; j++) {
				int bot, top;

				bot = j + (M - DP_BAND);
				if (bot > M) bot = M;
				BE[j] = bot;

				top = j - (N - DP_BAND);
				if (top < 1) top = 1;
				TE[j] = top;
		}

		/* Allocate and initialize the Cost matrix, C. */
		/* The size of C is (M + 1) x (N + 1). */
		/* Note that row and column 0 are unused. */
		/* Costs are x 100. */
		/*	C = (int **)safe_malloc((M + 1) * sizeof(int *)); */
		C = malloc((M + 1)*sizeof( int *));
		junk = malloc((M + 1) * (N + 1)*sizeof(int));
		for (i = 0; i <= M; i++)
				C[i] = junk + (i * (N + 1));

		for (i = 1; i <= M; i++) {
				int bx = B->pts[i-1].x;
				int by = B->pts[i-1].y;

				for (j = 1; j <= N; j++) {
						int ax = A->pts[j-1].x;
						int ay = A->pts[j-1].y;
						int dx = bx - ax;
						int dy = by - ay;
						int dist = isqrt(10000 * (dx * dx + dy * dy));
				
						C[i][j] = dist;
				}
		}

		/* Do the DP algorithm. */
		/* Proceed in column order, from highest column to the lowest. */
		/* Within each column, proceed from the highest row to the lowest. */
		for (j = N; j > 0; j--)
				for (i = M; i > 0; i--) {
						int min = MAX_DIST;
		
						if (i > BE[j] || i < TE[j] || (j == N && i == M))
								continue;
		
						if (j < N) {
								if (i >= TE[j+1]) {
										int tmp = C[i][j+1];
										if (tmp < min)
												min = tmp;
								}
		
								if (i < M) {
										int tmp = C[i+1][j+1];
										if (tmp < min)
												min = tmp;
								}
						}
		
						if (i < BE[j]) {
								int tmp = C[i+1][j];
								if (tmp < min) min = tmp;
						}
		
						C[i][j] += min;
				}

		dist = (C[1][1] + N / 2) / N;

		if (C != nil) free(C);
		if (junk != nil) free(junk);
		if (BE != nil) free(BE);
		if (TE != nil) free(TE);
		return(dist);
}


/*************************************************************

  Digest-processing routines

 *************************************************************/

static int lialg_read_classifier_digest(rClassifier *rec) {
	int nclasses;
	FILE *fp;

	/* Try to open the corresponding digest file. */
	{
				char *clx_path;
				char *dot;
		
				/* Get a copy of the filename, with some room on the end. */
				/*	clx_path = safe_malloc(strlen(rec->file_name) + 5); */
				clx_path = malloc((strlen(rec->file_name) + 5) *sizeof(char));
				strcpy(clx_path, rec->file_name);
		
				/* Truncate the path after the last dot. */
				dot = strrchr(clx_path, '.');
				if (dot == nil) { free(clx_path); return(-1); }
				*(dot + 1) = 0;
		
				/* Append the classifier-digest extension. */
				strcat(clx_path, "clx");
		
				fp = fopen(clx_path, "r");
				if (fp == nil) {
						free(clx_path);
						return(-1);
				}
		
				free(clx_path);
	}

	/* Read-in the name and dominant points for each class. */
	for (nclasses = 0; !feof(fp); nclasses++) {
		point_list *dpts = nil;
		char class[BUFSIZ];
		int npts;
		int j;

		if (fscanf(fp, "%s %d", class, &npts) != 2) {
			if (feof(fp)) break;

			goto failed;
		}
		rec->cnames[nclasses] = strdup(class);

		/* Allocate a dominant-points list. */
		/* dpts = (point_list *)safe_malloc(sizeof(point_list)); */
		dpts = malloc(sizeof(point_list));
		dpts->pts = mallocz(npts*sizeof(pen_point), 1);
		if (dpts->pts == nil) goto failed;
		dpts->npts = npts;
		dpts->next = nil;

		/* Read in each point. */
		for (j = 0; j < npts; j++) {
			int x, y;

			if (fscanf(fp, "%d %d", &x, &y) != 2) goto failed;
			dpts->pts[j].x = x;
			dpts->pts[j].y = y;
		}

		/* Compute the chain-code. */
		lialg_compute_chain_code(dpts);

		/* Store the list in the rec data structure. */
		rec->dompts[nclasses] = dpts;

		continue;

failed:
		fprint(2, "read_classifier_digest failed...\n");
		for (; nclasses >= 0; nclasses--) {
			if (rec->cnames[nclasses] != nil) {
				free(rec->cnames[nclasses]);
				rec->cnames[nclasses] = nil;
			}
			if (rec->dompts[nclasses] != nil) {
				delete_examples(rec->dompts[nclasses]);
				rec->dompts[nclasses] = nil;
			}
		}
		if (dpts != nil)
			delete_examples(dpts);
		fclose(fp);
		return(-1);
	}

	fclose(fp);
	return(0);
}


/*************************************************************

  Canonicalization routines

 *************************************************************/

static int lialg_canonicalize_examples(rClassifier *rec) {
	int i;
	int nclasses;

	if (lidebug) {
		fprint(2, "lialg_canonicalize_examples working on %s\n",
				rec->file_name);
	}
	/* Initialize canonical-example arrays. */
	for (i = 0; i < MAXSCLASSES; i++) {
		rec->canonex[i] = nil;
	}

	/* Figure out number of classes. */
	for (nclasses = 0;
		  nclasses < MAXSCLASSES && rec->cnames[nclasses] != nil;
		  nclasses++)
		;

	/* Canonicalize the examples for each class. */
	for (i = 0; i < nclasses; i++) {
		int j, k;
		int nex;
		point_list *pts, *tmp, *avg;
		int maxxrange, maxyrange;
		int minx, miny, maxx, maxy;
		int avgxrange, avgyrange, avgxoff, avgyoff, avgscale;

		
		if (lidebug) {
			fprint(2, "lialg_canonicalize_examples working on class %s\n",
					rec->cnames[i]);
		}
		/* Make a copy of the examples. */
		pts = nil;
		tmp = rec->ex[i];
		for (nex = 0; tmp != nil; nex++, tmp = tmp->next) {
			if ((pts = add_example(pts, tmp->npts, tmp->pts)) == nil) {
				delete_examples(pts);
				return(-1);
			}
		}

		/* Canonicalize each example, and derive the max x and y ranges. */
		maxxrange = 0;
		maxyrange = 0;
		for (j = 0, tmp = pts; j < nex; j++, tmp = tmp->next) {
			if (lialg_canonicalize_example_stroke(tmp) != 0) {
  	        if (lidebug) {
					fprint(2, "lialg_canonicalize_example_stroke returned error\n");
				}
				return(-1);
			}

			if (tmp->xrange > maxxrange) maxxrange = tmp->xrange;
			if (tmp->yrange > maxyrange) maxyrange = tmp->yrange;
		}

		/* Normalize max ranges. */
		if (((100 * maxxrange + CANONICAL_X / 2) / CANONICAL_X) >
			((100 * maxyrange + CANONICAL_Y / 2) / CANONICAL_Y)) {
			maxyrange = (maxyrange * CANONICAL_X + maxxrange / 2) / maxxrange;
			maxxrange = CANONICAL_X;
		}
		else {
			maxxrange = (maxxrange * CANONICAL_Y + maxyrange / 2) / maxyrange;
			maxyrange = CANONICAL_Y;
		}

		/* Re-scale each example to max ranges. */
		for (j = 0, tmp = pts; j < nex; j++, tmp = tmp->next) {
			int scalex = (tmp->xrange == 0) ? 100 : (100 * maxxrange + tmp->xrange / 2) / tmp->xrange;
			int scaley = (tmp->yrange == 0) ? 100 : (100 * maxyrange + tmp->yrange / 2) / tmp->yrange;
			if (lialg_translate_points(tmp, 0, 0, scalex, scaley) != 0) {
				delete_examples(pts);
				return(-1);
			}
		}

		/* Average the examples; leave average in first example. */
		avg = pts;				/* careful aliasing!! */
		for (k = 0; k < NCANONICAL; k++) {
			int xsum = 0;
			int ysum = 0;

			for (j = 0, tmp = pts; j < nex; j++, tmp = tmp->next) {
						xsum += tmp->pts[k].x;
						ysum += tmp->pts[k].y;
			}

			avg->pts[k].x = (xsum + j / 2) / j;
			avg->pts[k].y = (ysum + j / 2) / j;
		}

		/* Compute BB of averaged stroke and re-scale. */
		lialg_get_bounding_box(avg, &minx, &miny, &maxx, &maxy);
		avgxrange = maxx - minx;
		avgyrange = maxy - miny;
		avgscale = (((100 * avgxrange + CANONICAL_X / 2) / CANONICAL_X) >
					((100 * avgyrange + CANONICAL_Y / 2) / CANONICAL_Y))
		  ? (100 * CANONICAL_X + avgxrange / 2) / avgxrange
		  : (100 * CANONICAL_Y + avgyrange / 2) / avgyrange;
		if (lialg_translate_points(avg, minx, miny, avgscale, avgscale) != 0) {
			delete_examples(pts);
			return(-1);
		}

		/* Re-compute the x and y ranges and center the stroke. */
		lialg_get_bounding_box(avg, &minx, &miny, &maxx, &maxy);
		avgxrange = maxx - minx;
		avgyrange = maxy - miny;
		avgxoff = -((CANONICAL_X - avgxrange + 1) / 2);
		avgyoff = -((CANONICAL_Y - avgyrange + 1) / 2);
		if (lialg_translate_points(avg, avgxoff, avgyoff, 100, 100) != 0) {
			delete_examples(pts);
			return(-1);
		}

		/* Create a point list to serve as the ``canonical representation. */
		if ((rec->canonex[i] = add_example(nil, avg->npts, avg->pts)) == nil) {
			delete_examples(pts);
			return(-1);
		}
		(rec->canonex[i])->xrange = maxx - minx;
		(rec->canonex[i])->yrange = maxy - miny;

		if (lidebug) {
			fprint(2, "%s, avgpts = %d\n", rec->cnames[i], avg->npts);
			for (j = 0; j < avg->npts; j++) {
						fprint(2, "  (%P)\n", avg->pts[j].Point);
			}
		}

		/* Compute dominant points of canonical representation. */
		rec->dompts[i] = lialg_compute_dominant_points(avg);

		/* Clean up. */
		delete_examples(pts);
	}

	/* Sanity check. */
	for (i = 0; i < nclasses; i++) {
		char *best_name = lialg_recognize_stroke(rec, rec->canonex[i]);

		if (best_name != rec->cnames[i])
			fprint(2, "%s, best = %s\n", rec->cnames[i], best_name);
	}

	return(0);
}


static int lialg_canonicalize_example_stroke(point_list *points) {
	int minx, miny, maxx, maxy, xrange, yrange, scale;

	/* Filter out points that are too close. */
	if (lialg_filter_points(points) != 0) return(-1);

	/* Must be at least two points! */
	if (points->npts < 2) {
		if (lidebug) {
			fprint(2, "lialg_canonicalize_example_stroke: npts=%d\n",
					points->npts);
		}
		return(-1);
	}

	/* Scale up to avoid conversion errors. */
	lialg_get_bounding_box(points, &minx, &miny, &maxx, &maxy);
	xrange = maxx - minx;
	yrange = maxy - miny;
	scale = (((100 * xrange + CANONICAL_X / 2) / CANONICAL_X) >
			 ((100 * yrange + CANONICAL_Y / 2) / CANONICAL_Y))
	  ? (100 * CANONICAL_X + xrange / 2) / xrange
	  : (100 * CANONICAL_Y + yrange / 2) / yrange;
	if (lialg_translate_points(points, minx, miny, scale, scale) != 0) {
		if (lidebug) {
			fprint(2, "lialg_translate_points (minx=%d,miny=%d,scale=%d) returned error\n", minx, miny, scale);
		}
		return(-1);
	}

	/* Compute an equivalent stroke with equi-distant points. */
	if (lialg_compute_equipoints(points) != 0) return(-1);

	/* Re-translate the points to the origin. */
	lialg_get_bounding_box(points, &minx, &miny, &maxx, &maxy);
	if (lialg_translate_points(points, minx, miny, 100, 100) != 0) {
		if (lidebug) {
			fprint(2, "lialg_translate_points (minx=%d,miny=%d) returned error\n", minx, miny);
		}
		return(-1);
	}

	/* Store the x and y ranges in the point list. */
	xrange = maxx - minx;
	yrange = maxy - miny;
	points->xrange = xrange;
	points->yrange = yrange;

	if (lidebug) {
		int i;
		fprint(2, "Canonicalized:   %d, %d, %d, %d\n", minx, miny, maxx, maxy);
		for (i = 0; i < points->npts; i++)
			fprint(2, "      (%P)\n", points->pts[i].Point);
		fflush(stderr);
	}

	return(0);
}


static int lialg_compute_equipoints(point_list *points) {
	pen_point *equipoints = mallocz(NCANONICAL*sizeof(pen_point), 1);
	int nequipoints = 0;
	int pathlen = lialg_compute_pathlen(points);
	int equidist = (pathlen + (NCANONICAL - 1) / 2) / (NCANONICAL - 1);
	int i;
	int dist_since_last_eqpt;
	int remaining_seglen;
	int dist_to_next_eqpt;

	if (equipoints == nil) {
		fprint(2, "can't allocate memory in lialg_compute_equipoints");
		return(-1);
	}

	if (lidebug) {
		fprint(2, "compute_equipoints:  npts = %d, pathlen = %d, equidist = %d\n",
				points->npts, pathlen, equidist);
		fflush(stderr);
	}

	/* First original point is an equipoint. */
	equipoints[0] = points->pts[0];
	nequipoints++;
	dist_since_last_eqpt = 0;

	for (i = 1; i < points->npts; i++) {
		int dx1 = points->pts[i].x - points->pts[i-1].x;
		int dy1 = points->pts[i].y - points->pts[i-1].y;
		int endx = 100 * points->pts[i-1].x;
		int endy = 100 * points->pts[i-1].y;
		remaining_seglen = isqrt(10000 * (dx1 * dx1 + dy1 * dy1));
		dist_to_next_eqpt = equidist - dist_since_last_eqpt;

		while (remaining_seglen >= dist_to_next_eqpt) {
			if (dx1 == 0) {
				/* x-coordinate stays the same */
				if (dy1 >= 0)
					endy += dist_to_next_eqpt;
				else
					endy -= dist_to_next_eqpt;
			}
			else {
				int slope = (100 * dy1 + dx1 / 2) / dx1;
				int tmp = isqrt(10000 + slope * slope);
				int dx = (100 * dist_to_next_eqpt + tmp / 2) / tmp;
				int dy = (slope * dx + 50) / 100;

				if (dy < 0) dy = -dy;
				if (dx1 >= 0)
					endx += dx;
				else
					endx -= dx;
				if (dy1 >= 0)
					endy += dy;
				else
					endy -= dy;
			}

			equipoints[nequipoints].x = (endx + 50) / 100;
			equipoints[nequipoints].y = (endy + 50) / 100;
			nequipoints++;
/*	    assert(nequipoints <= NCANONICAL);*/
			dist_since_last_eqpt = 0;
			remaining_seglen -= dist_to_next_eqpt;
			dist_to_next_eqpt = equidist;
		}

		dist_since_last_eqpt += remaining_seglen;
	}

	/* Take care of last equipoint. */
	if (nequipoints == NCANONICAL) {
		/* Good. */
	} else if (nequipoints == (NCANONICAL - 1)) {
		/* Make last original point the last equipoint. */
		equipoints[nequipoints] = points->pts[points->npts - 1];
	} else {
	  if (lidebug) {
		fprint(2,"lialg_compute_equipoints: nequipoints = %d\n", 
				nequipoints);
	  }
/*	assert(false);*/
		return(-1);
	}

	points->npts = NCANONICAL;
	delete_pen_point_array(points->pts);
	points->pts = equipoints;
	return(0);
}


/*************************************************************

  Utility routines

 *************************************************************/

/* Result is x 100. */
static int lialg_compute_pathlen(point_list *points) {
	return(lialg_compute_pathlen_subset(points, 0, points->npts - 1));
}


/* Result is x 100. */
static int lialg_compute_pathlen_subset(point_list *points,
										   int start, int end) {
	int pathlen;
	int i;

	pathlen = 0;
	for (i = start + 1; i <= end; i++) {
		int dx = points->pts[i].x - points->pts[i-1].x;
		int dy = points->pts[i].y - points->pts[i-1].y;
		int dist = isqrt(10000 * (dx * dx + dy * dy));
		pathlen += dist;
	}

	return(pathlen);
}


/* Note that this does NOT update points->xrange and points->yrange! */
static int lialg_filter_points(point_list *points) {
	int filtered_npts;
	pen_point *filtered_pts = mallocz(points->npts*sizeof(pen_point), 1);
	int i;

	if (filtered_pts == nil) {
		fprint(2, "can't allocate memory in lialg_filter_points");
		return(-1);
	}

	filtered_pts[0] = points->pts[0];
	filtered_npts = 1;
	for (i = 1; i < points->npts; i++) {
		int j = filtered_npts - 1;
		int dx = points->pts[i].x - filtered_pts[j].x;
		int dy = points->pts[i].y - filtered_pts[j].y;
		int magsq = dx * dx + dy * dy;

		if (magsq >= DIST_SQ_THRESHOLD) {
			filtered_pts[filtered_npts] = points->pts[i];
			filtered_npts++;
		}
	}

	points->npts = filtered_npts;
	delete_pen_point_array(points->pts);
	points->pts = filtered_pts;
	return(0);
}


/* scalex and scaley are x 100. */
/* Note that this does NOT update points->xrange and points->yrange! */
static int lialg_translate_points(point_list *points,
								   int minx, int miny,
								   int scalex, int scaley) {
	int i;

	for (i = 0; i < points->npts; i++) {
				points->pts[i].x = ((points->pts[i].x - minx) * scalex + 50) / 100;
				points->pts[i].y = ((points->pts[i].y - miny) * scaley + 50) / 100;
	}

	return(0);
}


static void lialg_get_bounding_box(point_list *points,
									int *pminx, int *pminy,
									int *pmaxx, int *pmaxy) {
	int minx, miny, maxx, maxy;
	int i;

	minx = maxx = points->pts[0].x;
	miny = maxy = points->pts[0].y;
	for (i = 1; i < points->npts; i++) {
				pen_point *pt = &(points->pts[i]);
				if (pt->x < minx) minx = pt->x;
				else if (pt->x > maxx) maxx = pt->x;
				if (pt->y < miny) miny = pt->y;
				else if (pt->y > maxy) maxy = pt->y;
	}

	*pminx = minx;
	*pminy = miny;
	*pmaxx = maxx;
	*pmaxy = maxy;
}


int wtvals[] = {100, 104, 117, 143, 189, 271, 422};

static void lialg_compute_lpf_parameters(void) {
	int i;

		for (i = LP_FILTER_WIDTH; i >= 0; i--) {
//		double x = 0.04 * (i * i);
//		double tmp = 100.0 * exp(x);
//		int wt = floor((double)tmp);
				int wt = wtvals[i];
				lialg_lpfwts[LP_FILTER_WIDTH - i] = wt;
				lialg_lpfwts[LP_FILTER_WIDTH + i] = wt;
		}
		lialg_lpfconst = 0;
		for (i = 0; i < (2 * LP_FILTER_WIDTH + 1); i++) {
				lialg_lpfconst += lialg_lpfwts[i];
		}
}


/* Code from Joseph Hall (jnhall@sat.mot.com). */
static int isqrt(int n) {
		register int i;
		register long k0, k1, nn;

		for (nn = i = n, k0 = 2; i > 0; i >>= 2, k0 <<= 1)
				;
		nn <<= 2;
	for (;;) {
				k1 = (nn / k0 + k0) >> 1;
				if (((k0 ^ k1) & ~1) == 0)
				break;
				k0 = k1;
		}
		return (int) ((k1 + 1) >> 1);
}


/* Helper routines from Mark Hayter. */
static int likeatan(int tantop, int tanbot) { 
		int t;
		/* Use tan(theta)=top/bot --> order for t */
		/* t in range 0..0x40000 */

		if ((tantop == 0) && (tanbot == 0)) 
				t = 0;
	else
		{
				t = (tantop << 16) / (abs(tantop) + abs(tanbot));
				if (tanbot < 0) 
						t = 0x20000 - t;
				else 
						if (tantop < 0) t = 0x40000 + t;
		}
		return t;
}


static int quadr(int t) {
	return (8 - (((t + 0x4000) >> 15) & 7)) & 7;
}
