/* 
 *  hre_api.c:        Implementation of HRE API
 *  Author:           James &
 *  Created On:       Wed Dec  9 13:49:14 1992
 *  Last Modified By: James Kempf
 *  Last Modified On: Fri Sep 23 13:49:04 1994
 *  Update Count:     137
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

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <scribble.h>

#include "scribbleimpl.h"
#include "hre_internal.h"

/* ari -- prototype for rii function */
recognizer __recognizer_internal_initialize(rec_info* ri);

/*Version number of API.*/

char* REC_VERSION = "2.0";

/*Domain name for internationalized text.*/

#define INTL_DOMAIN "recognition_manager"

/* XXX -- Intl Hack -- Jay & Ari */
#define	dgettext(domain, msg)	(msg)
#define	bindtextdomain(dirname,	domain)

/*
 * These magic numbers are used to ensure the integrity of the
 * recognizer structure.
*/


#define REC_MAGIC       0xfeed
#define REC_END_MAGIC   0xbeef

/*Check the recognizer for validity*/

#define RI_CHECK_MAGIC(rec) \
  ( (rec != nil) && \
    (((recognizer)rec)->recognizer_magic == REC_MAGIC) && \
   (((recognizer)rec)->recognizer_end_magic == REC_END_MAGIC) &&\
   (((recognizer)rec)->recognizer_version == REC_VERSION) )

/*The name of the initialization & finalization functions.*/

/* static char rii_name[] = "__recognizer_internal_initialize";
static char rif_name[] = "__recognizer_internal_finalize";  */

/*User home directory for recognizer info.*/
/* ari -- changed USERRECHOME from ".recognizers" */
#define HOME "HOME"
#define USERRECHOME ".classifiers"

/*Local functions*/

static char* shared_library_name(char* directory,char* locale,char* name);
static rec_info* make_rec_info(char* directory,char* name,char** subset);
static void delete_rec_info(rec_info* ri);
static int check_for_user_home(void);
static void intl_initialize(void);

static void cleanup_rec_element(rec_element* re,bool delete_points_p);

/*The last error.*/

static char* the_last_error = nil;

static char *safe_malloc (int nbytes)
{
  char *res = malloc(nbytes);
  if (res == nil) {
    sysfatal("malloc failure");
  }
  return (res);
}


/*
 * Implementation of API functions
*/

/*
 * recognizer_load - Load the recognizer matching the rec_info struct.
 * If name is not null, then load the recognizer having that name. Returns
 * the recognizer object, or null if it can't load the recognizer, and
 * sets errno to indicate why.
*/

recognizer 
recognizer_load(char* directory, char* name, char** subset)
{
    recognizer	rec;				/*the recognizer*/
    rec_info*	rinf;				/*rec_info for recognizer information*/
    static bool	intl_init = false;	/*true if recog. manager initted.*/

    if( intl_init == false ) {
      intl_init = true;
      intl_initialize();
    }

    /*The name takes precedence.*/
    rinf = make_rec_info(directory, name, subset);
    if (rinf == nil) {
	the_last_error = 
	  dgettext(INTL_DOMAIN,
		   "Ran out of memory during prelinking initialization.");
	return((recognizer)nil);
    } 
/* fprint(2, "Got past make_rec_info.\n"); */

    /*Let recognition code create recognizer and initialize*/
    rec = __recognizer_internal_initialize(rinf);
    if (rec == nil) {
	return((recognizer)nil);
    }
/* fprint(2, "Did rii.\n"); */
    /*Check whether it's been correctly initialized*/

    if( rec->recognizer_load_state == nil ||
        rec->recognizer_save_state == nil ||
        rec->recognizer_load_dictionary == nil ||
        rec->recognizer_save_dictionary == nil ||
        rec->recognizer_free_dictionary == nil ||
        rec->recognizer_add_to_dictionary == nil ||
        rec->recognizer_delete_from_dictionary == nil ||
        rec->recognizer_error == nil ||
        rec->recognizer_set_context == nil ||
        rec->recognizer_get_context == nil ||
        rec->recognizer_clear == nil ||
        rec->recognizer_get_buffer == nil ||
        rec->recognizer_set_buffer == nil ||
        rec->recognizer_translate == nil ||
        rec->recognizer_get_extension_functions == nil ||
        rec->recognizer_get_gesture_names == nil ||
        rec->recognizer_set_gesture_action == nil
       ) {

	recognizer_unload(rec);
/* fprint(2, "Unloading b/c null function pointer.\n"); */
	the_last_error = 
	  dgettext(INTL_DOMAIN,
		   "One or more recognizer function pointers is nil.");
	return((recognizer)nil);
    }


    /*Set the rec_info structure.*/

    rec->recognizer_info = rinf;

    /*Check whether home directory is there for recognizer info.*/

/*
 *  ari -- don't bother.  We're not going to load from each user's
 *  home directory at this point.  Instead, we'll use a stupid
 *  little a-b-c file because it loads FAST.
 *
 *    if( check_for_user_home() < 0 ) {
 *	recognizer_unload(rec);
 *	return((recognizer)nil);
 *   }
 */
    /*We got it!*/
/* fprint(2, "Done.\n"); */

    return(rec);
}

/*
 * recognizer_unload - Unload the recognizer.
*/

int
recognizer_unload(recognizer rec)
{
    /*Make sure magic numbers right.*/
    
	if( !RI_CHECK_MAGIC(rec) ) {
		the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
		return(-1);
	}
    
	return __recognizer_internal_finalize(rec);
}

/*
 * recognizer_load_state-Get any recognizer state associated with name
 * in dir. Note that name may not be simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
*/

int recognizer_load_state(recognizer rec, char* dir, char* name)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
		the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
		return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_load_state(rec, dir, name));
}

/*
 * recognizer_save_state-Save any recognizer state to name
 * in dir. Note that name may not be a simple file name, since
 * there may be more than one file involved. Return 0 if successful,
 * -1 if not.
*/

int recognizer_save_state(recognizer rec,char* dir,char* name)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_save_state(rec,dir,name));
}

/*
 * recognizer_load_dictionary-Load dictionary, return pointer
 * to it, or nil if error.
*/

wordset recognizer_load_dictionary(recognizer rec,char* dir,char* name)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(nil);
    }

    /*Do the function.*/

    return(rec->recognizer_load_dictionary(rec,dir,name));
}

/*
 * recognizer_save_dictionary-Save the  dictionary to the file, return 0 if
 * OK, -1 if error.
*/

int recognizer_save_dictionary(recognizer rec,char* dir,char* name,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_save_dictionary(rec,dir,name,dict));
}

/*
 * recognizer_free_dictionary-Free the dictionary, return 0 if
 * OK, -1 if error.
*/

int recognizer_free_dictionary(recognizer rec,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_free_dictionary(rec,dict));
}

/*
 * recognizer_add_to_dictionary-Add word to the dictionary,
 * return 0 if OK, -1 if error.
*/


int recognizer_add_to_dictionary(recognizer rec,letterset* word,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_add_to_dictionary(rec,word,dict));
}

/*
 * recognizer_delete_from_dictionary-Delete word from the dictionary,
 * return 0 if OK, -1 if error.
*/

int 
recognizer_delete_from_dictionary(recognizer rec,letterset* word,wordset dict)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_delete_from_dictionary(rec,word,dict));
}

/*
 * recognizer_get_info-Get a pointers to the rec_info
 * giving the locales and subsets supported by the recognizer
 * and the shared library pathname.
*/

const rec_info*
recognizer_get_info(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return((rec_info*)nil);
    }

    /*Return the rec_info object.*/

    return(rec->recognizer_info);
}

/*
 * recognizer_manager_version-Return the version number string of the
 * recognition manager.
*/

const char* recognizer_manager_version(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(nil);
    }

    return(rec->recognizer_version);
  
}
/*
 * recognizer_error-Return the last error message, or nil if none.
*/

char* recognizer_error(recognizer rec)
{
    
    /*Make sure magic numbers right and function there.*/

    if( !RI_CHECK_MAGIC(rec) && the_last_error == nil ) {
      return(dgettext(INTL_DOMAIN,"Bad recognizer object."));

    } else if( the_last_error != nil ) {
      char* error = the_last_error;

      the_last_error = nil;
      return(error);
    }

    /*Do the function.*/

    return(rec->recognizer_error(rec));
}

/*
 * recognizer_set_context-Set the recognition context for translation.
 * Return 0 if successful, -1 if error.
*/

int recognizer_set_context(recognizer rec,rc* rec_xt)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_set_context(rec,rec_xt));
}

/* 
 * recognzier_get_context-Get the recognition context for translation.
 * If none or error, return nil.
*/

rc* recognizer_get_context(recognizer rec)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(nil);
    }

    /*Do the function.*/

    return(rec->recognizer_get_context(rec));
}

/*
 * recognizer_clear-Clear buffer and recognition context.
 * Return 0 if success, else -1.
*/

int recognizer_clear(recognizer rec,bool delete_points_p)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_clear(rec,delete_points_p));
}

/*recognizer_get_buffer-Get stroke buffer. Return 0 if success, else -1.*/


int recognizer_get_buffer(recognizer rec, uint* nstrokes,Stroke** strokes)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_get_buffer(rec,nstrokes,strokes));

}

/*
 * recognizer_set_buffer-Set stroke buffer to arg. Return 0 if success, else 
 * return -1.
*/

int recognizer_set_buffer(recognizer rec,uint nstrokes,Stroke* strokes)
{

    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(-1);
    }

    /*Do the function.*/

    return(rec->recognizer_set_buffer(rec,nstrokes,strokes));

}

/*
 * recognizer_translate-Translate the strokes in the current context, including
 * buffered strokes. If nstrokes == 0 or strokes == nil, return 
 * translation of stroke buffer.
*/

int recognizer_translate(recognizer rec,
			 uint nstrokes,
			 Stroke* strokes,
			 bool correlate_p,
			 int* nret,
			 rec_alternative** ret)
{
    int retval;
    char msg[80];
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN, msg);
	return(-1);
    }

/* ari */
/*    {
 *      uint i;
 *      Stroke ari_pstr;
 *      pen_point* ari_pts;
 *      int ari;
 *      for (i = 0; i < nstrokes; i++) {
 *	ari_pstr = strokes[i];
 *	ari_pts = ari_pstr.ps_pts;
 *	fprint(2, "\nrecognizer_translate: ari_pts = %ld, sizeof(Time) = %d, sizeof(ari_pts[0] = %d, %d points are...\n", ari_pts, sizeof(Time), sizeof(ari_pts[0]), ari_pstr.ps_npts);
 *	for (ari = 0; ari < ari_pstr.ps_npts; ari++)
 *	   fprint(2, "%ld -- (%d, %d)  ", ari_pts[ari], ari_pts[ari].x, ari_pts[ari].y);
 *      }
 *    }     
*/
    /*Do the function.*/
/* ari -- this is calling cmu_recognizer_translate */
    retval = rec->recognizer_translate(rec,
				     nstrokes,
				     strokes,
				     correlate_p,
				     nret,
				     ret);
    return (retval);
}


/*
 * recognizer_get_extension_functions-Return a null terminated array
 * of functions providing extended functionality. Their interfaces
 * will change depending on the recognizer.
*/

rec_fn* recognizer_get_extension_functions(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return((rec_fn*)nil);
    }

    /*Do the function.*/

    return(rec->recognizer_get_extension_functions(rec));
}


/*
 * recognizer_get_gesture_names - Return a null terminated array of
 * gesture name strings.
*/

char**
recognizer_get_gesture_names(recognizer rec)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return(nil);
    }

    /*Do the function.*/

    return(rec->recognizer_get_gesture_names(rec));
}

/*
 * recognizer_set_gesture_action-Set the action function for the gesture.
*/

xgesture 
recognizer_train_gestures(recognizer rec,char* name,xgesture fn,void* wsinfo)
{
    /*Make sure magic numbers right.*/

    if( !RI_CHECK_MAGIC(rec) ) {
	the_last_error = dgettext(INTL_DOMAIN,"Bad recognizer object.");
	return((xgesture)-1);
    }

    /*Do the function.*/

    return(rec->recognizer_set_gesture_action(rec,name,fn,wsinfo));
}

/*
 * Local functions.
*/

/*
 * shared_library_name-Get the full pathname to the shared library,
 *    based on the recognizer name and the environment.
*/


static char* shared_library_name(char* directory,char* locale,char* name)
{
    char* ret;
    int len = strlen(name);

    /*If directory is there, it takes precedence.*/

    if( directory != nil ) {
		ret = (char*)safe_malloc(strlen(directory) + len + 2);
		strcpy(ret,directory);
		strcat(ret,"/");
		strcat(ret,name);
    } else {
		char* dir;
	
		/*First try the environment variable.*/
	
		if( (dir = getenv(RECHOME)) == nil ) {
		    dir = "REC_DEFAULT_HOME_DIR";
	
		  }
	
		ret = (char*)safe_malloc(strlen(dir) + strlen(locale) + len + 3);
		/*Form the pathname.*/
		strcpy(ret,dir);
		strcat(ret,"/");
		strcat(ret,locale);
		strcat(ret,"/");
		strcat(ret,name);
	}

    return(ret);
}

/*
 * intl_initialize-Initialize the internationaliztion of messages for
 * the recognition manager.
*/

static void intl_initialize(void)
{
	char* dirname;

	/*Get recognizer home directory name from environment.*/

	if( (dirname = getenv(RECHOME)) == nil ) {
		dirname = "REC_DEFAULT_HOME_DIR";
	}

	/*Bind the text domain.*/
	USED(dirname);
	bindtextdomain(dirname, INTL_DOMAIN);
}


/*make_rec_info-Create a rec_info structure*/

static rec_info* make_rec_info(char*, char*, char** subset)
{
    int i,len;
    rec_info* ri;
    char* locale;

    ri = (rec_info*)safe_malloc(sizeof(rec_info));
    ri->ri_locale = nil;
    ri->ri_name = nil;
    ri->ri_subset = nil;

    /*Get locale*/

    if( (locale = getenv(LANG)) == nil ) {
		locale = strdup(REC_DEFAULT_LOCALE);
    }

    if( (ri->ri_locale = strdup(locale)) == nil ) {
		delete_rec_info(ri);
		return(nil);
    }

    /*Get shared library pathname.*/

    /*Initialize the subset information.*/

    if( subset != nil ) {
	
	/*Count the subset strings.*/

	for( len = 1; subset[len] != nil; len++ ) ;
	
	/*Copy the subset strings.*/
	
	ri->ri_subset = (char**)safe_malloc((len +1)*sizeof(char*));
	
	for( i = 0; i < len; i++ ) {
	    if( subset[i] != nil ) {
		if( (ri->ri_subset[i] = strdup(subset[i])) == nil ) {
		    delete_rec_info(ri);
		    return(nil);
		}
	    } else {
		ri->ri_subset[i] = subset[i];
	    }
	}

	ri->ri_subset[i] = nil;

    } else {

	ri->ri_subset = nil;
    }
    
    return(ri);
}

static void delete_rec_info(rec_info* ri)
{
    if( ri != nil ) {
	if( ri->ri_locale != nil ) {
	    free(ri->ri_locale);
	}
/*
 *	if( ri->ri_name != nil ) {
 *	    free(ri->ri_name);
 *	}
 */
	if( ri->ri_subset != nil ) {
	    int i;
	    for( i = 0; ri->ri_subset[i] != nil; i++) {
		free(ri->ri_subset[i]);
	    }
	    free(ri->ri_subset);
	}
	free(ri);
    }
}

/*check_for_user_home-Check whether USERRECHOME has been created.*/

static int check_for_user_home()
{
	char* homedir = getenv(HOME);
	char* rechome;
	Dir *dir;

	if( homedir == nil ) {
		the_last_error = "Home environment variable HOME not set.";
		return(-1);
	}

    rechome = (char*)safe_malloc(strlen(homedir) + strlen(USERRECHOME) + 2);

    /*Form name.*/

    strcpy(rechome,homedir);
    strcat(rechome,"/");
    strcat(rechome,USERRECHOME);

    /*Create directory.*/

    dir = dirstat(rechome);
    if (dir != nil) {
		if (dir->mode & DMDIR) {
			free(dir);
			free(rechome);
			return 0;
		}
		free(dir);
	} else {
		int fd;
		if ((fd = create(rechome, OREAD, DMDIR|0755)) >= 0) {
			close(fd);
    		free(rechome);
    		return(0);
		}
    }
	free(rechome);
	return(-1);
}

/*
 * Constructor functions for making structures.
 *
 *    The general philosophy here is that we control all memory
 *    in connected data structures, *except* for pen_point arrays.
 *    There are likely to be lots and lots of points, they are likely
 *    to come from the window system; so if we wanted to control them,
 *    we would have to copy which would be slow. We require the client
 *    to deal with them directly, or the client can give us permission
 *    to delete them.
*/

/*
 * recognizer
*/


recognizer make_recognizer(rec_info* rif)
{
    recognizer rec;
    
    /*Allocate it.*/

    rec = (recognizer)safe_malloc(sizeof(*rec));
    rec->recognizer_magic = REC_MAGIC;
    rec->recognizer_version = REC_VERSION;
    rec->recognizer_info = rif;
    rec->recognizer_specific = nil;
    rec->recognizer_end_magic = REC_END_MAGIC;
    rec->recognizer_load_state = nil;
    rec->recognizer_save_state = nil;
    rec->recognizer_load_dictionary = nil;
    rec->recognizer_save_dictionary = nil;
    rec->recognizer_free_dictionary = nil;
    rec->recognizer_add_to_dictionary = nil;
    rec->recognizer_delete_from_dictionary = nil;
    rec->recognizer_error = nil;
    rec->recognizer_set_context = nil;
    rec->recognizer_get_context = nil;
    rec->recognizer_clear = nil;
    rec->recognizer_get_buffer = nil;
    rec->recognizer_set_buffer = nil;
    rec->recognizer_translate = nil;
    rec->recognizer_get_extension_functions = nil;
    rec->recognizer_get_gesture_names = nil;
    rec->recognizer_set_gesture_action = nil;
    return(rec);
}

void delete_recognizer(recognizer rec)
{

    if( rec != nil ) {
	if( rec->recognizer_info != nil ) {
	    delete_rec_info(rec->recognizer_info);
	}
	free(rec);
    }
}

/*
 * rec_alternative
*/

rec_alternative* make_rec_alternative_array(uint size)
{
    int i;
    rec_alternative* ri;

    ri = (rec_alternative*) safe_malloc(size * sizeof(rec_alternative));

    for( i = 0; i < size; i++ ) {
        ri[i].ra_elem.re_type = REC_NONE;
	ri[i].ra_elem.re_result.aval = nil;
	ri[i].ra_elem.re_conf = 0;
	ri[i].ra_nalter = 0;
	ri[i].ra_next = nil;
    }

    return(ri);    
}

rec_alternative*
  initialize_rec_alternative(rec_alternative* ra, uint nelm)
{
  if( ra != nil ) {
    if( (ra->ra_next = make_rec_alternative_array(nelm)) == nil ) {
      return(nil);
    }

    ra->ra_nalter = nelm;
  }

  return(ra);
}

void delete_rec_alternative_array(uint nalter,
				  rec_alternative* ra,
				  bool delete_points_p)
{
  int i;

    if( ra != nil ) {

      for( i = 0; i < nalter; i++ ) {
	cleanup_rec_element(&ra[i].ra_elem,delete_points_p);
	
	/*Now do the next one down the line.*/
	
	if( ra[i].ra_nalter > 0 ) {
	  delete_rec_alternative_array(ra[i].ra_nalter,
				       ra[i].ra_next,
				       delete_points_p);
        }
      }

      free(ra);
    }
}


/*initialize_rec_element-Initialize a recognition element.*/

rec_element*
initialize_rec_element(rec_element* re,
		       char type,
		       uint size,
		       void* trans,
		       rec_confidence conf)
{
    if( re != nil ) {

	re->re_type = type;
	re->re_conf = conf;
	re->re_result.aval = nil;
	
	switch (type) {
	    
	  case REC_GESTURE:
	    if( size > 0 && trans != nil ) {
		re->re_result.gval = 
		     (gesture*)safe_malloc(sizeof(gesture));
		memcpy((void*)re->re_result.gval,trans,sizeof(gesture));
	    }
	    break;
	    
	  case REC_ASCII:
	  case REC_VAR:
	  case REC_OTHER:
	    if( size > 0 && trans != nil ) {
		re->re_result.aval = 
		     (char*)safe_malloc((size+1)*sizeof(char));
		memcpy((void*)re->re_result.aval,trans,size*sizeof(char));
		re->re_result.aval[size] = '\000';
	    }
	    break;
	    
	  case REC_WCHAR:
	    if( size > 0 && trans != nil ) {
		re->re_result.wval = 
		     (wchar_t*)safe_malloc((size+1)*sizeof(wchar_t));
		memcpy((void*)re->re_result.wval,trans,size*sizeof(wchar_t));
		re->re_result.wval[size] = '\000';
	    }
	    break;
	    
	  case REC_CORR:
	    if( size > 0 && trans != nil ) {
	      re->re_result.rcval =
		   (rec_correlation*)safe_malloc(sizeof(rec_correlation));
	      memcpy((void*)re->re_result.rcval,
		     trans,
		     sizeof(rec_correlation));
	    }
	    break;

	  default:
	    return(nil);
	}

    }

    return(re);
}

static void cleanup_rec_element(rec_element* re,bool delete_points_p)
{
  switch(re->re_type) {
    
  case REC_NONE:
    break;
    
  case REC_ASCII:
  case REC_VAR:
  case REC_WCHAR:
  case REC_OTHER:
    free(re->re_result.aval);
    break;
    
  case REC_GESTURE:
    delete_gesture_array(1,re->re_result.gval,true);
    break;

  case REC_CORR:
    delete_rec_correlation(re->re_result.rcval,
			   delete_points_p);
    break;
    
  }
  
}

/*
 * rec_correlation
*/


rec_correlation* 
make_rec_correlation(char type,
		     uint size,
		     void* trans,
		     rec_confidence conf,
		     uint ps_size)
{
  rec_correlation* rc;

    rc = (rec_correlation*)safe_malloc(sizeof(rec_correlation));

    rc->ro_nstrokes = ps_size;

    /*First initialize element.*/

    if( initialize_rec_element(&(rc->ro_elem),
			       type,
			       size,
			       trans,
			       conf) == nil ) {
      return(nil);
    }
    
    if( (rc->ro_strokes = make_Stroke_array(ps_size)) == nil ) {
      return(nil);
    }
    
    rc->ro_start = (uint*)safe_malloc(ps_size * sizeof(int));
    rc->ro_stop = (uint*)safe_malloc(ps_size * sizeof(int));
    return(rc);
}

void delete_rec_correlation(rec_correlation* rc,bool delete_points_p)
{
  if( rc != nil ) {

    cleanup_rec_element(&rc->ro_elem,delete_points_p);

    delete_Stroke_array(rc->ro_nstrokes,rc->ro_strokes,delete_points_p);

    if( rc->ro_start != nil ) {
      free(rc->ro_start);
    }

    if( rc->ro_stop != nil ) {
      free(rc->ro_stop);
    }

    free(rc);
  }

}


/*
 * rec_fn
*/


rec_fn* make_rec_fn_array(uint size)
{
    rec_fn* ri = (rec_fn*)safe_malloc((size + 1) * sizeof(rec_fn));
    int i;

    for( i = 0; i < size; i++ ) {
	ri[i] = nil;
    }

    ri[i] = nil;

    return(ri);
}

void delete_rec_fn_array(rec_fn* rf)
{
    if( rf != nil ) {
	free(rf);
    }
}

/*
 * Stroke
*/


Stroke* make_Stroke_array(uint size)
{
    int i;
    Stroke* ri;

    ri = (Stroke*) safe_malloc(size * sizeof(Stroke));
    for( i = 0; i < size; i++ ) {
	ri[i].npts = 0;
	ri[i].pts = nil;
    }

    return(ri);       
}

Stroke* initialize_Stroke(Stroke* ps,
				  uint npts,
				  pen_point* pts)
{
  if( ps != nil ) {
    ps->npts = npts;
    ps->pts = pts;
  }
  return (ps);
}

void delete_Stroke_array(uint size,Stroke* ps,bool delete_points_p)
{
  int i;
  
    if( ps != nil ) {

      for( i = 0; i < size; i++ ) {
	    if( delete_points_p ) {
		delete_pen_point_array(ps[i].pts);
	    }
      }
	
      free(ps);
    }
}

/*
 * pen_point
*/

void delete_pen_point_array(pen_point* pp)
{
    if( pp != nil ) {
	free(pp);
    }
}

/*
 * gesture 
*/

gesture*
make_gesture_array(uint size)
{
    return((gesture*)safe_malloc(size * sizeof(gesture)));
}

gesture* initialize_gesture(gesture* g,
			    char* name,
			    uint nhs,
			    pen_point* hspots,
			    pen_rect bbox,
			    xgesture fn,
			    void* wsinfo)
{
	if( g != nil ) {

		/*We don't do points, 'cause they come from the window system.*/

		g->g_nhs = nhs;
		g->g_hspots = hspots;

		g->g_name = strdup(name);

		g->g_bbox = bbox;
		g->g_action = fn;
		g->g_wsinfo = wsinfo;
	}
	return(g);
}

void
delete_gesture_array(uint size,gesture* ga,bool delete_points_p)
{
    int i;

    if( ga != nil ) {

      for( i = 0; i < size; i++ ) {
	
	free(ga[i].g_name);
	
	if( delete_points_p ) {
	  delete_pen_point_array(ga[i].g_hspots);
	}
      }
      
      free(ga);
    }
}

/*
 * copy fns for stroke buffer management.
*/

static Stroke* 
copy_Stroke(Stroke* ps1,Stroke* ps2)
{
  initialize_Stroke(ps1,
			ps2->npts,
			ps2->pts);
  return(ps1);

}

Stroke*
 copy_Stroke_array(uint nstrokes,
		    Stroke* strokes)
{
  int i;
  Stroke* ps = make_Stroke_array(nstrokes);

  if( ps != nil ) {

    for( i = 0; i < nstrokes; i++ ) {

      copy_Stroke(&ps[i],&strokes[i]);

    }

  }

  return(ps);
}

uint*
 copy_state_trans_array(uint ntrans,uint* trans)
{
  uint* pt = (uint*)safe_malloc(ntrans*sizeof(uint));
  int i;

  for( i = 0; i < ntrans; i++ ) {
    pt[i] = trans[i];
  }
  return(pt);

}

Stroke*
concatenate_Strokes(int nstrokes1,
			Stroke* strokes1,
			int nstrokes2,
			Stroke* strokes2,
			int* nstrokes3,
			Stroke** strokes3)
{
  int i;
  int ns;
  Stroke* ps;

  /*Measure new strokes*/

  ns = nstrokes1 + nstrokes2;

  /*Allocate memory*/

  if( (ps = make_Stroke_array(ns)) == nil ) {
    return(nil);
  }

  /*Copy old ones into new.*/

  for( i = 0; i < nstrokes1; i++ ) {
    if( copy_Stroke(&ps[i],&strokes1[i]) == nil ) {
      delete_Stroke_array(ns,ps,false);
      return(nil);
    }
  }

  for( ; i < ns; i++ ) {
    if( copy_Stroke(&ps[i],&strokes2[i - nstrokes1]) == nil ) {
      delete_Stroke_array(ns,ps,false);
      return(nil);
    }
  }

  *nstrokes3 = ns;
  *strokes3 = ps;

  return(ps);
}
