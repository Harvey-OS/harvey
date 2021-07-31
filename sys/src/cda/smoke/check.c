/*
 * The heart of Smoke - the Static Checker
 */

#include "smoke.h"
#include <stdio.h>

#define MAXINPUTS 100
#define MAXOUTPUTS 100
#define MAXSINKS 100
#define MAXSOURCES 100
#define MAXANALOG 100
#define MAXUNDECLARED 100
#define LOAD_LIMIT 10

int warningCount = 0;
int inputPinCount = 0;
int outputPinCount = 0;
int sinkPinCount = 0;
int sourcePinCount = 0;
int analogPinCount = 0;
int undeclaredPinCount = 0;
int totalPinCount = 0;

struct pin *inputPins[MAXINPUTS];
struct pin *outputPins[MAXOUTPUTS];
struct pin *sourcePins[MAXSOURCES];
struct pin *sinkPins[MAXSINKS];
struct pin *analogPins[MAXANALOG];
struct pin *undeclaredPins[MAXUNDECLARED];

extern int aFlag;
extern int bFlag;
extern int cFlag;
extern int mFlag;
extern int nFlag;
extern int pFlag;
extern int sFlag;
extern int tFlag;
extern int wFlag;
extern int xFlag;
extern int LFlag;
extern int max_loads;
extern struct signal *lastSignal;
extern struct chip *firstChip;
extern struct bucket *chipBucketTable[ICSIZE];
extern char *wFiles[MAXFILES];

char *pinDirectionString[] =
	{ "", " input", " output", " bidirectional" };

char *powerGroundStrings[] =
	{ "GND", "VCC", "VDD", "VEE", "VFF", "VGG", "VSS", 0 };

extern void StaticCheck(void);
extern void BuildPinArrays(void);
extern void ChaseSignals(void);
extern void CheckChips(void);
extern void CheckSignal(struct signal *);
extern void CheckForCollision(struct signal *, struct pin *);
extern int PinDirection(int);
extern unsigned char PinType(struct pin *, int);
extern void PrintSignal(struct signal *, int);
extern void CheckSignalNet(struct signal *);
extern void DoPinType(struct pin *, struct signal *, unsigned char);
extern int TypePin(unsigned char);
extern void CheckSignalPins(struct signal *);
extern int PowerGroundNet(struct signal *);
extern int GroundNet(struct signal *);
extern int AnalogNet(struct signal *);
extern int OpenCollectorNet(struct signal *);
extern void CheckSources(struct signal *);
extern void CheckSinks(struct signal *);
extern void CheckBidirectional(struct signal *);
extern void CheckTotemPole(struct signal *, struct pin **, int);
extern void CheckForMixedNets(struct signal *);
extern void CheckOpenCollector(struct signal *);
extern void CheckForShorts(struct signal *);
extern void CheckDriverPins(struct signal *);
extern void PinWarning(struct instance *, struct pin *, int, char *);
extern void PinSignalWarning(struct pin *, struct signal *, char *);
extern void PinSignalSignalWarning(struct pin *, struct signal *, struct signal *, char *);
extern void SignalWarning(struct signal *, int, char *);
extern void ParanoidSignalWarning(struct signal *, char *);
extern void SimpleWarning(char *, char *);

void Trouble(char *, char *);
void Warning(char *, char *);
void Msg(char *, char *);
void Bug(char *);

void StaticCheck(void)
{
	(void) BuildPinArrays();
	(void) ChaseSignals();
} /* end StaticCheck */

void BuildPinArrays(void)
{
	struct chip *chipPointer;
	struct instance *instancePointer;
	for (chipPointer = firstChip;
	     chipPointer != NULL_STRUCT(chip);
	     chipPointer = chipPointer -> next)
	    if (chipPointer -> pin_definitions != NULL_STRUCT(pin_definitions))
		for (instancePointer = chipPointer -> first_instance;
		     instancePointer != NULL_STRUCT(instance);
		     instancePointer = instancePointer -> next)
			if ((chipPointer -> pin_definitions -> pin_count != 0) &&
			    (instancePointer != NULL_STRUCT(instance)) &&
			    (instancePointer -> pins == (struct pin **) 0))
			    instancePointer -> pins = (struct pin **)
			    calloc(chipPointer -> pin_definitions -> pin_count + 1, sizeof(struct pin *));
} /* end BuildPinArrays */

void ChaseSignals(void)
{
	struct signal *signalPointer;
	for (signalPointer = lastSignal;
	     signalPointer != NULL_STRUCT(signal);
	     signalPointer = signalPointer -> next)
		(void) CheckSignal(signalPointer);
} /* end ChaseSignals */

void CheckChips(void)
{
	struct chip *chipPointer;
	struct instance *instancePointer;
	struct pin *pinPointer;
	struct pin_definitions *definitionPointer;
	char *pinStringPointer;
	int pinNumber, pinType;

	for (chipPointer = firstChip;
	     chipPointer != NULL_STRUCT(chip);
	     chipPointer = chipPointer -> next)
	{
		definitionPointer = chipPointer -> pin_definitions;
		if (definitionPointer != NULL_STRUCT(pin_definitions))
		    for (instancePointer = chipPointer -> first_instance;
			 instancePointer != NULL_STRUCT(instance);
			 instancePointer = instancePointer -> next)
			    for (pinNumber = 0;
				 pinNumber < definitionPointer -> pin_count;
				 pinNumber++)
			    {
				pinPointer = instancePointer -> pins[pinNumber];
				pinStringPointer = definitionPointer -> pin_types;
				if (pinStringPointer != NULL) {
				    pinType = TypePin(pinStringPointer[pinNumber]);
				    if ((pinPointer == NULL_STRUCT(pin)) &&
				        (pinType & INPUT_PIN) &&
				        (!(pinType & (POWER_PIN | GROUND_PIN))))
					    (void) PinWarning(instancePointer, pinPointer, pinNumber, "input pin not connected");
				}
			    } /* end for */
	} /* end for */		
} /* end CheckChips */

void CheckSignal(struct signal *signalPointer)
{
	int pins;
	int pinType;
	char message[STRLEN];
	char *pinName;
	struct pin *thePin;
	struct pin_definitions *pinDefinitions;
	if (tFlag) printf("%s: #%d\n", signalPointer -> bucket -> name,
				signalPointer -> pin_count);
	pins = signalPointer -> pin_count;
	if (signalPointer -> type & IGNORE_SIGNAL) return;
	if (pins == 0) (void) Warning("net %s has no pins?", signalPointer -> bucket -> name);
	else
	if (pins == 1)
	{
		if (signalPointer -> type & MACRO_SIGNAL & mFlag)
			return;
		thePin = signalPointer -> first_pin;
		if (LFlag && (signalPointer -> bucket -> name[0] == '$'))
			return; /* ignore local nets */
		pinDefinitions = thePin -> instance -> chip -> pin_definitions;
		if (cFlag) return;
		if (xFlag)
		{
			pinName = "";
			pinType = NO_TYPE;
			if ((pinDefinitions != NULL_STRUCT(pin_definitions)) &&
			    (pinDefinitions -> pin_names) &&
			    (pinDefinitions -> pin_names[(thePin -> pin_number) - 1]))
			{
				pinName = pinDefinitions -> pin_names[(thePin -> pin_number) - 1];
				pinType = TypePin(pinDefinitions -> pin_types[(thePin -> pin_number) - 1]);
			}
			if (wFlag)
			    (void) sprintf(message, "lone%s pin at %s.%d (%s.%s) [%s]",
			    pinDirectionString[PinDirection(pinType)],
			    thePin -> instance -> bucket -> name,
			    thePin -> pin_number,
			    thePin -> instance -> chip -> bucket -> name,
			    pinName,
			    wFiles[thePin -> fileindex]);
			else
			    (void) sprintf(message, "lone%s pin at %s.%d (%s.%s)",
			    pinDirectionString[PinDirection(pinType)],
			    thePin -> instance -> bucket -> name,
			    thePin -> pin_number,
			    thePin -> instance -> chip -> bucket -> name,
			    pinName);
		} /* end if */
		else
			(void) sprintf(message, "lone%s pin at %s.%d",
			pinDirectionString[PinDirection(pinType)],
			thePin -> instance -> bucket -> name,
			thePin -> pin_number);
		(void) SignalWarning(signalPointer, FULL_PIN, message);
		(void) CheckForCollision(signalPointer, thePin);
	} /* end if */
	else
		(void) CheckSignalNet(signalPointer);
} /* end CheckSignal */

void CheckForCollision(struct signal *signalPointer, struct pin *pinPointer)
{
	int pinNumber, pinCount;
	struct pin **pinArray;
	pinNumber = pinPointer -> pin_number;
	pinArray = pinPointer -> instance -> pins;
	if (pinPointer -> instance -> chip -> pin_definitions)
	    pinCount = pinPointer -> instance -> chip -> pin_definitions -> pin_count;
	if (pinArray == (struct pin **) 0) return;

	if (pinNumber <= pinCount)
	    if (pinArray[pinNumber - 1] == (struct pin *) 0)
		pinArray[pinNumber - 1] = pinPointer;
	    else;
	else
	if (pinArray[pinNumber - 1])
		(void) PinSignalSignalWarning(pinPointer, signalPointer, pinArray[pinNumber - 1] -> signal, "two signals collide on pin");
	else	(void) PinSignalWarning(pinPointer, signalPointer, "pin number greater than package count");
} /* end CheckForCollision */			

int PinDirection(int type)
{
	return((type & INPUT_PIN)? 1 :
	       (type & OUTPUT_PIN)? 2 :
	       (type & (INPUT_PIN | OUTPUT_PIN))? 3 :
	       0);
} /* end PinDirection */

unsigned char PinType(struct pin *pinPointer, int printFlag)
{
	int pin_number;
	struct chip *chipPointer;
	unsigned char pin_type;

	pin_number = pinPointer -> pin_number;
	if (printFlag) fprintf(stdout, "\t%-15s\t%d",
			pinPointer -> instance -> bucket -> name,
			pin_number);
	chipPointer = pinPointer -> instance -> chip;
	pin_type = 0;
	if ((chipPointer -> pin_definitions != NULL_STRUCT(pin_definitions)) &&
	    (chipPointer -> pin_definitions -> pin_types != (char *) 0) &&
	    (pin_number <= strlen(chipPointer -> pin_definitions -> pin_types)))
	{
		pin_type = chipPointer -> pin_definitions -> pin_types[pin_number - 1];
		if (printFlag && chipPointer -> pin_definitions -> pin_names)
			fprintf(stdout, "\t(%s.%s [%c])",
				chipPointer -> bucket -> name,
				(chipPointer -> pin_definitions -> pin_names[pin_number - 1])?
				chipPointer -> pin_definitions -> pin_names[pin_number - 1] : "?",
				(pin_type == 0)? '?' : pin_type);
		if (printFlag & wFlag)
			fprintf(stdout, "\t\t[%s]", wFiles[pinPointer -> fileindex]);
	} /* end if */
	if (printFlag) fprintf(stdout, "\n");
	return(pin_type);
} /* end PinType */

void PrintSignal(struct signal *signalPointer, int mask)
{
	struct pin *pinPointer;
	if (!xFlag) return;
	for (pinPointer = signalPointer -> first_pin;
	     pinPointer != NULL_STRUCT(pin);
	     pinPointer = pinPointer -> instance_chain)
	{
		if (nFlag || pinPointer -> pin_type_vector & mask)
			(void) PinType(pinPointer, xFlag);
	} /* end for */
} /* end Print Signal */

void CheckSignalNet(struct signal *signalPointer)
{
	struct pin *pinPointer;
	char *signal_name;
	int pinType;
	char signalPins[MAXPERNET];

	inputPinCount = 0;
	outputPinCount = 0;
	sinkPinCount = 0;
	sourcePinCount = 0;
	analogPinCount = 0;
	undeclaredPinCount = 0;
	totalPinCount = 0;

	signal_name = signalPointer -> bucket -> name;
	if (cFlag)
	{
		fprintf(stdout, "%s:", signal_name);
		if (xFlag) printf("\n");
		else
		if (signalPointer -> pin_count > MAXPERNET)
			Bug("signal net too large in CheckSignalNet");
	} /* end if */
	for (pinPointer = signalPointer -> first_pin;
	     pinPointer != NULL_STRUCT(pin);
	     pinPointer = pinPointer -> instance_chain)
	{
		(void) CheckForCollision(signalPointer, pinPointer);
		pinType = PinType(pinPointer, cFlag & xFlag);
		if (cFlag & !xFlag)
			signalPins[totalPinCount] = pinType;
		else
			(void) DoPinType(pinPointer, signalPointer, pinType);
		totalPinCount++;
	} /* end for */
	if (cFlag)
	{
		if (!xFlag)
		{
			signalPins[totalPinCount] = '\0';
			fprintf(stdout, "\t%s\n", signalPins);
		} /* end if */
		if (pFlag) fprintf(stdout, "\t\t\t\t%di,%do,%ds,%dS,%da,%du\n",
					inputPinCount, outputPinCount,
					sinkPinCount, sourcePinCount,
					analogPinCount, undeclaredPinCount);
		return;
	} /* end if */
	if ((sinkPinCount > 0) && (sourcePinCount == 0) &&
	    (!PowerGroundNet(signalPointer)) && (!AnalogNet(signalPointer)))
		if (OpenCollectorNet(signalPointer))
			(void) ParanoidSignalWarning(signalPointer, "no o.c. pullup");
		else
			(void) ParanoidSignalWarning(signalPointer, "no source");
	if ((!PowerGroundNet(signalPointer)) && (inputPinCount > 0) && (outputPinCount == 0))
		(void) ParanoidSignalWarning(signalPointer, "no outputs");
	if ((!PowerGroundNet(signalPointer)) && (outputPinCount > 0) && (inputPinCount == 0))
		(void) ParanoidSignalWarning(signalPointer, "no inputs");
	(void) CheckSignalPins(signalPointer);
} /* end CheckSignalNet */

void DoPinType(struct pin *pinPointer, struct signal *signalPointer, unsigned char pinType)
{
	int pinTypeVector;
	char message[STRLEN];
	pinTypeVector = TypePin(pinType);
	switch (pinType)
	{
	case 0:
		undeclaredPins[undeclaredPinCount++] = pinPointer;
		break;
	case 'n':
		(void) PinSignalWarning(pinPointer, signalPointer, "no connection permitted");
		break;
	default:
		if (pinTypeVector == 0)
		{
			(void) sprintf(message, "unknown pin type %c", pinType);
			(void) PinSignalWarning(pinPointer, signalPointer, message);
		} /* end if */
	} /* end switch */
	if (pinTypeVector & INPUT_PIN)
		inputPins[inputPinCount++] = pinPointer;
	if (pinTypeVector & OUTPUT_PIN)
		outputPins[outputPinCount++] = pinPointer;
	if (pinTypeVector & SOURCE_PIN)
		sourcePins[sourcePinCount++] = pinPointer;
	if (pinTypeVector & SINK_PIN)
		sinkPins[sinkPinCount++] = pinPointer;
	if (pinTypeVector & ANALOG_PIN)
		analogPins[analogPinCount++] = pinPointer;
	pinPointer -> pin_type_vector = pinTypeVector;
} /* end DoPinType */

int TypePin(unsigned char pinType)
{
	int typeVector;
	typeVector = 0x0;
	switch(pinType)
	{
	case 0: /* undeclared */
		break;
	case '4': /* 3s output and input */
		typeVector |= INPUT_PIN | SINK_PIN;
	case '3': /* 3s output */
		typeVector |= OUTPUT_PIN | SOURCE_PIN | TRISTATE_PIN | DIGITAL_PIN;
		break;
	case '6': /* backplane pin */
		typeVector |= SOURCE_PIN;
	case '5': /* Open collector output and input */
		typeVector |= INPUT_PIN;
	case '1': /* Open collector output */
		typeVector |= OC_PIN | OUTPUT_PIN | SINK_PIN | DIGITAL_PIN;
		break;
	case '2': /* Totem pole output */
		typeVector |= TOTEM_POLE_PIN | OUTPUT_PIN | SOURCE_PIN | DIGITAL_PIN;
		break;
	case '0': /* Open collector and pullup */
		typeVector |= OC_PIN | OUTPUT_PIN | SOURCE_PIN | DIGITAL_PIN;
		break;
	case 'i': /* input */
	case 'k': /* pulldown input */
		typeVector |= INPUT_PIN | SINK_PIN | DIGITAL_PIN;
		break;
	case 'v': /* vcc - .vb 1 */
	case 'V':
	case 'w': /* .vb 2 */
	case 'W':
	case 'x': /* .vb 3 */
	case 'X':
	case 'y': /* .vb 4 */
	case 'Y':
	case 'z': /* .vb 5 */
	case 'Z':
		typeVector |= POWER_PIN | INPUT_PIN;
		break;
	case '9': /* misc voltage source */
		typeVector |= POWER_PIN;
	case 'p': /* pullup */
		typeVector |= OUTPUT_PIN | SOURCE_PIN | PULLUP_PIN | DIGITAL_PIN;
		break;
	case 'g': /* GND */
	case 'G':
		typeVector |= GROUND_PIN;
	case 'P': /* pulldown */
		typeVector |= SINK_PIN | DIGITAL_PIN;
		break;
	case 'j': /* pullup input */
		typeVector |= INPUT_PIN | SOURCE_PIN | SINK_PIN | DIGITAL_PIN;
		break;
	case 'A': /* analog input/output */
		typeVector |= INPUT_PIN;
	case '8': /* analog output */
		typeVector |= OUTPUT_PIN | ANALOG_PIN;
		break;
	case 'a': /* analog input */
		typeVector |= INPUT_PIN | ANALOG_PIN;
		break;
	case 'b': /* PAL input/output */
		typeVector |= INPUT_PIN | OUTPUT_PIN | SOURCE_PIN | DIGITAL_PIN;
		break;
	case 'B': /* bidirectional */
		typeVector |= BI_PIN;
		break;
	case 's': /* switch */
		typeVector |= BI_PIN | SOURCE_PIN | SINK_PIN | SWITCH_PIN;
		break;
	case 't': /* terminator */
		typeVector |= BI_PIN | SOURCE_PIN | SINK_PIN | TERMINATOR_PIN | DIGITAL_PIN;
		break;
	case 'r': /* receiver - */
		typeVector |= SINK_PIN | MINUS_PIN | INPUT_PIN;
		break;
	case 'R': /* receiver + */
		typeVector |= SINK_PIN | PLUS_PIN | INPUT_PIN;
		break;
	case 'd': /* xmtr - */
		typeVector |= SOURCE_PIN | MINUS_PIN | OUTPUT_PIN;
		break;
	case 'D': /* xmtr + */
		typeVector |= SOURCE_PIN | PLUS_PIN | OUTPUT_PIN;
		break;
	case 'I': /* current source */
		typeVector |= CURRENT_PIN | SOURCE_PIN;
		break;
	case 'J': /* current sink */
		typeVector |= CURRENT_PIN | SINK_PIN;
		break;
	case 'n': /* no connection permitted */
		break;
	case '.': /* no type */
		break;
	} /* end switch */
	return(typeVector);
} /* end TypePin */

void CheckSignalPins(struct signal *signalPointer)
{
	(void) CheckSources(signalPointer);
	(void) CheckSinks(signalPointer);
	(void) CheckBidirectional(signalPointer);
	(void) CheckTotemPole(signalPointer, outputPins, outputPinCount);
	(void) CheckForMixedNets(signalPointer);
	(void) CheckForShorts(signalPointer);
	(void) CheckDriverPins(signalPointer);
} /* end CheckSignalPins */

int PowerGroundNet(struct signal *signalPointer)
{
	int offset, i;
	char *signalName;
	signalName = signalPointer -> bucket -> name;
	offset = strlen(signalName) - 3;
	if (offset < 0) return(FALSE);
	for (i=0; powerGroundStrings[i]; i++)
	    if (strcmp((char *) signalName + offset, powerGroundStrings[i]) == 0)
		return(TRUE);
	return(FALSE);
} /* end PowerGroundNet */

int GroundNet(struct signal *signalPointer)
{
	int offset;
	char *signalName;
	signalName = signalPointer -> bucket -> name;
	offset = strlen(signalName) - 3;
	if (offset < 0) return(FALSE);
	if (strcmp((char *) signalName + offset, "GND") == 0)
		return(TRUE);
	return(FALSE);
} /* end GroundNet */

int AnalogNet(struct signal *net)
{
	return((analogPinCount != 0) ? TRUE : FALSE);
} /* end AnalogNet */

int OpenCollectorNet(struct signal *signalPointer)
{
	struct pin *pinPointer;
	int ocCount, pinType;
	ocCount = 0;
	for (pinPointer = signalPointer -> first_pin;
	     pinPointer != NULL_STRUCT(pin);
	     pinPointer = pinPointer -> instance_chain)
	{
		pinType = pinPointer -> pin_type_vector;
		if (pinType & OC_PIN) ocCount++;
	} /* end for */
	return(ocCount);
} /* end OpenCollectorNet */

void CheckSources(struct signal *signalPointer)
{
	int i, sourceCount, pullupCount, pinType;
	sourceCount = pullupCount = 0;
	for(i = 0; i < sourcePinCount; i++)
	{
		pinType = sourcePins[i] -> pin_type_vector;
		if (!(pinType & (TRISTATE_PIN | SWITCH_PIN | TERMINATOR_PIN))) sourceCount++;
		if (pinType & PULLUP_PIN) pullupCount++;
		if (GroundNet(signalPointer) && (pinType & OUTPUT_PIN) && !(pinType & INPUT_PIN))
			(void) PinSignalWarning(sourcePins[i], signalPointer, "output pin connected to ground");
	} /* end for */
	if (PowerGroundNet(signalPointer)) return;
	if (!AnalogNet(signalPointer) && ((sourceCount - pullupCount) > 1) && !sFlag) (void) SignalWarning(signalPointer, SOURCE_PIN, "too many sources");
} /* end CheckSources */

void CheckSinks(struct signal *signalPointer)
{
	int i, pinType, sinkCount, inputCount;
	char loadMessage[STRLEN];
	sinkCount = 0;
	inputCount = 0;
	for(i = 0; i < sinkPinCount; i++)
	{
		pinType = sinkPins[i] -> pin_type_vector;
		if (pinType & INPUT_PIN) inputCount++;
		else sinkCount++;
	} /* end for */
	if (PowerGroundNet(signalPointer)) return;
	if (inputCount >= max_loads)
	{
		(void) sprintf(loadMessage, "too many loads (%d >= %d)", inputCount, max_loads);
		(void) SignalWarning(signalPointer, INPUT_PIN, loadMessage);
	} /* end if */
} /* end CheckSinks */

void CheckBidirectional(struct signal *signalPointer)
{
	if (bFlag || AnalogNet(signalPointer) || PowerGroundNet(signalPointer)) return;
	if ((inputPinCount == 1) &&
	    ((inputPins[0] -> pin_type_vector & BI_PIN) == BI_PIN) &&
	    (outputPinCount >= 1))
		(void) SignalWarning(signalPointer, BI_PIN, "bidirectional net has only one receiver");
	if ((outputPinCount == 1) &&
	    ((outputPins[0] -> pin_type_vector & BI_PIN) == BI_PIN) &&
	    (inputPinCount >= 1))
		(void) SignalWarning(signalPointer, BI_PIN, "bidirectional net has only one transmitter");
} /* end CheckBidrectional */

void CheckTotemPole(struct signal *signalPointer, struct pin *pinArray[], int pinCount)
{
	int i, totemCount, pinType, tristate;
	for (i=0, totemCount=0, tristate=0; i<pinCount; i++)
	{
		pinType = pinArray[i] -> pin_type_vector;
		if (pinType & TOTEM_POLE_PIN) totemCount++;
		if (pinType & TRISTATE_PIN) tristate++;
	} /* end for */
	if (totemCount > 1)
		(void) SignalWarning(signalPointer, TOTEM_POLE_PIN, "too many totem pole drivers");
	if (totemCount && tristate)
		(void) SignalWarning(signalPointer, OUTPUT_PIN, "totem pole driver on tristate bus");
} /* end CheckTotemPole */

void CheckForMixedNets(struct signal *signalPointer)
{
	int analogCount, digitalCount,  pinType;
	struct pin *pinPointer;
	if (PowerGroundNet(signalPointer) || aFlag) return;
	analogCount = 0;
	digitalCount = 0;
	for (pinPointer = signalPointer -> first_pin;
	     pinPointer != NULL_STRUCT(pin);
	     pinPointer = pinPointer -> instance_chain)
	{
		pinType = pinPointer -> pin_type_vector;
		if (pinType & ANALOG_PIN) analogCount++;
		else
		if (pinType & DIGITAL_PIN) digitalCount++;
	} /* end for */
	if ((analogCount > 0) && (digitalCount > 0))
		(void) SignalWarning(signalPointer, FULL_PIN, "mixed analog and digital");
} /* end CheckForMixedNets */

void CheckOpenCollector(struct signal *signalPointer)
{
	int i, ocCount, sourceCount, pinType;
	ocCount=0;
	for (i=0; i<outputPinCount; i++)
	{
		pinType = outputPins[i] -> pin_type_vector;
		if (pinType & OC_PIN) ocCount++;
	} /* end for */
	sourceCount=0;
	for (i=0; i<sourcePinCount; i++)
	{
		pinType = sourcePins[i] -> pin_type_vector;
		if (pinType & SOURCE_PIN) sourceCount++;
	} /* end for */
	if ((ocCount >= 1) && (sourceCount == 0))
		(void) ParanoidSignalWarning(signalPointer, "no o.c. source");
} /* end CheckOpenCollector */

void CheckForShorts(struct signal *signalPointer)
{
	struct pin *pinPointer, *groundPinPointer, *voltagePinPointer;
	int groundPinCount, voltagePinCount, pinType;
	groundPinCount = voltagePinCount = 0;
	for (pinPointer = signalPointer -> first_pin;
	     pinPointer != NULL_STRUCT(pin);
	     pinPointer = pinPointer -> instance_chain)
	{
		pinType = pinPointer -> pin_type_vector;
		if (pinType & GROUND_PIN)
		{
			groundPinCount++;
			groundPinPointer = pinPointer;
		} /* end if */
		if (pinType & POWER_PIN)
		{
			voltagePinCount++;
			voltagePinPointer = pinPointer;
		} /* end if */
	} /* end for */
	if ((voltagePinCount == 0) || (groundPinCount == 0)) return;
	if (nFlag)
		(void) SignalWarning(signalPointer, POWER_PIN | GROUND_PIN, "power/ground short");
	else
		if (voltagePinCount <= groundPinCount)
			(void) PinSignalWarning(voltagePinPointer, signalPointer, "power to ground short");
		else	(void) PinSignalWarning(groundPinPointer, signalPointer, "ground to power short");
} /* end CheckForShorts */

void CheckDriverPins(struct signal *signalPointer)
{
	struct pin *pinPointer;
	int plusPinCount, minusPinCount, otherCount, pinCount, pinType;
	plusPinCount = minusPinCount = otherCount = pinCount = 0;
	for (pinPointer = signalPointer -> first_pin;
	     pinPointer != NULL_STRUCT(pin);
	     pinPointer = pinPointer -> instance_chain)
	{
		pinType = pinPointer -> pin_type_vector;
		if (pinType & PLUS_PIN) plusPinCount++;
		if (pinType & MINUS_PIN) minusPinCount++;
		if (pinType & (POWER_PIN|GROUND_PIN|SWITCH_PIN|TERMINATOR_PIN)) otherCount++;
		pinCount++;
	} /* end for */
	if ((plusPinCount == 0) && (minusPinCount == 0)) return;
	if ((plusPinCount + minusPinCount + otherCount) != pinCount)
		(void) SignalWarning(signalPointer, FULL_PIN, "mixed line driver/receiver net");
	if ((plusPinCount > 0) && (minusPinCount > 0))
		(void) SignalWarning(signalPointer, MINUS_PIN | PLUS_PIN, "polarity confusion on driver/receiver net");
} /* end CheckDriverPins */

void PinWarning(struct instance *instancePointer, struct pin *pinPointer, int pinNumber, char *message)
{
	char fullMessage[STRLEN];
	char *pin_name;
	pin_name = instancePointer -> chip -> pin_definitions -> pin_names[pinNumber];
	if (wFlag)
	(void) sprintf(fullMessage, "pin %s.%d (%s.%s) [%s]: %s",
		instancePointer -> bucket -> name,
		pinNumber + 1,
		instancePointer -> chip -> bucket -> name,
		pin_name ? pin_name : "",
		wFiles[pinPointer -> fileindex],
		message);
	else
	(void) sprintf(fullMessage, "pin %s.%d (%s.%s): %s",
		instancePointer -> bucket -> name,
		pinNumber + 1,
		instancePointer -> chip -> bucket -> name,
		pin_name ? pin_name : "",
		message);
	(void) SimpleWarning(fullMessage, NULL);
} /* end PinWarning */

void PinSignalWarning(struct pin *pinPointer, struct signal *signalPointer, char *message)
{
	char fullMessage[STRLEN];
	struct chip *theChip;
	theChip = pinPointer -> instance -> chip;
	if (xFlag)
	    if (wFlag)
		(void) sprintf(fullMessage, "net %s: pin %s.%d (%s.%s) [%s]: %s",
			signalPointer -> bucket -> name,
			pinPointer -> instance -> bucket -> name,
			pinPointer -> pin_number,
			theChip -> bucket -> name,
			(theChip -> pin_definitions -> pin_names)?
			theChip -> pin_definitions -> pin_names[pinPointer -> pin_number - 1] : 0,
			wFiles[pinPointer -> fileindex],
			message);
	    else
		(void) sprintf(fullMessage, "net %s: pin %s.%d (%s.%s): %s",
			signalPointer -> bucket -> name,
			pinPointer -> instance -> bucket -> name,
			pinPointer -> pin_number,
			theChip -> bucket -> name,
			(theChip -> pin_definitions -> pin_names)?
			    ((theChip -> pin_definitions -> pin_names[pinPointer -> pin_number - 1]) ?
			      theChip -> pin_definitions -> pin_names[pinPointer -> pin_number - 1]
			      : "?")
			: "?",
			message);
	else
	    if (wFlag)
		(void) sprintf(fullMessage, "net %s: pin %s.%d [%s]: %s",
			signalPointer -> bucket -> name,
			pinPointer -> instance -> bucket -> name,
			pinPointer -> pin_number,
			wFiles[pinPointer -> fileindex],
			message);
	    else
		(void) sprintf(fullMessage, "net %s: pin %s.%d: %s",
			signalPointer -> bucket -> name,
			pinPointer -> instance -> bucket -> name,
			pinPointer -> pin_number,
			message);
	(void) SimpleWarning(fullMessage, NULL);
} /* end PinSignalWarning */

void PinSignalSignalWarning(struct pin *pinPointer, struct signal *signalPointerA, struct signal *signalPointerB, char *message)
{
	char fullMessage[STRLEN];
	if (xFlag)
		(void) sprintf(fullMessage, "nets %s and %s: pin %s.%d (%s.%s): %s",
			signalPointerA -> bucket -> name,
			signalPointerB -> bucket -> name,
			pinPointer -> instance -> bucket -> name,
			pinPointer -> pin_number,
			pinPointer -> instance -> chip -> bucket -> name,
			(pinPointer -> instance -> chip -> pin_definitions -> pin_names)?
			pinPointer -> instance -> chip -> pin_definitions -> pin_names[pinPointer -> pin_number - 1] : 0,
			message);
	else
		(void) sprintf(fullMessage, "nets %s and %s: pin %s.%d: %s",
			signalPointerA -> bucket -> name,
			signalPointerB -> bucket -> name,
			pinPointer -> instance -> bucket -> name,
			pinPointer -> pin_number,
			message);
	(void) SimpleWarning(fullMessage, NULL);
} /* end PinSignalSignalWarning */

void SignalWarning(struct signal *signalPointer, int mask, char *message)
{
	char fullMessage[STRLEN];
	(void) sprintf(fullMessage, "net %s: %s",
		signalPointer -> bucket -> name,
		message);
	(void) SimpleWarning(fullMessage, NULL);
	if (xFlag) (void) PrintSignal(signalPointer, mask);
} /* end SignalWarning */

void ParanoidSignalWarning(struct signal *signalPointer, char *message)
{
	if (pFlag || (undeclaredPinCount == 0))
		(void) SignalWarning(signalPointer, FULL_PIN, message);
} /* end ParanoidSignalWarning */

void SimpleWarning(char *message, char *argument)
{
	fprintf(stdout, message, argument);
	fprintf(stdout, "\n");
	warningCount++;
} /* end SimpleWarning */
