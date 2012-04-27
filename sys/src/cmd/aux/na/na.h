#ifndef NA_H
#define NA_H

struct na_patch {
	unsigned lwoff;
	unsigned char type;
};

int na_fixup(unsigned long *script, unsigned long pa_script, unsigned long pa_reg,
    struct na_patch *patch, int patches,
    int (*externval)(int x, unsigned long *v));

#endif
