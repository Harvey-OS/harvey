#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_9fs
SRCS=	mount_9fs.c getmntopts.c crypt.c
MAN8=	mount_9fs.8

CFLAGS = -ggdb -O0

MOUNT=	/usr/src/sbin/mount
CFLAGS+= -DNFS -I${MOUNT}
.PATH:	${MOUNT}

.if exists(${DESTDIR}/usr/lib/libkrb.a) && (defined(MAKE_KERBEROS) \
	|| defined(MAKE_EBONES))
CFLAGS+=-DKERBEROS
DPADD=	${LIBKRB} ${LIBDES}
LDADD=	-lkrb -ldes
DISTRIBUTION=	krb
.endif

.include <bsd.prog.mk>
