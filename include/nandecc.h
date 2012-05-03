#pragma src "/usr/inferno/libnandfs"

typedef enum NandEccError {
	NandEccErrorBad,
	NandEccErrorGood,
	NandEccErrorOneBit,
	NandEccErrorOneBitInEcc,
} NandEccError;

ulong nandecc(uchar buf[256]);
NandEccError nandecccorrect(uchar buf[256], ulong calcecc, ulong *storedecc, int reportbad);

