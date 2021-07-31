#define BSIZE	4000
#define MOUSE	0
#define KBD	1
#define	HOST	2
#define HOST_BLOCKED	1
#define KBD_BLOCKED	2

typedef struct IOEvent {
	short	key;
	short	size;
	uchar	data[BSIZE];
} IOEvent;

