/*
 * Pins routines (for UCDS library)
 */

#include "smoke.h"
#include <stdio.h>
#include <ctype.h>

#define PINSIZE 1023

char *pinNames[MAXPIN];
char pinTypes[MAXTYPES];
struct bucket *pinBucketTable[PINSIZE];
struct chip *currentChip = NULL_STRUCT(chip);
int maxPinNumber = 0;
int chipType = NO_TYPE;

extern struct bucket *chipBucketTable[ICSIZE];
extern char chipName[STRLEN];
extern char signalName[STRLEN];
extern char chip[STRLEN];
extern int lineNumber;
extern int dFlag;
extern int gFlag;
extern int tFlag;

int ispunctuation(char);
void ReadChipDefinition(FILE *);
void ReadSignal(char, FILE *);
void DefineChip(FILE *);
void DefineSynonym(char *, char *);
void DefineChipName(char *, char *);
void CloseDefinition(void);
struct pin_definitions *NewPinDefinitions(void);
void DefinePins(FILE *, char *);
void AnalyzePinTemplate(char *, char *, char *, FILE *);
void DefinePinTypes(FILE *, char *);
void ScanString(char **, char **, char);
void InsertPin(char *, char *, FILE *);
void ReadMacroSignal(FILE *);
void IgnoreSignal(FILE *);
struct bucket *Lookup(char *, struct bucket **, int);
struct bucket *Enter(char *, struct bucket **, int);

extern void ReadAndBuildNetwork(FILE *);
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

void Trouble(char *, char *);
void Warning(char *, char *);
void Msg(char *, char *);
void Bug(char *);

int ispunctuation(char ch)
{
	if ((ch == '-') || (ch == '+') || (ch == '/') || (ch == '.') || (ch == '_')|| (ch == '$')) return(-1);
	return(0);
} /* end ispunctuation */

void ReadChipDefinition(FILE *stream)
{
	char ch;
	ch = getc(stream);
	if (isspace(ch)) /* .t */
		(void) DefineChip(stream);
	else
	if (ch == 'p') /* .tp */
		(void) DefinePins(stream, pinTypes);
	else
	if ((ch == 't') || (ch == 'T')) /* .tt or .tT */
		(void) DefinePinTypes(stream, pinTypes);
	else	(void) IgnoreLine(stream);
} /* end ReadChipDefinition */

void ReadSignal(char ch, FILE *stream)
{
	int pinNumber;
	char signalName[STRLEN];
	char *signalNamePointer;
	signalNamePointer = signalName;
	do
	{
		*signalNamePointer++ = ch;
		ch = getc(stream);
	} while (!isspace(ch));
	*signalNamePointer = '\0';
	(void) fscanf(stream, "%d", &pinNumber);
	if (pinNumber == 0)
		Bug("zero pin number in ReadSignal");
	(void) AddPin(chipName, chip, signalName, pinNumber);
	(void) IgnoreLine(stream);
} /* end ReadPin */

void DefineChip(FILE *stream)
{
	char field1[STRLEN], field2[STRLEN], field3[STRLEN];
	(void) fscanf(stream, "%s %s", field1, field2);
	if (strcmp(field2, "=") == 0)
	{
		(void) fscanf(stream, "%s", field3);
		(void) DefineSynonym(field1, field3);
		(void) IgnoreLine(stream);
	} /* end if */
	else
	{
		(void) ReadLine(stream, field3);
		(void) MarkComment(field3);
		(void) DefineChipName(field1, field3);
	} /* end else */
} /* end DefineChip */

void DefineSynonym(char *synonymName, char *defineName)
{
	struct bucket *synonymBucket;
	struct chip *defineChip, *synonymChip;
	synonymBucket = Lookup(synonymName, chipBucketTable, ICSIZE);
	if (synonymBucket == NULL_STRUCT(bucket)) return;
	defineChip = LookupOrEnterChip(defineName);
	synonymChip = LookupOrEnterChip(synonymName);
	synonymBucket -> pointer.chip = synonymChip;
	if (defineChip -> pin_definitions == NULL_STRUCT(pin_definitions))
	{
		synonymChip -> chain = defineChip -> chain;
		defineChip -> chain = synonymChip;
	} /* end if */
	else
		synonymChip -> pin_definitions = defineChip -> pin_definitions;
} /* end DefineSynonym */

void DefineChipName(char *name, char *rest)
{
	struct bucket *nameBucket;
	char typeName[STRLEN];
	int i, ground, power, fieldCount;
	if (currentChip != NULL_STRUCT(chip)) (void) CloseDefinition();
	while (isspace(*rest)) rest++;
	fieldCount = (strlen(rest) > 0) ? 
		sscanf(rest, "%d %d %s", &ground, &power, typeName) : 0;
	nameBucket = Lookup(name, chipBucketTable, ICSIZE);
	if (!dFlag && (nameBucket == NULL_STRUCT(bucket)))
		currentChip = NULL_STRUCT(chip);
	else
	{
		currentChip = LookupOrEnterChip(name);
		if (tFlag) fprintf(stderr, "{%s ", name);
	} /* end else */
	maxPinNumber = 0;
	chipType = NO_TYPE;
	pinTypes[0] = '\0'; /* null pin types */
	for (i=0; i<MAXPIN; i++) pinNames[i] = (char *) 0;
} /* end DefineChipName */

void CloseDefinition(void)
{
	int i;
	struct chip *chipPointer;
	struct pin_definitions *definitionsPointer;
	if (tFlag) fprintf(stderr, " : %d}\n", maxPinNumber);
	definitionsPointer = NewPinDefinitions();
	definitionsPointer -> pin_count = maxPinNumber;
	definitionsPointer -> pin_types = (strlen(pinTypes) > 0)? NewString(pinTypes) : 0;
	if (maxPinNumber > 0)
	{
		maxPinNumber++; /* count 0 too */
		definitionsPointer -> pin_names = (char **) malloc(maxPinNumber * sizeof(char *));
		for (i=0; i<maxPinNumber; i++)
			definitionsPointer -> pin_names[i] = pinNames[i];
	} /* end if */
	currentChip -> pin_definitions = definitionsPointer;
	for (chipPointer = currentChip -> chain;
	     chipPointer != NULL_STRUCT(chip);
	     chipPointer = chipPointer -> chain)
		chipPointer -> pin_definitions = definitionsPointer;
} /* end CloseDefinition */

struct pin_definitions *NewPinDefinitions(void)
{
	struct pin_definitions *pinDefinitionsPointer;
	pinDefinitionsPointer = alloc_struct(pin_definitions);
	return(pinDefinitionsPointer);
} /* end NewPinDefinitions */

void DefinePins(FILE *stream, char *pinTypes)
{
	char pinTemplate[STRLEN], pinName[STRLEN];
	if (currentChip != NULL_STRUCT(chip))
	{
		fscanf(stream, "%s", pinTemplate);
		pinName[0] = '\0';
		(void) AnalyzePinTemplate(pinName, pinTemplate, pinTypes, stream);
	} /* end if */
	(void) IgnoreLine(stream);
} /* end DefinePins */

void AnalyzePinTemplate(char *head, char *tail, char *typeString, FILE *stream)
{
	char template[STRLEN], numberString[STRLEN], newHead[STRLEN];
	char *savedHeadPointer, *newHeadPointer, *templatePointer, *numberPointer;
	int pinNumber, firstNumber, secondNumber;
	if (*tail == '\0')
	{
		(void) InsertPin(head, typeString, stream);
		return;
	} /* end if */
	savedHeadPointer = head;
	while (*head) head++; /* advance to end */
	while (isalnum(*tail) || (ispunctuation(*tail))) *head++ = *tail++;
	*head = '\0';
	if (*tail == '\0')
	{
		(void) InsertPin(savedHeadPointer, typeString, stream);
		return;
	} /* end if */
	if (*tail == '[')
	{
		tail++;
		templatePointer = template;
		(void) ScanString(&templatePointer, &tail, ']');
		templatePointer = template;
		if (template[1] == '-') {
			if (template[2] == '\0') /* terminating - */
			{
				newHeadPointer = newHead;
				(void) sprintf(newHeadPointer, "%s%c", savedHeadPointer, *templatePointer++);
				(void) AnalyzePinTemplate(newHeadPointer, tail, typeString, stream);
				(void) sprintf(newHeadPointer, "%s%c", savedHeadPointer, *templatePointer++);
				(void) AnalyzePinTemplate(newHeadPointer, tail, typeString, stream);
			} /* end if */
			else
			{
				firstNumber = templatePointer[0];
				secondNumber = templatePointer[2];
				for (pinNumber = firstNumber; pinNumber <= secondNumber; pinNumber++)
				{
					newHeadPointer = newHead;
					sprintf(newHeadPointer, "%s%c", savedHeadPointer, pinNumber);
					(void) AnalyzePinTemplate(newHeadPointer, tail, typeString, stream);
				} /* end for */
			} /* end else */
		} /* end if '-' */
		else
		{
			for (pinNumber = 0; pinNumber < strlen(template); pinNumber++)
			{
				newHeadPointer = newHead;
				sprintf(newHeadPointer, "%s%c", savedHeadPointer, template[pinNumber]);
				(void) AnalyzePinTemplate(newHeadPointer, tail, typeString, stream);
			} /* end for */
		} /* end else */
	} /* end if */
	else
	if (*tail == '<')
	{
		tail++;
		templatePointer = template;
		(void) ScanString(&templatePointer, &tail, '>');
		templatePointer = template;
		numberPointer = numberString;
		numberString[0] ='\0';
		(void) ScanString(&numberPointer, &templatePointer, ':');
		if (*templatePointer == '\0') /* no second number */
			(void) Trouble("no number after :", NULL);
		else
		{
			firstNumber = atoi(numberString);
			secondNumber = atoi(templatePointer);
			if (firstNumber <= secondNumber)
				for (pinNumber = firstNumber; pinNumber <= secondNumber; pinNumber++)
				{
					newHeadPointer = newHead;
					sprintf(newHeadPointer, "%s%d", savedHeadPointer, pinNumber);
					(void) AnalyzePinTemplate(newHeadPointer, tail, typeString, stream);
				} /* end for */
			else
				for (pinNumber = firstNumber; pinNumber >= secondNumber; pinNumber--)
				{
					newHeadPointer = newHead;
					sprintf(newHeadPointer, "%s%d", savedHeadPointer, pinNumber);
					(void) AnalyzePinTemplate(newHeadPointer, tail, typeString, stream);
				} /* end for */
		} /* end if */
	} /* end if */
} /* end AnalyzePinTemplate */

void DefinePinTypes(FILE *stream, char *pinTypes)
{
	char ch;
	char *pinTypesPointer;
	pinTypesPointer = pinTypes;
	while (!feof(stream))
	{
		ch = getc(stream);
		if (ch == '\n') break;
		if (isalnum(ch)) *pinTypesPointer++ = ch;
	} /* end while */
	lineNumber++;
	*pinTypesPointer = '\0';
} /* end DefinePinTypes */

void ScanString(char **destination, char **source, char stopCharacter)
{
	while (**source && (**source != stopCharacter))
	{
		**destination = **source;
		(*destination)++;
		(*source)++;
	} /* end while */
	**destination = '\0';
	if (**source == stopCharacter) (*source)++; /* skip over stop */
} /* end ScanString */

void InsertPin(char *name, char *typeString, FILE *stream)
{
	int pinNumber;
	(void) fscanf(stream, "%d", &pinNumber);
	if (tFlag && gFlag)
		fprintf(stderr, "[%d,%c]", pinNumber, (pinNumber > strlen(typeString))?
						'\0' : typeString[pinNumber - 1]);
	if (pinNumber > MAXPIN)
		Trouble("pin number greater than compiled in limit", NULL);
	else
	if (pinNumber > maxPinNumber) maxPinNumber = pinNumber;
	pinNames[pinNumber - 1] = NewString(name);
} /* end InsertPin */

void ReadMacroSignal(FILE *stream)
{
	char signalName[STRLEN];
	struct signal *signalPointer;
	(void) fscanf(stream, "%s", signalName);
	signalPointer = LookupOrEnterSignal(signalName);
	signalPointer -> type |= MACRO_SIGNAL;
} /* end ReadMacroSignal */

void IgnoreSignal(FILE *stream)
{
	char signalName[STRLEN];
	struct signal *signalPointer;
	(void) fscanf(stream, "%s", signalName);
	signalPointer = LookupOrEnterSignal(signalName);
	signalPointer -> type |= IGNORE_SIGNAL;
} /* end IgnoreSignal */

struct bucket *Lookup(char *name, struct bucket *hashTable[], int size)
{
	int hash;
	struct bucket *bucketPointer, *lastBucketPointer;
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
	return(NULL_STRUCT(bucket));
} /* end Lookup */

struct bucket *Enter(char *name, struct bucket *hashTable[], int size)
{
	struct bucket *oldBucket, *newBucket;
	int hash;
	oldBucket = Lookup(name, hashTable, size);
	if (oldBucket != NULL_STRUCT(bucket)) return(oldBucket);
	newBucket = NewBucket(name);
	hash = HashName(name, size);
	if (hashTable[hash] == NULL_STRUCT(bucket))	
		hashTable[hash] = newBucket;
	else
	{
		newBucket -> next = hashTable[hash];
		hashTable[hash] = newBucket;
	} /* end else */
	return(newBucket);
} /* end LookupOrEnter */

