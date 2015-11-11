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


typedef struct _rijndaelCtx
{
	uint32_t		k_len;
	int				decrypt;
	uint32_t		e_key[64];
	uint32_t		d_key[64];
} rijndaelCtx;


/* Standard interface for AES cryptographic routines				*/

/* These are all based on 32 bit unsigned values and will therefore */
/* require endian conversions for big-endian architectures			*/

rijndaelCtx *
			rijndael_set_key(rijndaelCtx *, const uint32_t *, const uint32_t, int);
void		rijndael_encrypt(rijndaelCtx *, const uint32_t *, uint32_t *);
void		rijndael_decrypt(rijndaelCtx *, const uint32_t *, uint32_t *);

/* conventional interface */

void		aes_set_key(rijndaelCtx *ctx, const uint8_t *key, unsigned keybits, int enc);
void		aes_ecb_encrypt(rijndaelCtx *ctx, uint8_t *data, unsigned len);
void		aes_ecb_decrypt(rijndaelCtx *ctx, uint8_t *data, unsigned len);
void		aes_cbc_encrypt(rijndaelCtx *ctx, uint8_t *iva, uint8_t *data, unsigned len);
void		aes_cbc_decrypt(rijndaelCtx *ctx, uint8_t *iva, uint8_t *data, unsigned len);
