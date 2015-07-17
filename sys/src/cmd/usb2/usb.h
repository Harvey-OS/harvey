
typedef struct Usbconfig Usbconfig;

struct Usbconfig {
	int config;
	int iface;
	int alt;
	int nendpt;
	int nextra;
	struct {
		int addr;
		int type;
		int maxpkt;
		int pollival;
	} endpt[16];
	uint8_t extra[236];
};

int usbdescread(int fd, uint8_t *buf, int len, int desctype, int index);
int usbconfread(int fd, Usbconfig **confp);
int usbconfprint(int fd, Usbconfig *cp);
int usbopen(int fd, Usbconfig *cp, int epi, int *ctlp);
