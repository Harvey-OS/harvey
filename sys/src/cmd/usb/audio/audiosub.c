#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbaudio.h"
#include "usbaudioctl.h"

Namelist terminal_types[] = {
	{	0x100, "USB Terminal, undefined type"},
	{	0x101, "USB Streaming"},
	{	0x201, "Microphone"},
	{	0x301, "Speaker"},
	{	0x302, "Headphones"},
	{	0x401, "Handset"},			/* bi-directional */
	{	0x501, "Phone Line"},			/* analog telephone line jack */
	{	0x601, "Analog connector"},
	{	0x602, "Digital audio interface"},
	{	0x603, "Line connector"},		/* 3.5mm jack */
	{	0x604, "Legacy audio connector"},	/* input, connected to audio OUT */
	{	0x605, "S/PDIF"},
	{	0x606, "1394 DA stream"},
	{	0x607, "1394 DV stream soundtrack"},
	{	0x703, "CD player"},
	{	0x704, "DAT"},
	{	0x706, "Minidisk"},
	{	0x70b, "DVD Audio"},
	{	0x713, "Synthesizer"},
	{	0, nil }
};

/* units[8][16] stores rec and play and other unit's id,  K.Okamoto
	8: one of Play, Record, masterRecAGC, masterRecMute, LRRecVol, 
		LRPlayVol, masterPlayMute, masterPlayVol
	8: unit's ID
	1: unit attribute of readable, settable, chans, value[9] and min, max, step
*/
FeatureAttr units[8][16];
int nunits[8];		/* number in use for individual Unit's type K.Okamoto */
int nf;		/* current total number of featurelist K.Okamoto */

static int
findunit(int nr)
{
	int rec, i;

	if (nr == 12)		/* bSourceID == Mixer */
		return Play;	/* K.Okamoto */
	for (rec = 0; rec < 2; rec++)
		for (i = 0; i < nunits[rec]; i++)
			if (units[rec][i].id == nr)
				return rec;
	return -1;
}

void
audio_interface(Device *d, int n, ulong csp, void *bb, int nb) {
	byte *b = bb;
	int ctl, ch, u, x, nbchan, bmc;
	byte *p;
	int class, subclass, ifc, dalt;
	Audioalt *aa;

	if ((dalt = (csp >> 24) & 0xff) == 0xff) dalt = -1;
	if ((ifc = (csp >> 16) & 0xff) == 0xff) ifc = -1;
	class = Class(csp);
	subclass = Subclass(csp);

	if (debug & Dbginfo) fprint(2, "%d.%d: ", class, subclass);
	switch (subclass) {
	case 1:	// control
		switch (b[2]) {
		case 0x01:	/* class-specific AC interface header */
			if (debug & Dbginfo){
				fprint(2, "Class-Specific AC Interface Header Descriptor\n");
				fprint(2, "\tAudioDeviceClass release (bcd)%c%c%c%c, TotalLength %d, InCollection %d aInterfaceNr %d %d",
					'0'+((b[4]>>4)&0xf), '0'+(b[4]&0xf),
					'0'+((b[3]>>4)&0xf), '0'+(b[3]&0xf),
					b[5]|(b[6]<<8), b[7], 1, b[8]);
				if(b[7]-1)		/* K.Okamoto */
					for(ctl=1;ctl<b[7];ctl++) 
						fprint(2,", aInterfaceNr%d %d", ctl+1, b[8+ctl]);
				fprint(2, "\n");
			}
			break;
		case 0x02:	// input
			if (debug & Dbginfo){
				fprint(2, "Audio Input Terminal Descriptor\n");
				fprint(2, "\tbTerminalId %d, wTerminalType 0x%x (%s), bAssocTerminal %d bNrChannels %d, wChannelConfig 0x%x, iChannelNames %d iTerminal %d\n",
					b[3], b[4]|(b[5]<<8),
					namefor(terminal_types, b[4]|(b[5]<<8)),
					b[6], b[7], b[8]|(b[9]<<8), b[10], b[11]);
			}
			if ((b[4]|b[5]<<8) == 0x101){	/* b[4]|b[5]<<8 = wTerminalType */
				if (verbose)
					fprint(2, "Audio output unit %d\n", b[3]);
				/* USB streaming input: play interface */
				units[Play][nunits[Play]++].id = b[3];	/* b[3] = bTerminalId */
			}else{
				if (verbose)
					fprint(2, "Device can record from %s\n",
						namefor(terminal_types, b[4]|(b[5]<<8)));
				/* Non-USB input: record interface */
				units[Record][nunits[Record]++].id = b[3];
			}
			break;
		case 0x03:	// output
			if(debug & Dbginfo){
				fprint(2, "Audio Output Terminal Descriptor\n");
				fprint(2, "\tbTerminalId %d, wTerminalType 0x%x (%s), bAssocTerminal %d bSourceId %d, iTerminal %d\n",
					b[3], b[4]|(b[5]<<8),
					namefor(terminal_types, b[4]|(b[5]<<8)),
					b[6], b[7], b[8]);
			}
			if((b[4]|b[5]<<8) == 0x101){
				if (verbose)
					fprint(2, "Audio input unit %d\n", b[3]);
				/* USB streaming output: record interface */
				units[Record][nunits[Record]++].id = b[3];
			}else{
				if (verbose)
					fprint(2, "Device can play to %s\n",
						namefor(terminal_types, b[4]|(b[5]<<8)));
				/* Non-USB output: play interface */
				units[Play][nunits[Play]++].id = b[3];
			}
			break;
		case 0x04:	/* mixer */
			if (debug & Dbginfo){			/* K.Okamoto */
				fprint(2, "Audio Mixer Unit %d\n", b[3]);
				fprint(2, "\tbNrInPins %d, ", b[4]);
				for(ctl = 1;ctl <= b[4]; ctl++)
					fprint(2, "Input Pin %d's UnitID %d, ", ctl, b[4+ctl]);
				ctl--;
				nbchan = b[5+ctl];
				fprint(2, "bNrChannels %d, ", nbchan);
				fprint(2, "wChannelConfig 0x%x, ", b[6+ctl]|b[7+ctl]<<8);
				fprint(2, "iChannelNames %d, ", b[8+ctl]);
				bmc = 9 + ctl;
				fprint(2, "bmControls 0x%x", b[bmc++]);
				nbchan = b[0]-bmc-1;
				if(nbchan > 1)
					while(nbchan--) fprint(2, "%x", b[bmc++]);
				fprint(2, ", iMixer %d\n", b[bmc]);
			}
			if (verbose)
				fprint(2, "Audio Mixer Unit %d\n", b[3]);
			mixerid = b[3];
			break;
		case 0x05:	/* selector */
			if(verbose)
				fprint(2, "Audio Selector Unit %d\n", b[3]);
			if(debug & Dbginfo)
				fprint(2, "\tbUnitId %d, bNrInPins %d", b[3], b[4]);
			selector = b[3];		/* K.Okamoto */
			if(b[4]){	/* number of Pins >=1, K.Okamoto */
				if (debug & Dbginfo) fprint(2, ", baSourceIDs: [%d", b[5]);
				u = findunit(b[5]);
				for (ctl = 1; ctl < b[4]; ctl++){
					if (u < 0)
						u = findunit(b[5+ctl]);
					else if ((x = findunit(b[5+ctl])) >= 0 && u != x && verbose)
						fprint(2, "\tSelector %d for output AND input\n", b[3]);
					if (debug & Dbginfo) fprint(2, ", %d", b[5+ctl]);
				}
				if (debug & Dbginfo) fprint(2, "]\n");
				if (u >= 0){
					if (selectorid[u] >= 0)
						fprint(2, "Second selector (%d, %d) on %s\n", selectorid[u], b[3], u?"record":"playback");
					selectorid[u] = b[3];
				}
			}
			break;
		case 0x06:
			/* feature, b[6]: master channel, b[7]: logical channel 1, 2, 3, .. -->
			 * b[5]: byte size of individual element data size
			 * b[3] id number, b[4]: source ID, 
			 */
			if (verbose) fprint(2, "Audio Feature Unit %d", b[3]);
			if (debug & Dbginfo)
				fprint(2, "\tbUnitId %d, bSourceId %d, bControlSize %d\n",
					b[3], b[4], b[5]);
			u = findunit(b[4]);
			if (u >= 0){
				if (verbose) fprint(2, " for %s\n", u?"Record":"Playback");
//				units[u][nunits[u]++].id = b[3];
				if (featureid[u] >= 0)
					if (verbose) fprint(2, "Second feature unit (%d, %d) on %s\n", featureid[u], b[3], u?"record":"playback");
				featureid[u] = b[3];
			}else
				if (verbose) fprint(2, ", not known what for\n");
			p = b + 6;	/* p: each logical channel data */
			/* check first for master channel */
			for(ctl = 1; ctl < 0x0b; ctl++){
				if((1<<(ctl-1)) & (b[6] | ((b[5]>1)?(b[7]<<8):0))){
					/* b[6]: master channel control,
					 * D0: Mute, D1: Volume, D2: Bass, D3: Mid, 
					 * D4: Treble, D5: Equalizer, D6: Agc, D7: Delay,
					 * D8: Bass Boost, D9: Loudness ...
					 */
					if (ctl == 1 && u == Record){
						units[masterRecMute][nunits[masterRecMute]].id = b[3];
						units[masterRecMute][nunits[masterRecMute]].readable =1;
						units[masterRecMute][nunits[masterRecMute]].settable = 1;
						units[masterRecMute][nunits[masterRecMute]++].chans = 0;
						controls[u][ctl].readable = 1;
						controls[u][ctl].settable = 1;
						controls[u][ctl].chans = 0;
					} else if (ctl == 1 && u == Play) {
						units[masterPlayMute][nunits[masterPlayMute]].id = b[3];
						units[masterPlayMute][nunits[masterPlayMute]].readable = 1;
						units[masterPlayMute][nunits[masterPlayMute]].settable = 1;
						units[masterPlayMute][nunits[masterPlayMute]++].chans = 0;
						controls[u][ctl].readable = 1;
						controls[u][ctl].settable = 1;
						controls[u][ctl].chans = 0;
					} else if (ctl == 2 && u == Play) {
						units[masterPlayVol][nunits[masterPlayVol]].id = b[3];
						units[masterPlayVol][nunits[masterPlayVol]].readable = 1;
						units[masterPlayVol][nunits[masterPlayVol]].settable = 1;
						units[masterPlayVol][nunits[masterPlayVol]++].chans = 0;
						controls[u][ctl].readable = 1;
						controls[u][ctl].settable = 1;
						controls[u][ctl].chans = 0;
					} else if (ctl == 7 && u == Record){ 	/* K.Okamoto for CM106/F */
						units[masterRecAGC][nunits[masterRecAGC]].id = b[3];
						units[masterRecAGC][nunits[masterRecAGC]].readable = 1;
						units[masterRecAGC][nunits[masterRecAGC]].settable = 1;
						units[masterRecAGC][nunits[masterRecAGC]++].chans = 0;
						controls[u][ctl].readable = 1;
						controls[u][ctl].settable = 1;
						controls[u][ctl].chans = 0;
					}
					if (verbose)
						fprint(2, "\t%s control on master channel\n",
							controls[0][ctl].name);
				}
			}
			/* Now going to check each logical channel */
			p += (b[5]>1)?2:1;
			for (ch = 0; ch < (nb - 8)/b[5]; ch++) {
				for (ctl = 1; ctl < 0x0b; ctl++) {
					/* p[0]: each channel control,
					 * D0: Mute, D1: Volume, D2: Bass, D3: Mid, 
					 * D4: Treble, D5: Equalizer, D6: Agc, D7: Delay,
					 * D8: Bass Boost, D9: Loudness ...
					 */
					if ((1<<(ctl-1)) & (p[0] | ((b[5]>1)?(p[1]<<8):0))) {
						if (ctl == 2 && u == Play ) {
							if (!ch) {
								units[LRPlayVol][nunits[LRPlayVol]].id = b[3];
								units[LRPlayVol][nunits[LRPlayVol]].readable = 1;
								units[LRPlayVol][nunits[LRPlayVol]].settable = 1;
							}
							units[LRPlayVol][nunits[LRPlayVol]].chans |= 1<<ch;
						}
						else if (ctl == 2 && u == Record) {
							if (!ch) {
								units[LRRecVol][nunits[LRRecVol]].id = b[3];
								units[LRRecVol][nunits[LRRecVol]].readable = 1;
								units[LRRecVol][nunits[LRRecVol]].settable = 1;
							}
							units[LRRecVol][nunits[LRRecVol]].chans |= 1<<ch;
						}

/* Why we have to set these flags? K.Okamoto */
						controls[u][ctl].readable = 1;
						controls[u][ctl].settable = 1;
						controls[u][ctl].chans |= 1<<ch;
/**/
						if (verbose)
							fprint(2, "\t%s control on channel %d\n",
								controls[0][ctl].name, ch+1);
					}
				}
				p += (b[5]>1)?2:1;
			}
			if (units[LRRecVol][nunits[LRRecVol]].id) nunits[LRRecVol]++;
			if (units[LRPlayVol][nunits[LRPlayVol]].id) nunits[LRPlayVol]++;
			if (debug & Dbginfo) {	/* K.Okamoto */
				fprint(2, "Number of Play Units = %d, and consists of ", nunits[Play]);
				for(x=0; x<nunits[Play]; x++) fprint(2, "%d, ", units[Play][x].id);
				fprint(2, "\nNumber of Record Units = %d, and consists of ", nunits[Record]);
				for(x=0; x<nunits[Record]; x++) fprint(2, "%d, ", units[Record][x].id);
				fprint(2, "\nNumber of masterRecAGC Units = %d, and consists of ", nunits[masterRecAGC]);
				for(x=0; x<nunits[masterRecAGC]; x++) 
					if (units[masterRecAGC][x].id) fprint(2, "%d, ", units[masterRecAGC][x].id);
				fprint(2, "\nNumber of masterRecMute Units = %d, and consists of ", nunits[masterRecMute]);
				for(x=0; x<nunits[masterRecMute]; x++) 
					if (units[masterRecMute][x].id) fprint(2, "%d, ", units[masterRecMute][x].id);
				fprint(2, "\nNumber of LRRecVol Units = %d, and consists of ", nunits[LRRecVol]);
				for(x=0; x<nunits[LRRecVol]; x++) 
					if (units[LRRecVol][x].id) fprint(2, "%d, ", units[LRRecVol][x].id);
				fprint(2, "\nNumber of masterPlayMute Units = %d, and consists of ", nunits[masterPlayMute]);
				for(x=0; x<nunits[masterPlayMute]; x++) 
					if (units[masterPlayMute][x].id) fprint(2, "%d, ", units[masterPlayMute][x].id);
				fprint(2, "\nNumber of masterPlayVol Units = %d, and consists of ", nunits[masterPlayVol]);
				for(x=0; x<nunits[masterPlayVol]; x++)
					if (units[masterPlayVol][x].id) fprint(2, "%d, ", units[masterPlayVol][x].id);
				fprint(2, "\nNumber of LRPlayVol Units = %d, and consists of ", nunits[LRPlayVol]);
				for(x=0; x<nunits[LRPlayVol]; x++) 
					if (units[LRPlayVol][x].id) fprint(2, "%d, ", units[LRPlayVol][x].id);
				fprint(2, "\n");
			}
			break;
		default:
			pcs_raw("audio control unknown", bb, nb);
		}
		break;
	case 2: // stream
		switch (b[2]) {
		case 0x01:
			if (debug & Dbginfo)
				fprint(2, "Audio stream for TerminalID %d, delay %d, format_tag %#ux\n",
					b[3], b[4], b[5] | (b[6]<<8));
			break;
		case 0x02:
			if (d->config[n]->iface[ifc]->dalt[dalt] == nil)
				d->config[n]->iface[ifc]->dalt[dalt] = mallocz(sizeof(Dalt),1);
			aa = (Audioalt *)d->config[n]->iface[ifc]->dalt[dalt]->devspec;
			if (aa == nil) {
				aa = mallocz(sizeof(Audioalt), 1);
				d->config[n]->iface[ifc]->dalt[dalt]->devspec = aa;
			}
			if (verbose){
				if (b[4] <= 2)
					fprint(2, "Interface %d, alt %d: %s, %d bits, ",
						ifc, dalt, (b[4] == 1)?"mono":"stereo", b[6]);
				else
					fprint(2, "Interface %d, alt %d: %d channels, %d bits, ",
						ifc, dalt, b[4], b[6]);
			}
			if(b[7] == 0){
				if (verbose)
					fprint(2, "frequency variable between %d and %d\n",
						b[8] | b[9]<<8 | b[10]<<16, b[11] | b[12]<<8 | b[13]<<16);
				aa->minfreq = b[8] | b[9]<<8 | b[10]<<16;
				aa->maxfreq = b[11] | b[12]<<8 | b[13]<<16;
				aa->caps |= has_contfreq;
			}else{
				if (verbose)
					fprint(2, "discrete frequencies are:");
				for (ch = 0; ch < b[7] && ch < 8; ch++){
					aa->freqs[ch] = b[8+3*ch] | b[9+3*ch]<<8 | b[10+3*ch]<<16;
					if (verbose)
						fprint(2, " %d", b[8+3*ch] | b[9+3*ch]<<8 | b[10+3*ch]<<16);
				}
				if (ch < 8)
					aa->freqs[ch] = -1;
				if (verbose)
					fprint(2, "\n");
				if (ch > 1)
					aa->caps |= has_discfreq;	/* more than one frequency */
				else
					aa->caps |= onefreq;		/* only one frequency */
			}
			aa->nchan = b[4];
			aa->res = b[6];
			aa->subframesize = b[5];
			break;
		default:
			if (debug & Dbginfo)
				pcs_raw("audio stream unknown", bb, nb);
		}
		break;
	case 3: // midi
	default:
		if (debug & Dbginfo){
			fprint(2, "Unknown audio stream type: ");
			pcs_raw("CS_INTERFACE", bb, nb);
		}
	}
}
