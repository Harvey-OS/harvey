#ifndef _PATHSTUFF_H
#define _PATHSTUFF_H

extern char * convert_Path_to_windows32(char *Path, char to_delim);
extern char * convert_vpath_to_windows32(char *Path, char to_delim);
extern char * w32ify(char *file, int resolve);
extern char * getcwd_fs(char *buf, int len);

#endif
