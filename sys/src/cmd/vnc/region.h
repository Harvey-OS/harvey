
#ifndef REGION_H
#define REGION_H

typedef struct Region Region;
typedef struct RegData RegData;

struct RegData {
	int size;		/* # of rects it can hold */
	int nrects;		/* # used */
	Rectangle * rects;
};

struct Region {
	Rectangle extents;		/* the bounding box */

	RegData;
};

#define REGION_EMPTY(reg) ((reg)->nrects == 0)

extern void region_init(Region *);
extern void region_union(Region *, Rectangle, Rectangle);
extern void region_reset(Region *);
extern void region_copy(Region *, Region *);


#endif /* !REGION_H */
