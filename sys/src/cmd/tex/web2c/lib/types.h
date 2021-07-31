/* types.h: general types.  */

#ifndef TYPES_H
#define TYPES_H

/* Booleans.  */
typedef enum { false = 0, true = 1 } boolean;

/* The X11 library defines `FALSE' and `TRUE', and so we only want to
   define them if necessary.  */
#ifndef FALSE
#define FALSE false
#define TRUE true
#endif /* FALSE */

/* The usual null-terminated string.  */
typedef char *string;

/* A generic pointer in ANSI C.  */
typedef void *address;

#endif /* not TYPES_H */
