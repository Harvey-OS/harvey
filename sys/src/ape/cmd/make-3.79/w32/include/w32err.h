#ifndef _W32ERR_H_
#define _W32ERR_H_

#ifndef EXTERN_DECL
#define EXTERN_DECL(entry, args) entry args
#endif

EXTERN_DECL(char * map_windows32_error_to_string, (DWORD error));

#endif /* !_W32ERR_H */
