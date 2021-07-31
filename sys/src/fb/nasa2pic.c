#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
enum{
	Nrecord = 2000,
};
typedef struct Node NODE;
struct Node {
	short dn;
	NODE *right;
	NODE *left;
};
typedef struct Record Record;
struct Record{
	uchar	*data;
	int	len;
};
int rno;
Record *record;
NODE *tree;
NODE *new_node(short value){
	NODE *temp;         /* Pointer to the memory block */
	temp = malloc(sizeof(NODE));
	if(temp != 0) {
		temp->right = 0;
		temp->dn = value;
		temp->left = 0;
	}
	else {
		print("\nOut of memory in new_node!\n");
		exits("memory");
	}
	return temp;
}
void sort_freq(long *freq_list, NODE** node_list, long num_freq){
   	NODE **k, **l, *temp2;
	long *i, *j, temp1, cnt;
	if(num_freq <= 0) return;      /* If no elements or invalid, return */
	i = freq_list;
	k = node_list;
	cnt = num_freq;
	while(--cnt) {
		temp1 = *(++i);
		temp2 = *(++k);
		j = i;
		l = k;
		while(j[-1] > temp1) {
			j[0] = j[-1];
			l[0] = l[-1];
			j--;
			l--;
			if(j <= freq_list)
				break;
		}
		*j = temp1;
		*l = temp2;
	}
}
NODE *huff_tree(long *hist){
	uchar *cp;
	long freq_list[512];
	NODE **node_list, **np, *temp;
	long *fp, num_freq, j;
	short num_nodes, cnt;
	short znull = -1;
	fp = freq_list;
	node_list = (NODE **) malloc(sizeof(temp)*512);
	if (node_list == 0) {
		print("\nOut of memory in huff_tree!\n");
		exits("memory");
	}
	np = node_list;
	num_nodes=1;
	for(cnt=512 ; cnt-- ; num_nodes++) {
		cp = (uchar*)hist;
		hist++;
		j = (cp[3]<<24)|(cp[2]<<16)|(cp[1]<<8)|cp[0];
		*fp++ = j;
		temp = new_node(num_nodes);
		*np++ = temp;
	}
	fp[-1] = 0;         /* Ensure the last element is zeroed out.  */
	num_freq = 512;
	sort_freq(freq_list,node_list,num_freq);
	fp = freq_list;
	np = node_list;
	for(num_freq=512; *fp == 0 && num_freq; num_freq--) {
		fp++;
		np++;
	}
	for(temp= *np; num_freq-- > 1;) {
		temp = new_node(znull);
		temp->right = *np++;
		temp->left = *np;
		*np = temp;
		fp[1] += *fp;
		*fp++ = 0;
		sort_freq(fp, np, num_freq);
	}
  	return temp;
}
void dcmprs(char *ibuf, char *obuf, long *nin, long *nout, NODE *root){
	NODE *ptr = root;
	uchar test;
	uchar idn;
	char odn;
	char *ilim = ibuf + *nin;
	char *olim = obuf + *nout;
	if(ilim > ibuf && olim > obuf)
		odn = *obuf++ = *ibuf++;
	else {
		SET(odn);
		print("\nInvalid byte count in dcmprs!\n");
 		exits("invalid");
	}
	for (idn=(*ibuf) ; ibuf < ilim  ; idn =(*++ibuf)) {
		for (test=0x80 ; test ; test >>= 1) {
			ptr = (test & idn) ? ptr->left : ptr->right;
			if (ptr->dn != -1) {
				if (obuf >= olim)
					return;
				odn -= ptr->dn + 256;
				*obuf++ = odn;
				ptr = root;
			}
		}
	}
}
void decompress(char *ibuf, char *obuf, long *nin, long *nout){
    dcmprs(ibuf, obuf, nin, nout, tree);
}
void decmpinit(long *hist){
	tree = huff_tree(hist);
}
void getattr(char *attr, char *val){
	int i, n;
	char *p, *r;
	for(i = 1; i < rno; i++) {
		r = (char*)record[i].data;
		if(strstr(r, attr)) {
			p = strchr(r, '=');
			if(p == 0)
				break;
			p++;
			n = p-r;
			n = record[i].len-n;
			memmove(val, p, n);
			val[n] = '\0';
			return;
		}
	}
	print("No attribute %s\n", attr);
	exits("no attr");
}
void main(int argc, char **argv){
	Dir d;
	Bitmap *b;
	Record *r;
	char buf[4096];
	long *hist;
	uchar *p, *e, *s, *img;
	int fd, n, eh, et, size, x, y, ldepth, imr;
	PICFILE *out;
	if(getflags(argc, argv, "")!=2) usage("file");
	fd = open(argv[1], OREAD);
	if(fd < 0) {
		print("open %s: %r\n", argv[1]);
		return;
	}
	if(dirfstat(fd, &d) < 0) {
		print("stat %s: %r\n", argv[1]);
		return;
	}
	p = malloc(d.length);
	if(p == 0) {
		print("no memory\n");
		exits("memory");
	}
	n = read(fd, p, d.length);
	if(n != d.length) {
		print("read %s: %r\n", argv[1]);
		free(p);
		close(fd);
		return;
	}
	close(fd);
	s = p;
	e = p+d.length;
	rno = 1;
	while(p < e) {
		n = p[0]|(p[1]<<8);
		p += 2;
		p += n + (n&1);
		rno++;
	}
/*	print("%d records\n", rno); */
	p = s;
	record = malloc(sizeof(Record)*rno);
	rno = 1;
	while(p < e) {
		n = p[0]|(p[1]<<8);
/*		print("record %d %d (%ux %ux) ", rno, n, p[0], p[1]);	*/
		p += 2;
/*		print("%.*s\n", n, p);  */
		record[rno].len = n;
		record[rno].data = p;
		p += n + (n&1);
		rno++;
	}
	getattr("^ENCODING_HISTOGRAM", buf);
	eh = atoi(buf);
	getattr("^ENGINEERING_TABLE", buf);
	et = atoi(buf);
	size = 0;
	for(n = eh; n < et; n++)
		size += record[n].len;
	hist = malloc(size);
	p = (uchar*)hist;
	for(n = eh; n < et; n++) {
		memmove(p, record[n].data, record[n].len);
		p += record[n].len;
	}
	decmpinit(hist);
	free(hist);
	getattr("LINES", buf);
	y = atoi(buf);
	getattr("LINE_SAMPLES", buf);
	x = atoi(buf);
	getattr("SAMPLE_BITS", buf);
	ldepth = atoi(buf);
	if(ldepth != 8) {
		print("only understand 8 bit images\n");
		exits("ldepth");
	}
	getattr("^IMAGE", buf);
	imr = atoi(buf);
	img = malloc(x*y);
	p = img;
	/* print("x %d y %d ldepth %d\n", x, y, ldepth); */
	for(n = 0; n < y; n++) {
		r = &record[imr+n];
		decompress((char*)r->data, (char*)p, &r->len, &x);
		p += x;
	}
	out=picopen_w("OUT", "runcode", 0, 0, x, y, "m", 0, 0);
	if(out==0){
		picerror("OUT");
		exits("open output");
	}
	for(n=0;n!=y;n++) picwrite(out, img+n*x);
	free(img);
	free(s);
	free(record);
}
