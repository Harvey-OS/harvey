
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
