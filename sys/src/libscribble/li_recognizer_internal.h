/*
 *  li_recognizer_internal.h
 *
 *  Adapted from cmu_recognizer_internal.h.
 *  Credit to Dean Rubine, Jim Kempf, and Ari Rapkin.
 */

#define MAXSCLASSES 100

typedef struct PointList {
	Stroke;
	int xrange, yrange;
	struct PointList* next;
} point_list;

typedef struct {
    char*		file_name;			/*The classifier file name.*/
    int		nclasses;				/*Number of symbols in class */
    point_list*	ex[MAXSCLASSES];		/*The training examples.*/
    char*		cnames[MAXSCLASSES];	/*The class names.*/
    point_list*	canonex[MAXSCLASSES];	/*Canonicalized vrsions of strokes */
    point_list*	dompts[MAXSCLASSES];	/*Dominant points */
} rClassifier;


/*This structure contains extra fields for instance-specific data.*/

typedef struct {
    /*Instance-specific data.*/
    uint li_magic;            /*Just to make sure nobody's cheating.*/
    rClassifier li_rc;         /*The character classifier.*/
} li_recognizer;


/*Name of the default classifier file.*/
#define LI_DEFAULT_CLASSIFIER_FILE   "default.cl"

/*Classifier file extension.*/
#define LI_CLASSIFIER_EXTENSION ".cl"

/*Locale supported by recognizer.*/
#define LI_SUPPORTED_LOCALE  REC_DEFAULT_LOCALE
