CmdiccR *		extauth(int fd, ushort p1, ushort p2, uchar chan, uchar *data, uchar dlen);
CmdiccR *		getchall(int fd, ushort p1, uchar chan, int howmany);
CmdiccR *crdenc(int fd, uchar p1, uchar p2, uchar *b, uchar blen, uchar chan);
CmdiccR *verify(int fd, ushort cla, uchar pid, uchar *pin, uchar plen);
