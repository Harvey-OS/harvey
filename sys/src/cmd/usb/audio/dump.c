#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"
#include "dat.h"
#include "fns.h"
#include "audioclass.h"

static char *unittype[] =
{
	[INPUT_TERMINAL]		"Input Terminal",
	[OUTPUT_TERMINAL]	"Output Terminal",
	[MIXER_UNIT]			"Mixer Unit",
	[SELECTOR_UNIT]		"Selector Unit",
	[FEATURE_UNIT]		"Feature Unit",
	[PROCESSING_UNIT]		"Processing Unit",
	[EXTENSION_UNIT]		"Extension Unit",
};

typedef struct Namelist Namelist;
struct Namelist
{
	short	index;
	char		*name;
};

static Namelist termtypes[] =
{
	{ USB_UNDEFINED, "USB Terminal, undefined type"},
	{ USB_STREAMING, "USB Streaming"},
	{ USB_VENDOR, "USB Vendor-Specific"},

	{ INPUT_UNDEFINED, "Input, undefined type"},
	{ MICROPHONE, "Microphone"},
	{ DESK_MICROPHONE, "Desk Microphone"},
	{ PERSONAL_MICROPHONE, "Personal Microphone"},
	{ OMNI_MICROPHONE, "Omni-directional Microphone"},
	{ MICROPHONE_ARRAY, "Microphone Array"},
	{ PROC_MICROPHONE_ARRAY, "Processing Microphone Array"},

	{ OUTPUT_UNDEFINED, "Output Undefined"},
	{ SPEAKER, "Speaker"},
	{ HEADPHONES, "Headphones"},
	{ HMD_AUDIO, "Head Mounted Display Audio"},
	{ DESK_SPEAKER, "Desk Speaker"},
	{ ROOM_SPEAKER, "Room Speaker"},
	{ COMM_SPEAKER, "Communication Speaker"},
	{ LFE_SPEAKER, "Low Frequency Effects Speaker"},

	{ BIDIRECTIONAL_UNDEFINED, "Bi-directional Undefined"},
	{ HANDSET, "Handset"},
	{ HEADSET, "Headset"},
	{ SPEAKERPHONE, "Speakerphone"},
	{ ECHO_SUPP_SPEAKERPHONE, "Echo-suppressing Speakerphone"},
	{ ECHO_CANC_SPEAKERPHONE, "Echo-cancelling Speakerphone"},

	{ TELEPHONY_UNDEFINED, "Telephony Undefined"},
	{ PHONELINE, "Phone Line"},
	{ TELEPHONE, "Telephone"},
	{ DOWN_LINE_PHONE, "Down Line Phone"},

	{ EXTERNAL_UNDEFINED, "External Undefined"},
	{ ANALOG_CONNECTOR, "Analog Connector"},
	{ DIGITAL_AUDIO, "Digital Audio Interface"},
	{ LINE_CONNECTOR, "Line Connector"},
	{ LEGACY_AUDIO, "Legacy Audio Connector"},
	{ SPDIF_INTERFACE, "S/PDIF Interface"},
	{ I1394_DA, "1394 DA Stream"},
	{ I1394_DV, "1394 DV Stream Soundtrack"},

	{ EMBEDDED_UNDEFINED, "Embedded Undefined"},
	{ LEVEL_CALIBRATION_NOICE_SOURCE, "Level Calibration Noise Source"},
	{ EQUALIZATION_NOISE, "Equalization Noise"},
	{ CD_PLAYER, "CD Player"},
	{ DAT, "DAT"},
	{ DCC, "DCC"},
	{ MINIDISK, "MiniDisk"},
	{ ANALOG_TAPE, "Analog Tape"},
	{ PHONOGRAPH, "Phonograph"},
	{ VCR_AUDIO, "VCR Audio"},
	{ VIDEODISC_AUDIO, "Video Disc Audio"},
	{ DVD_AUDIO, "DVD Audio"},
	{ TVTUNER_AUDIO, "TV Tuner Audio"},
	{ SATELLITE_AUDIO, "Satellite Receiver Audio"},
	{ CABLETUNER_AUDIO, "Cable Tuner Audio"},
	{ DSS_AUDIO, "DSS Audio"},
	{ RADIO_RECEIVER, "Radio Receiver"},
	{ RADIO_TRANSMITTER, "Radio Transmitter"},
	{ MULTITRACK_RECORDER, "Multi-track Recorder"},
	{ SYNTHESIZER, "Synthesizer"},

	{ 0, nil }
};

static Namelist featcontrols[] =
{
	{ MUTE_CONTROL-1, "Mute"},
	{ VOLUME_CONTROL-1, "Volume"},
	{ BASS_CONTROL-1, "Bass"},
	{ MID_CONTROL-1, "Mid"},
	{ TREBLE_CONTROL-1, "Treble"},
	{ EQUALIZER_CONTROL-1, "Equalizer"},
	{ AGC_CONTROL-1, "AGC"},
	{ DELAY_CONTROL-1, "Delay"},
	{ BASS_BOOST_CONTROL-1, "Bass Boost"},
	{ LOUDNESS_CONTROL-1, "Loudness"},

	{ 0, nil }
};

static Namelist formats[] =
{
	/* type 1 */
	{ PCM, "PCM"},
	{ PCM8, "PCM8"},
	{ IEEE_FLOAT, "IEEE_FLOAT"},
	{ ALAW, "ALAW"},
	{ MULAW, "MULAW"},

	/* type 2 */
	{ MPEG, "MPEG"},
	{ AC_3, "AC-3"},

	/* type 3 */
	{ IEC1937_AC_3, "IEC1937 AC-3"},
	{ IEC1937_MPEG1_L1, "IEC1937 MPEG-1 Layer 1"},
	{ IEC1937_MPEG1_L2_3, "IEC1937 MPEG-1 Layer 2/3"},
	{ IEC1937_MPEG2_NOEXT, "IEC1937 MPEG-2 NOEXT"},
	{ IEC1937_MPEG2_EXT, "IEC1937 MPEG-2 EXT"},
	{ IEC1937_MPEG2_L1_LS, "IEC1937 MPEG-2 Layer 1_LS"},
	{ IEC1937_MPEG2_L2_3_LS, "IEC1937 MPEG-2 Layer 2/3_LS"},

	{ 0, nil }
};

static Namelist spatials[] =
{
	{ 0, "Left Front"},
	{ 1, "Right Front"},
	{ 2, "Center Front"},
	{ 3, "Low Frequency Enhancement"},
	{ 4, "Left Surround"},
	{ 5, "Right Surround"},
	{ 6, "Left of Center"},
	{ 7, "Right of Center"},
	{ 8, "Surround"},
	{ 9, "Side Left"},
	{ 10, "Side Right"},
	{ 11, "Top"},

	{ 0, nil }
};

static char *
namefor(Namelist *list, int item)
{
	while (list->name){
		if (list->index == item)
			return list->name;
		list++;
	}
	return "<unnamed>";
}

static void
dumpbits(Namelist *list, int bits)
{
	int i, spoke;

	fprint(2, "{");
	spoke = 0;
	for(i = 0; i < 8*sizeof(int); i++) {
		if(bits & (1<<i)) {
			fprint(2, "%s%s", spoke ? ", " : " ", namefor(list, i));
			spoke = 1;
		}
	}
	fprint(2, " }\n");
}

void
dumpaudiofunc(Audiofunc *af)
{
	int i;
	Unit *u;
	Dalt *dalt;
	Dinf *intf;
	Stream *s;
	Funcalt *falt;
	Streamalt *salt;

	intf = af->intf;
	fprint(2, "audio function on %D config %d interface %d\n", intf->d, intf->conf->x, intf->x);
	for(falt = af->falt; falt != nil; falt = falt->next) {
		fprint(2, "\tcontrol alt %d, %d streams\n", falt->dalt->alt, falt->nstream);
		for(u = falt->unit; u != nil; u = u->next) {
			fprint(2, "\t\tunit id %d: %s\n", u->id, unittype[u->type]);
			fprint(2, "\t\t\t%d channels, spatial location mask %.4x\n", u->nchan, u->chanmask);
			fprint(2, "\t\t\t");
			dumpbits(spatials, u->chanmask);
			switch(u->type) {
			case INPUT_TERMINAL:
			case OUTPUT_TERMINAL:
				fprint(2, "\t\t\tterminal type %.4x: %s\n", u->termtype, namefor(termtypes, u->termtype));
				if(u->assoc != 0)
					fprint(2, "\t\t\tassociated terminal id %d\n", u->assoc);
				break;
			case FEATURE_UNIT:
				fprint(2, "\t\t\tmaster hascontrol %.4x ", u->hascontrol[0]);
				dumpbits(featcontrols, u->hascontrol[0]);
				for(i = 1; i <= u->nchan; i++) {
					fprint(2, "\t\t\tchannel %d hascontrol %.4x ", i, u->hascontrol[i]);
					dumpbits(featcontrols, u->hascontrol[i]);
				}
				break;
			case PROCESSING_UNIT:
				fprint(2, "\t\t\tprocessing type %.4x\n", u->proctype);
				break;
			case EXTENSION_UNIT:
				fprint(2, "\t\t\textension type %.4x\n", u->exttype);
				break;
			}
			for(i = 0; i < u->nsource; i++)
				fprint(2, "\t\t\tsource id %d\n", u->sourceid[i]);
		}
		for(i = 0; i < falt->nstream; i++) {
			s = falt->stream[i];
			fprint(2, "\t\tstream %d\n", s->id);
		}
	}
	for(s = af->streams; s != nil; s = s->next) {
		fprint(2, "\tstream id %d on interface %d\n", s->id, s->intf->x);
		for(salt = s->salt; salt != nil; salt = salt->next) {
			dalt = salt->dalt;
			fprint(2, "\t\talt %d: ", dalt->alt);
			if(dalt->npt == 0 || dalt->ep[0].maxpkt == 0) {
				fprint(2, "disabled\n");
				continue;
			}
			fprint(2, "delay %dms format %.4x (%s) channel %d subframe %d res %d\n",
				salt->delay, salt->format, namefor(formats, salt->format), salt->nchan, salt->subframe, salt->res);
			if(salt->nf == 0)
				fprint(2, "\t\t\tfrequency range %d-%d\n", salt->minf, salt->maxf);
			else {
				fprint(2, "\t\t\tfrequencies:");
				for(i = 0; i < salt->nf; i++)
					fprint(2, " %d", salt->f[i]);
				fprint(2, "\n");
			}
		}
	}
}
