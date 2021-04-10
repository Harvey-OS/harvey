/*
 * contrib/pgcrypto/rijndael.h
 *
 *	$OpenBSD: rijndael.h,v 1.3 2001/05/09 23:01:32 markus Exp $ */

/* This is an independent implementation of the encryption algorithm:	*/
/*																		*/
/*		   RIJNDAEL by Joan Daemen and Vincent Rijmen					*/
/*																		*/
/* which is a candidate algorithm in the Advanced Encryption Standard	*/
/* programme of the US National Institute of Standards and Technology.	*/
/*																		*/
/* Copyright in this implementation is held by Dr B R Gladman but I		*/
/* hereby give permission for its free direct or derivative use subject */
/* to acknowledgment of its origin and compliance with any conditions	*/
/* that the originators of the algorithm place on its exploitation.		*/
/*																		*/
/* Dr Brian Gladman (gladman@seven77.demon.co.uk) 14th January 1999		*/

typedef struct rijndaelCtx rijndaelCtx;

struct rijndaelCtx
{
	u32		k_len;
	int				decrypt;
	u32		e_key[64];
	u32		d_key[64];
};


/* Standard interface for AES cryptographic routines				*/

/* These are all based on 32 bit unsigned values and will therefore */
/* require endian conversions for big-endian architectures			*/

rijndaelCtx *
			rijndael_set_key(rijndaelCtx *, const u32 *,
					 const u32, int);
void		rijndael_encrypt(rijndaelCtx *, const u32 *, u32 *);
void		rijndael_decrypt(rijndaelCtx *, const u32 *, u32 *);

/* conventional interface */

void		aes_set_key(rijndaelCtx *ctx, const u8 *key,
				unsigned keybits, int enc);
void		aes_ecb_encrypt(rijndaelCtx *ctx, u8 *data, unsigned len);
void		aes_ecb_decrypt(rijndaelCtx *ctx, u8 *data, unsigned len);
void		aes_cbc_encrypt(rijndaelCtx *ctx, u8 *iva, u8 *data,
				    unsigned len);
void		aes_cbc_decrypt(rijndaelCtx *ctx, u8 *iva, u8 *data,
				    unsigned len);
