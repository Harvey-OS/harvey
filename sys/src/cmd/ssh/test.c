#include <u.h>
#include <libc.h>

#include "ssh.h"

char *progname;

static uchar		inivec[8];
static DES3state	indes3state;
static DESstate		indesstate;
static uchar		outivec[8];
static DES3state	outdes3state;
static DESstate		outdesstate;
uchar			thekey[7];

void
main(int argc, char *argv[]) {
	char line[1024];
	int i, value;
	unsigned char key[8], data1[8], data2[8], output[8];

	while (fgets(line, sizeof(line), stdin)) {
		for (i = 0; i < 8; i++) {
			if (sscanf(line + 2 * i, "%02x", &value) != 1) {
				fprintf(stderr, "1st col, i = %d, line: %s", i, line);
				exits("1");
			}
			key[i] = value;
		}

		fprint(2, "8-byte DES key 0x");
		for (i = 0; i < 8; i++) {
		  fprint(2, "%2.2x", key[i]);
		}
		fprint(2, "\n");

		fprint(2, "7-byte DES key 0x");
		for (i = 0; i < 7; i++) {
		  thekey[i] = ((key[i] & 0xfe) << i) |
				  (key[i+1] >> (7-i));
		  fprint(2, "%2.2x", thekey[i]);
		}
		fprint(2, "\n");

		for (i = 0; i < 8; i++) {
			if (sscanf(line + 2 * i + 17, "%02x", &value) != 1) {
				fprintf(stderr, "2nd col, i = %d, line: %s", i, line);
				exits("1");
			}
			data1[i] = value;
		}
		for (i = 0; i < 8; i++) {
			if (sscanf(line + 2 * i + 34, "%02x", &value) != 1) {
				fprintf(stderr, "2nd col, i = %d, line: %s", i, line);
				exits("1");
			}
			data2[i] = value;
		}
		memcpy(output, data1, 8);
		fprint(2, "Input  0x");
		for (i = 0; i < 8; i++)
			fprint(2, "%2.2x", output[i]);
		fprint(2, "\n");
		memset(inivec, 0, sizeof(inivec));
		memset(outivec, 0, sizeof(outivec));
		memset(&indesstate, 0, sizeof(indesstate));
		memset(&outdesstate, 0, sizeof(outdesstate));
		blockCBCEncrypt(output, inivec, thekey, 8, &indesstate);
		fprint(2, "Output 0x");
		for (i = 0; i < 8; i++)
			fprint(2, "%2.2x", output[i]);
		fprint(2, "\n");
		blockCBCDecrypt(output, outivec, thekey, 8, &outdesstate);
		if (memcmp(output, data1, 8) != 0)
		  fprintf(stderr, "Decrypt failed: %s", line);

		memcpy(output, data2, 8);
		fprint(2, "Input  0x");
		for (i = 0; i < 8; i++)
			fprint(2, "%2.2x", output[i]);
		fprint(2, "\n");
		blockCBCEncrypt(output, inivec, thekey, 8, &indesstate);
		fprint(2, "Output 0x");
		for (i = 0; i < 8; i++)
			fprint(2, "%2.2x", output[i]);
		fprint(2, "\n");
		blockCBCDecrypt(output, outivec, thekey, 8, &outdesstate);
	  if (memcmp(output, data2, 8) != 0)
		  fprintf(stderr, "Decrypt failed: %s", line);
	}
}
