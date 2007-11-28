
#ifndef LAME_ID3_H
#define LAME_ID3_H

#include "lame.h"

struct id3tag_spec
{
    /* private data members */
    int flags;
    const char *title;
    const char *artist;
    const char *album;
    int year;
    const char *comment;
    int track;
    int genre;
};


/* write tag into stream at current position */
extern int id3tag_write_v2(lame_global_flags *gfp);
extern int id3tag_write_v1(lame_global_flags *gfp);
/*
 * NOTE: A version 2 tag will NOT be added unless one of the text fields won't
 * fit in a version 1 tag (e.g. the title string is longer than 30 characters),
 * or the "id3tag_add_v2" or "id3tag_v2_only" functions are used.
 */

#endif
