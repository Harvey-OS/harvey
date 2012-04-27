#ifndef __IOCTL_H__
#define __IOCTL_H__

#ifndef _BSD_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* FIONREAD: return number of bytes readable in *(long*)arg */
#define FIONREAD 1

int ioctl(int, unsigned long, void*);

#ifdef __cplusplus
}
#endif


#endif /* !__IOCTL_H__ */
