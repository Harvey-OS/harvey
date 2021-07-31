void jis_in(int fd, long *notused, struct convert *out);
void jis_out(Rune *base, int n, long *notused);
void big5_in(int fd, long *notused, struct convert *out);
void big5_out(Rune *base, int n, long *notused);
void gb_in(int fd, long *notused, struct convert *out);
void gb_out(Rune *base, int n, long *notused);

#define		emit(x)		*(*r)++ = (x)
#define		NRUNE		65536
