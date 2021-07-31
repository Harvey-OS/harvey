/*
 *	USB protocol requests on Endpoint 0
 */

typedef uchar byte;

#define	GET2(p)	((((p)[1]&0xFF)<<8)|((p)[0]&0xFF))
#define	PUT2(p,v)	{((p)[0] = (v)); ((p)[1] = (v)>>8);}
#define	GET4(p)	((((p)[3]&0xFF)<<24)|(((p)[2]&0xFF)<<16)|(((p)[1]&0xFF)<<8)|((p)[0]&0xFF))
#define	PUT4(p,v)	{((p)[0] = (v)); ((p)[1] = (v)>>8); ((p)[2] = (v)>>16); ((p)[3] = (v)>>24);}
#define	BIT(i)	(1<<(i))

/*
 *	All device classes
 */
enum
{
	/* request type */
	RH2D = 0<<7,
	RD2H = 1<<7,
	Rstandard = 0<<5,
	Rclass = 1<<5,
	Rvendor = 2<<5,
	Rdevice = 0,
	Rinterface = 1,
	Rendpt = 2,
	Rother = 3,

	/* standard requests (type Rstandard) */
	GET_STATUS = 0,
	CLEAR_FEATURE = 1,
	SET_FEATURE = 3,
	SET_ADDRESS = 5,
	GET_DESCRIPTOR = 6,
	SET_DESCRIPTOR = 7,
	GET_CONFIGURATION = 8,
	SET_CONFIGURATION = 9,
	GET_INTERFACE = 10,
	SET_INTERFACE = 11,
	SYNCH_FRAME = 12,

	/* descriptor types */
	DEVICE = 1,
	CONFIGURATION = 2,
	STRING = 3,
	INTERFACE = 4,
	ENDPOINT = 5,
	HID = 0x21,
	REPORT = 0x22,
	PHYSICAL = 0x23,
};

typedef struct DDevice DDevice;
struct DDevice
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bcdUSB[2];
	byte	bDeviceClass;
	byte	bDeviceSubClass;
	byte	bDeviceProtocol;
	byte	bMaxPacketSize0;
	byte	idVendor[2];
	byte	idProduct[2];
	byte	bcdDevice[2];
	byte	iManufacturer;
	byte	iProduct;
	byte	iSerialNumber;
	byte	bNumConfigurations;
};
#define	DDEVLEN	18

typedef struct DConfig DConfig;
struct DConfig
{
	byte	bLength;
	byte	bDescriptorType;
	byte	wTotalLength[2];
	byte	bNumInterfaces;
	byte	bConfigurationValue;
	byte	iConfiguration;
	byte	bmAttributes;
	byte	MaxPower;
};
#define	DCONFLEN	9

typedef struct DInterface DInterface;
struct DInterface
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bInterfaceNumber;
	byte	bAlternateSetting;
	byte	bNumEndpoints;
	byte	bInterfaceClass;
	byte	bInterfaceSubClass;
	byte	bInterfaceProtocol;
	byte	iInterface;
};
#define	DINTERLEN	9

typedef struct DEndpoint DEndpoint;
struct DEndpoint
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bEndpointAddress;
	byte	bmAttributes;
	byte	wMaxPacketSize[2];
	byte	bInterval;
};
#define	DENDPLEN	7

/*
 *	Hub Class
 */
enum
{
	/* hub class feature selectors */
	C_HUB_LOCAL_POWER = 0,
	C_HUB_OVER_CURRENT,

	PORT_CONNECTION = 0,
	PORT_ENABLE = 1,
	PORT_SUSPEND = 2,
	PORT_OVER_CURRENT = 3,
	PORT_RESET = 4,
	PORT_POWER = 8,
	PORT_LOW_SPEED = 9,
	C_PORT_CONNECTION = 16,
	C_PORT_ENABLE,
	C_PORT_SUSPEND,
	C_PORT_OVER_CURRENT,
	C_PORT_RESET,

	/* feature selectors */
	DEVICE_REMOTE_WAKEUP = 1,
	ENDPOINT_STALL = 0,
};

typedef struct DHub DHub;
struct DHub
{
	byte	bLength;
	byte	bDescriptorType;
	byte	bNbrPorts;
	byte	wHubCharacteristics[2];
	byte	bPwrOn2PwrGood;
	byte	bHubContrCurrent;
	byte	DeviceRemovable[1];	/* variable length */
/*	byte	PortPwrCtrlMask;		/* variable length, deprecated in USB v1.1 */
};
#define	DHUBLEN	9
