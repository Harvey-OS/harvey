/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gsiscale.h */
/* Interface and definitions for image scaling */

/* Input values */
/*typedef byte PixelIn;*/		/* see sizeofPixelIn below */
/*#define maxPixelIn 255*/		/* see maxPixelIn below */

/* Temporary intermediate values */
typedef byte PixelTmp;
#define minPixelTmp 0
#define maxPixelTmp 255
#define unitPixelTmp 255

/* Output values */
/*typedef byte PixelOut;*/		/* see sizeofPixelOut below */
/*#define maxPixelOut 255*/		/* see maxPixelOut below */

/* Auxiliary structures. */

typedef struct {
	int	pixel;		/* offset of pixel in source data */
	float	weight;
} CONTRIB;

typedef struct {
	int	n;		/* number of contributors */
	int	max_index;	/* max value of pixel in contributors list, */
				/* not multiplied by stride */
	CONTRIB	*p;		/* pointer to list of contributors */
} CLIST;

/* Scaler state. */
typedef struct gs_image_scale_state_s {
		/* Client sets these before calling init. */
	int sizeofPixelIn;		/* size of input pixel, 1 or 2 */
	uint maxPixelIn;		/* max value of input pixel */
	int sizeofPixelOut;		/* size of output pixel, 1 or 2 */
	uint maxPixelOut;		/* max value of output pixel */
	int dst_width, dst_height;
	int src_width, src_height;
	int values_per_pixel;
	gs_memory_t *memory;
		/* Init may set these if client requests. */
	void /*PixelOut*/ *dst;
	const void /*PixelIn*/ *src;
		/* Init sets these. */
	double xscale, yscale;
	PixelTmp *tmp;
	CLIST *contrib;
	CONTRIB *items;
} gs_image_scale_state;
#define image_scale_state_max_ptrs 5
#define image_scale_state_clear_ptrs(pss)\
  ((pss)->tmp = 0, (pss)->contrib = 0, (pss)->items = 0,\
   (pss)->dst = 0, (pss)->src = 0)
#define image_scale_state_ENUM_PTRS(i, t, e)\
  ENUM_PTR(i, t, e tmp);\
  ENUM_PTR((i)+1, t, e contrib);\
  ENUM_PTR((i)+2, t, e items);\
  ENUM_PTR((i)+3, t, e dst);\
  ENUM_PTR((i)+4, t, e src)
#define image_scale_state_RELOC_PTRS(t, e)\
  RELOC_PTR(t, e tmp);\
  RELOC_PTR(t, e contrib);\
  RELOC_PTR(t, e items);\
  RELOC_PTR(t, e dst);\
  RELOC_PTR(t, e src)

/* ------ Procedural interface ------ */

/* Initialize the scaler state, allocating all necessary working storage. */
int gs_image_scale_init(P3(gs_image_scale_state *pss, bool alloc_dst,
			   bool alloc_src));

/* Scale an image. */
int gs_image_scale(P3(void /*PixelOut*/ *dst, const void /*PixelIn*/ *src,
		      gs_image_scale_state *pss));

/* Clean up by freeing working storage. */
void gs_image_scale_cleanup(P1(gs_image_scale_state *pss));
