#define	REQ	0200
#define ACK	0201

#define	WINDOW	0202		/*send window size*/
#define	CACHECHAR	0203	/*put char in cache*/ 
#define FONTCACHE	0204	/*make cache font*/
#define PUTCHAR		0205	/*output char from cache*/
#define	TEXTURE		0206
#define BITBLT		0207
#define ERASE		0210
#define FONTNO		0211
#define PUTCHARX	0212
#define PUTCHARX2	0213
#define PUTCHARNX	0214
#define PUTCHARNXY	0215
#define PUTCHARXY	0216
#define MBITBLT		0217
#define BBITBLT		0220	/*bottom up bitmap*/
#define CACHENEXT	0221	/*next bitmap to cache*/
#define SHOWCACHENEXT	0222	/*next bitmap to cache & display*/
#define CBITBLT		0223
#define INVERT		0224
#define CFREE		0225
#define SWITCH_CUP	0226
#define CHAR_TYPE1	0227

#define JDONE		0230
#define JCONT		0231
#define JEOF		0232
#define JLESS		0233
#define JPAGE		0234

#define STOP	4096
