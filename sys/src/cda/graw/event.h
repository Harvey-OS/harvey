#define BSIZE	64
#define button(i)	(mouse.buttons&(1<<(i-1)))

typedef struct MyEvent {
	short type;
	short len;
	char data[BSIZE];
} MyEvent;

enum {KBD, MOUSE};

Mouse mouse;
