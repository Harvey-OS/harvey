#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "audio.h"
#include "audioctl.h"

typedef struct Namelist Namelist;

struct Namelist
{
	short	index;
	char		*name;
};

Namelist terminal_types[] = {
	{	0x100, "USB Terminal, undefined type"},
	{	0x101, "USB Streaming"},
	{	0x201, "Microphone"},
	{	0x301, "Speaker"},
	{	0x603, "Line connector"},
	{	0x605, "S/PDIF"},
	{	0, nil }
};

units[2][8];	/* rec and play units */
nunits[2];		/* number in use */

char *
namefor(Namelist *list, int item)
{
	while(list->name){
		if(list->index == item)
			return list->name;
		list++;
	}
	return "<unnamed>";
}

static int
findunit(int nr)
{
	int rec, i;
	for(rec = 0; rec < 2; rec++)
		for(i = 0; i < nunits[rec]; i++)
			if(units[rec][i] == nr)
				return rec;
	return -1;
}

void
audio_interface(Dev *, Desc *dd)
{
	byte *b = (uchar*)&dd->data;
	byte *bb = b;
	int nb = dd->data.bLength;
	int ctl, ch, u, x;
	byte *p;
	int class, subclass;
	Audioalt *aa;
	char *hd;

	class = Class(dd->iface->csp);
	subclass = Subclass(dd->iface->csp);

	dprint(2, "%d.%d: ", class, subclass);
	switch (subclass){
	case 1:	// control
		switch (b[2]){
		case 0x01:
			dprint(2, "Class-Specific AC Interface Header Descriptor\n");
			dprint(2, "\tAudioDevClass release (bcd)%c%c%c%c, "
				"TotalLength %d, InCollection %d aInterfaceNr %d\n",
					'0'+((b[4]>>4)&0xf), '0'+(b[4]&0xf),
					'0'+((b[3]>>4)&0xf), '0'+(b[3]&0xf),
					b[5]|(b[6]<<8), b[7], b[8]);
			break;
		case 0x02:	// input
			dprint(2, "Audio Input Terminal Descriptor\n");
			dprint(2, "\tbTerminalId %d, wTerminalType "
				"0x%x (%s), bAssocTerminal %d bNrChannels %d, "
				"wChannelConfig %d, iChannelNames %d iTerminal %d\n",
					b[3], b[4]|(b[5]<<8),
					namefor(terminal_types, b[4]|(b[5]<<8)),
					b[6], b[7], b[8]|(b[9]<<8), b[10], b[11]);
			if((b[4]|b[5]<<8) == 0x101){
				if(verbose)
					fprint(2, "Audio output unit %d\n", b[3]);
				/* USB streaming input: play interface */
				units[Play][nunits[Play]++] = b[3];
			}else{
				if(verbose)
					fprint(2, "Dev can record from %s\n",
						namefor(terminal_types, b[4]|(b[5]<<8)));
				/* Non-USB input: record interface */
				units[Record][nunits[Record]++] = b[3];
			}
			break;
		case 0x03:	// output
			if(usbdebug){
				fprint(2, "Audio Output Terminal Descriptor\n");
				fprint(2, "\tbTerminalId %d, wTerminalType 0x%x (%s), bAssocTerminal %d bSourceId %d, iTerminal %d\n",
					b[3], b[4]|(b[5]<<8),
					namefor(terminal_types, b[4]|(b[5]<<8)),
					b[6], b[7], b[8]);
			}
			if((b[4]|b[5]<<8) == 0x101){
				if(verbose)
					fprint(2, "Audio input unit %d\n", b[3]);
				/* USB streaming output: record interface */
				units[Record][nunits[Record]++] = b[3];
				if(verbose)
					fprint(2, "Dev can play to %s\n",
						namefor(terminal_types, b[4]|(b[5]<<8)));
				/* Non-USB output: play interface */
				units[Play][nunits[Play]++] = b[3];
			}
			break;
		case 0x04:
			if(verbose)
				fprint(2, "Audio Mixer Unit %d\n", b[3]);
			if(usbdebug){
				fprint(2, "\t%d bytes:", nb);
				for(ctl = 0; ctl < nb; ctl++)
					fprint(2, " 0x%2.2x", b[ctl]);
				fprint(2, "\n\tbUnitId %d, bNrInPins %d", b[3], b[4]);
			}
			if(b[4]){
				dprint(2, ", baSourceIDs: [%d", b[5]);
				u = findunit(b[5]);
				for(ctl = 1; ctl < b[4]; ctl++){
					if(u < 0)
						u = findunit(b[5+ctl]);
					else if((x = findunit(b[5+ctl])) >= 0 && u != x && verbose)
						fprint(2, "\tMixer %d for I & O \n", b[3]);
					dprint(2, ", %d", b[5+ctl]);
				}
				dprint(2, "]\n");
				if(u >= 0){
					units[u][nunits[u]++] = b[3];
					if(mixerid[u] >= 0)
						fprint(2, "Second mixer (%d, %d) on %s\n",
						 mixerid[u], b[3], u?"record":"playback");
					mixerid[u] = b[3];
				}
				if(usbdebug){
					fprint(2, "Channels %d, config %d, ",
						b[ctl+5], b[ctl+5+1] | b[ctl+5+2] << 8);
					x = b[ctl+5] * b[4];
					fprint(2, "programmable: %d bits, 0x", x);
					x = (x + 7) >> 3;
					while(x--)
						fprint(2, "%2.2x", b[ctl+x+5+4]);
				}
			}
			break;
		case 0x05:
			if(verbose)
				fprint(2, "Audio Selector Unit %d\n", b[3]);
			dprint(2, "\tbUnitId %d, bNrInPins %d", b[3], b[4]);
			if(b[4]){
				u = findunit(b[5]);
				dprint(2, ", baSourceIDs: %s [%d",
					u?"record":"playback", b[5]);
				for(ctl = 1; ctl < b[4]; ctl++){
					if(u < 0)
						u = findunit(b[5+ctl]);
					else if((x = findunit(b[5+ctl])) >= 0 &&
						u != x && verbose)
						fprint(2, "\tSelector %d for I & O\n", b[3]);
					dprint(2, ", %d", b[5+ctl]);
				}
				dprint(2, "]\n");
				if(u >= 0){
					units[u][nunits[u]++] = b[3];
					if(selectorid[u] >= 0)
						fprint(2, "Second selector (%d, %d) on %s\n", selectorid[u], b[3], u?"record":"playback");
					selectorid[u] = b[3];
					controls[u][Selector_control].readable = 1;
					controls[u][Selector_control].settable = 1;
					controls[u][Selector_control].chans = 0;
				}
			}
			break;
		case 0x06:	// feature
			if(verbose) fprint(2, "Audio Feature Unit %d", b[3]);
			dprint(2, "\tbUnitId %d, bSourceId %d, bControlSize %d\n",
				b[3], b[4], b[5]);
			u = findunit(b[4]);
			if(u >= 0){
				if(verbose) fprint(2, " for %s\n", u?"Record":"Playback");
				units[u][nunits[u]++] = b[3];
				if(featureid[u] >= 0)
					if(verbose)
						fprint(2, "Second feature unit (%d, %d)"
							" on %s\n", featureid[u], b[3],
							u?"record":"playback");
				featureid[u] = b[3];
			}else
				if(verbose) fprint(2, ", not known what for\n");
			p = b + 6;
			for(ctl = 1; ctl < 0x0b; ctl++)
				if((1<<(ctl-1)) & (b[6] | ((b[5]>1)?(b[7]<<8):0))){
					if(verbose)
						fprint(2, "\t%s control on master channel\n",
							controls[0][ctl].name);
					if(u >= 0){
						controls[u][ctl].readable = 1;
						controls[u][ctl].settable = 1;
						controls[u][ctl].chans = 0;
					}
				}
			p += (b[5]>1)?2:1;
			for(ch = 0; ch < (nb - 8)/b[5]; ch++){
				for(ctl = 1; ctl < 0x0b; ctl++)
					if((1<<(ctl-1)) & (p[0] | ((b[5]>1)?(p[1]<<8):0))){
						if(verbose)
						  fprint(2, "\t%s control on channel %d\n",
							controls[0][ctl].name, ch+1);
						if(u >= 0){
							controls[u][ctl].readable = 1;
							controls[u][ctl].settable = 1;
							controls[u][ctl].chans |= 1 <<(ch+1);
						}
					}
				p += (b[5]>1)?2:1;
			}
			break;
		default:
			hd = hexstr(bb, nb);
			fprint(2, "audio control unknown: %s\n", hd);
			free(hd);
		}
		break;
	case 2: // stream
		switch (b[2]){
		case 0x01:
			dprint(2, "Audio stream for TerminalID %d, delay %d, format_tag %#ux\n",
				b[3], b[4], b[5] | (b[6]<<8));
			break;
		case 0x02:
			aa = (Audioalt *)dd->altc->aux;
			if(aa == nil){
				aa = mallocz(sizeof(Audioalt), 1);
				dd->altc->aux = aa;
			}
			if(verbose){
				if(b[4] <= 2)
					fprint(2, "Interface %d: %s, %d bits, ",
						dd->iface->id, (b[4] == 1)?"mono":"stereo", b[6]);
				else
					fprint(2, "Interface %d, %d channels, %d bits, ",
						dd->iface->id, b[4], b[6]);
			}
			if(b[7] == 0){
				if(verbose)
					fprint(2, "frequency variable between %d and %d\n",
						b[8] | b[9]<<8 | b[10]<<16, b[11] | b[12]<<8 | b[13]<<16);
				aa->minfreq = b[8] | b[9]<<8 | b[10]<<16;
				aa->maxfreq = b[11] | b[12]<<8 | b[13]<<16;
				aa->caps |= has_contfreq;
			}else{
				if(verbose)
					fprint(2, "discrete frequencies are:");
				for(ch = 0; ch < b[7] && ch < 8; ch++){
					aa->freqs[ch] = b[8+3*ch] | b[9+3*ch]<<8 | b[10+3*ch]<<16;
					if(verbose)
						fprint(2, " %d", b[8+3*ch] | b[9+3*ch]<<8 | b[10+3*ch]<<16);
				}
				if(ch < 8)
					aa->freqs[ch] = -1;
				if(verbose)
					fprint(2, "\n");
				if(ch > 1)
					aa->caps |= has_discfreq;	/* more than one frequency */
				else
					aa->caps |= onefreq;		/* only one frequency */
			}
			aa->nchan = b[4];
			aa->res = b[6];
			aa->subframesize = b[5];
			break;
		default:
			if(usbdebug){
				hd = hexstr(bb, nb);
				fprint(2, "audio stream unknown: %s\n", hd);
				free(hd);
			}
		}
		break;
	case 3: // midi
	default:
		if(usbdebug){
			hd = hexstr(bb, nb);
			fprint(2, "Unknown audio stream type: CS_INTERFACE: %s\n", hd);
			free(hd);
		}
	}
}

