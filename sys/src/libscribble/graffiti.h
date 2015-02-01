/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


#define NUM_RECS    3
#define DEFAULT_REC_DIR         "classsifiers"
#define REC_DEFAULT_USER_DIR    "/sys/lib/scribble/classifiers"
#define CLASSIFIER_DIR          "lib/classifiers"
#define DEFAULT_LETTERS_FILE    "letters.cl"
#define DEFAULT_DIGITS_FILE     "digits.cl"
#define DEFAULT_PUNC_FILE       "punc.cl"

struct graffiti {
	/* 3 recognizers, one each for letters, digits, and punctuation: */
	recognizer					rec[3];
	/* directory in which the current classifier files are found: */
	char						cldir[200];
	/* pointer to training function: */
	li_recognizer_train			rec_train;
	/* pointer to function that lists the characters in the classifier file */
	li_recognizer_getClasses	rec_getClasses; 
};

extern char *cl_name[3];
