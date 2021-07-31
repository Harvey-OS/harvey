/*
 * Network routines
 */

#include "smoke.h"
#include <stdio.h>
#include <ctype.h>

struct bucket *instanceBucketTable[INSTANCESIZE];
struct bucket *signalBucketTable[SIGNALSIZE];
struct bucket *chipBucketTable[ICSIZE];

struct signal *lastSignal = NULL_STRUCT(signal);
struct chip *lastChip = NULL_STRUCT(chip);
struct chip *firstChip = NULL_STRUCT(chip);
struct instance *lastInstance = NULL_STRUCT(instance);
	
char chipName[STRLEN];
char signalName[STRLEN];
char chip[STRLEN];
char wFileName[STRLEN];
char *wFiles[MAXFILES];
int wFileCount = -1;

extern int lineNumber;
extern int tFlag;
extern int wFlag;
extern int xFlag;
extern struct chip *currentChip;

extern void ReadAndBuildNetwork(FILE *stream);
extern void IgnoreLine(FILE *);
extern void ReadLine(FILE *, char *);
extern void MarkComment(char *);
extern void ReadChip(FILE *);
extern void AddPin(char *, char *, char *, int);
extern struct instance *FindInstance(char *, char *);
extern struct pin *NewPin(struct instance *, struct signal *, int);
extern struct bucket *LookupOrEnter(char *, struct bucket **, int);
extern struct instance *LookupOrEnterInstance(char *);
extern struct instance *NewInstance(void);
extern struct bucket *NewBucket(char *);
extern char *NewString(char *);
extern int HashName(char *, int);
extern struct signal *LookupOrEnterSignal(char *);
extern struct signal *LookupSignal(char *);
extern struct signal *NewSignal(void);
extern struct chip *LookupOrEnterChip(char *);
extern struct chip *NewChip(void);
extern void ReadIOConnector(FILE *);
extern int CountUndefinedChips(void);
extern void DisplaySignals(void);
extern void DisplayChips(int);
extern void DisplayChipsAndInstances(void);

void CloseDefinition(void);
void ReadMacroSignal(FILE *);
void Trouble(char *, char *);
void ReadChipDefinition(FILE *);
void IgnoreSignal(FILE *);
void Warning(char *, char *);
int ispunctuation(char);
void ReadSignal(char, FILE *);
void SimpleWarning(char *, char *);

void ReadAndBuildNetwork(FILE *stream)
{
	char ch;
	while (!feof(stream))
	{
	    ch = getc(stream);
	    if (feof(stream)) break;
	    if (ch == '.')
	    {
		switch (ch = getc(stream))
			{
		case 'f':
			if (currentChip != NULL_STRUCT(chip))
				(void) CloseDefinition();
			(void) fscanf(stream, "%s", wFileName);
			if (wFlag) fprintf(stderr, "\t%s:\n", wFileName);
			wFiles[++wFileCount] = (char *) malloc(strlen(wFileName) + 1);
			strcpy(wFiles[wFileCount], wFileName);
			continue;
		case 'c':
			(void) ReadChip(stream);
			continue;
		case 'm':
			(void) ReadMacroSignal(stream);
			continue;
		case 'o':
			(void) ReadIOConnector(stream);
			continue;
		case 'q':
			(void) IgnoreLine(stream);
			(void) getc(stream); /* Force EOF */
			if (!feof(stream)) (void) Trouble("No EOF after .q", NULL);
			break;
		case 't':
			(void) ReadChipDefinition(stream);
			continue;
		case 'x':
			(void) IgnoreSignal(stream);
			continue;
		default:
			(void) Warning("Unknown dot command", NULL);
			(void) IgnoreLine(stream);
			} /* end switch */
	    } /* end if */
	    else
	    if ((ch == '\t') || (ch == ' '))
		continue;
	    else
	    if (ch == '%')
		(void) IgnoreLine(stream);
	    else
	    if (isalnum(ch) || ispunctuation(ch))
		(void) ReadSignal(ch, stream);
	    else
	    if (ch == '\n')
		lineNumber++;
	    else
	    {
		(void) Warning("Strange character starts line", NULL);
		(void) IgnoreLine(stream);
	    } /* end else */
	} /* end while */
	if (currentChip != NULL_STRUCT(chip)) (void) CloseDefinition();
} /* end ReadAndBuildNetwork */

void IgnoreLine(FILE *stream)
{
	while (!feof(stream) && (getc(stream) != '\n'));
	lineNumber++;
} /* end IgnoreLine */

void ReadLine(FILE *stream, char *line)
{
	char ch;
	char *linePointer;
	linePointer = line;
	while (!feof(stream))
	{
		ch = getc(stream);
		if (ch == '\n') break;
		*linePointer++ = ch;
	} /* end while */
	*linePointer = '\0';
	lineNumber++;
} /* end ReadLine */

void MarkComment(char *line)
{
	while ((*line != '\0') && (*line != '%'))
		*line++;
	*line = '\0';
} /* end MarkComment */

void ReadChip(FILE *stream)
{
	(void) fscanf(stream, "%s%s", chipName, chip);
	(void) IgnoreLine(stream);
} /* end ReadChip */

void AddPin(char *instanceName, char *chipName, char *signalName, int pinNumber)
{
	struct instance *instancePointer;
	struct signal *signalPointer;
	struct pin *pinPointer, *lastPinPointer, *newPinPointer;
	char fullMessage[STRLEN];

	instancePointer = FindInstance(instanceName, chipName);
	signalPointer = LookupOrEnterSignal(signalName);

	/* Search signal chain next, append at end */
	pinPointer = signalPointer -> first_pin;
	newPinPointer = NewPin(instancePointer, signalPointer, pinNumber);
	if (pinPointer == NULL_STRUCT(pin))
		signalPointer -> first_pin = newPinPointer;
	else
	{
		while (pinPointer != NULL_STRUCT(pin))
		{
			lastPinPointer = pinPointer;
			if ((pinPointer -> pin_number == pinNumber) &&
			    (pinPointer -> instance == instancePointer))
			{
				(void) sprintf(fullMessage, "pin %s.%d (%s) already used",
				chipName, pinNumber, signalName);
				(void) Warning(fullMessage, NULL);
				return;
			} /* end if */
			pinPointer = pinPointer -> instance_chain;
		} /* end while */
		lastPinPointer -> instance_chain = newPinPointer;
	} /* end else */
	signalPointer -> pin_count++;
} /* end AddPin */

struct instance *FindInstance(char *instanceName, char *chipName)
{
	struct instance *instancePointer, *chainInstance;
	struct chip *chipPointer;
	instancePointer = LookupOrEnterInstance(instanceName);
	chipPointer = LookupOrEnterChip(chipName);
	if (instancePointer -> chip == NULL_STRUCT(chip))
		instancePointer -> chip = chipPointer;
	else
	if (strcmp(instancePointer -> chip -> bucket -> name, chipName) != 0)
		(void) SimpleWarning("chip %s has multiple types", instanceName);
	if (chipPointer -> first_instance == NULL_STRUCT(instance))
		chipPointer -> first_instance = instancePointer;
	else
	{
		/* search for "new" instance in instance chain */
		for (chainInstance = chipPointer -> first_instance;
		     chainInstance != NULL_STRUCT(instance);
		     chainInstance = chainInstance -> next)
			if (chainInstance == instancePointer)
				return(instancePointer);
		instancePointer -> next = chipPointer -> first_instance;
		chipPointer -> first_instance = instancePointer;
	} /* end else */
	return(instancePointer);
} /* end FindInstance */

struct pin *NewPin(struct instance *instancePointer, struct signal *signalPointer, int pinNumber)
{
	struct pin *newPin;
	newPin = alloc_struct(pin);
	newPin -> instance = instancePointer;
	newPin -> signal = signalPointer;
	newPin -> pin_number = pinNumber;
	newPin -> instance_chain = NULL_STRUCT(pin);
	newPin -> fileindex = wFileCount;
	return(newPin);
} /* end NewPin */

struct bucket *LookupOrEnter(char *name, struct bucket *hashTable[], int size)
{
	int hash;
	struct bucket *newBucket, *bucketPointer, *lastBucketPointer;
	hash = HashName(name, size);
	if (hashTable[hash] != NULL_STRUCT(bucket))
	{
		for (bucketPointer = hashTable[hash];
		     bucketPointer != NULL_STRUCT(bucket);
		     lastBucketPointer = bucketPointer,
		     bucketPointer = bucketPointer -> next)
			if (strcmp(bucketPointer -> name, name) == 0)
			return(bucketPointer);
	} /* end if */
	newBucket = NewBucket(name);
	if (hashTable[hash] == NULL_STRUCT(bucket))	
		hashTable[hash] = newBucket;
	else
		lastBucketPointer -> next = newBucket;
	return(newBucket);
} /* end LookupOrEnter */

struct instance *LookupOrEnterInstance(char *chipName)
{
	struct bucket *bucketPointer;
	struct instance *newInstance;

	bucketPointer = LookupOrEnter(chipName, instanceBucketTable, INSTANCESIZE);
	if (bucketPointer -> pointer.instance == NULL_STRUCT(instance))
	{
		newInstance = NewInstance();
		bucketPointer -> pointer.instance = newInstance;
		newInstance -> bucket = bucketPointer;
		return(newInstance);
	}
	else
		return(bucketPointer -> pointer.instance);
} /* end LookupOrEnterInstance */

struct instance *NewInstance(void)
{
	struct instance *new;
	new = alloc_struct(instance);
	lastInstance = new;
	return(new);
} /* end NewInstance */

struct bucket *NewBucket(char *name)
{
	struct bucket *new;
	new = alloc_struct(bucket);
	new -> name = NewString(name);
	new -> next = NULL_STRUCT(bucket);
	new -> pointer.chip = NULL_STRUCT(chip); /* Any null will do */
	return(new);
} /* end NewBucket */

char *NewString(char *source)
{
	char *destination, *result;
	destination = result = (char *) malloc(strlen(source) + 1);
	while (*destination++ = *source++);
	return(result);
} /* end NewString */

int HashName(char *name, int tableSize)
{
	int sum = 0;
	char *namePointer;
	namePointer = name;
	while (*namePointer) sum = (sum << 5) + *namePointer++;
	return(Abs(sum) % tableSize);
} /* end HashName */

struct signal *LookupOrEnterSignal(char *signalName)
{
	struct bucket *bucketPointer;
	struct signal *newSignal;

	bucketPointer = LookupOrEnter(signalName, signalBucketTable, SIGNALSIZE);
	if (bucketPointer -> pointer.signal == NULL_STRUCT(signal))
	{
		newSignal = NewSignal();
		bucketPointer -> pointer.signal = newSignal;
		newSignal -> bucket = bucketPointer;
		return(newSignal);
	}
	else
		return(bucketPointer -> pointer.signal);
} /* end LookupOrEnterSignal */

struct signal *LookupSignal(char *signalName)
{
	struct bucket *bucketPointer;

	bucketPointer = LookupOrEnter(signalName, signalBucketTable, SIGNALSIZE);
	return(bucketPointer -> pointer.signal);
} /* end LookupSignal */

struct signal *NewSignal(void)
{
	struct signal *newSignal;
	newSignal = alloc_struct(signal);
	newSignal -> first_pin = NULL_STRUCT(pin);
	newSignal -> pin_count = 0;
	newSignal -> next = lastSignal;
	newSignal -> type = NO_TYPE;
	lastSignal = newSignal;
	return(newSignal);
} /* end NewSignal */

struct chip *LookupOrEnterChip(char *chipName)
{
	struct bucket *bucketPointer;
	struct chip *newChip;

	bucketPointer = LookupOrEnter(chipName, chipBucketTable, ICSIZE);
	if (bucketPointer -> pointer.chip == NULL_STRUCT(chip))
	{
		newChip = NewChip();
		bucketPointer -> pointer.chip = newChip;
		newChip -> bucket = bucketPointer;
		return(newChip);
	}
	else
		return(bucketPointer -> pointer.chip);
} /* end LookupOrEnterChip */

struct chip *NewChip(void)
{
	struct chip *newChip;
	newChip = alloc_struct(chip);
	newChip -> next = NULL_STRUCT(chip);
	newChip -> chain = NULL_STRUCT(chip);
	newChip -> first_instance = NULL_STRUCT(instance);
	newChip -> pin_definitions = NULL_STRUCT(pin_definitions);
	if (firstChip == NULL_STRUCT(chip)) firstChip = newChip;
	else lastChip -> next = newChip;
	lastChip = newChip;
	return(newChip);
} /* end NewChip */

void ReadIOConnector(FILE *stream)
{
	(void) ReadChip(stream);
} /* end ReadIOConnector */

int CountUndefinedChips(void)
{
	struct chip *chipPointer;
	int count;
	for (chipPointer = firstChip, count = 0;
	     chipPointer != NULL_STRUCT(chip);
	     chipPointer = chipPointer -> next)
		if (chipPointer -> pin_definitions == NULL_STRUCT(pin_definitions))
			count++;
		/* else
		if (chipPointer -> pin_definitions -> pin_types == (char *) 0)
			count++; */
	return(count);
} /* end CountUndefinedChips */

void DisplaySignals(void)
{
	struct bucket *bucketPointer;
	struct signal *signalPointer;
	struct pin *pinPointer;
	int i;

	for (i=0; i < SIGNALSIZE; i++)
	{
		for (bucketPointer = signalBucketTable[i];
		     bucketPointer != NULL_STRUCT(bucket);
		     bucketPointer = bucketPointer -> next)
		{
			fprintf(stdout, "%s(%x): ", bucketPointer -> name, bucketPointer);
			signalPointer = bucketPointer -> pointer.signal;
			if (xFlag) fprintf(stdout, "[%x, p%x, b%x]",
				signalPointer, signalPointer -> first_pin,
				signalPointer -> bucket);
			for (pinPointer = signalPointer -> first_pin;
			     pinPointer != NULL_STRUCT(pin);
			     pinPointer = pinPointer -> instance_chain)
			{
				if (xFlag) fprintf(stdout, "(%x:%d)",
					pinPointer -> instance,
					pinPointer -> pin_number);
				else
					fprintf(stdout, "%s.%d ",
					pinPointer -> instance -> bucket ->name,
					pinPointer -> pin_number);
			} /* end for */
			fprintf(stdout, "\n");
		} /* end for */
	} /* end for */
} /* end DisplaySignals */

void DisplayChips(int displayUndefines)
{
	struct chip *chipPointer;
	int count, pin;
	for (chipPointer = firstChip, count = 0;
	     chipPointer != NULL_STRUCT(chip);
	     chipPointer = chipPointer -> next, count++)
	{
		if (chipPointer -> pin_definitions == NULL_STRUCT(pin_definitions))
			if (displayUndefines)
				(void) SimpleWarning("chip %s undefined.",
					chipPointer -> bucket -> name);
			else;
		else
		{
			if (displayUndefines)
			{
		 		if (chipPointer -> pin_definitions -> pin_types == (char *) 0)
				(void) SimpleWarning("chip %s missing .tt line",
				chipPointer -> bucket -> name);
			}
			else
			{
				fprintf(stdout, "%d: %s [%s]\n", count+1,
				chipPointer -> bucket -> name,
				chipPointer -> pin_definitions -> pin_types);
				for (pin = 0; pin < chipPointer -> pin_definitions -> pin_count; pin++)
					fprintf(stdout, "\t%d - %s\n", pin + 1, chipPointer -> pin_definitions -> pin_names[pin]);
			} /* end if */
		} /* end else */
	} /* end for */
} /* end DisplayChips */

void DisplayChipsAndInstances(void)
{
	struct instance *theInstance;
	struct chip *chipPointer;
	int count;

	for (chipPointer = firstChip, count = 0;
	     chipPointer != NULL_STRUCT(chip);
	     chipPointer = chipPointer -> next, count++)
	{
		printf("%s:\n", chipPointer -> bucket -> name);
		for (theInstance = chipPointer -> first_instance;
		     theInstance != NULL_STRUCT(instance);
		     theInstance = theInstance -> next)
			printf("\t%s (0x%x)\n", theInstance -> bucket -> name, theInstance);
	} /* end for */
} /* end DisplayChipsAndInstances */
