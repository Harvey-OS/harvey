/*
 * ext2subs.c version 0.20
 * 
 * Some strategic functions come from linux/fs/ext2
 * kernel sources written by Remy Card.
 *
*/

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

#define putext2(e)	putbuf((e).buf)
#define dirtyext2(e)	dirtybuf((e).buf)

static Intmap *uidmap, *gidmap;

static int
getnum(char *s, int *n)
{
	char *r;

	*n = strtol(s, &r, 10);
	return (r != s);
}

static Intmap*
idfile(char *f)
{
	Biobuf *bin;
	Intmap *map;
	char *fields[3];
	char *s;
	int nf, id;

	map = allocmap(0);
	bin = Bopen(f, OREAD);
	if (bin == 0)
		return 0;
	while ((s = Brdline(bin, '\n')) != 0) {
		s[Blinelen(bin)-1] = '\0';
		nf = getfields(s, fields, 3, 0, ":");
		if (nf == 3 && getnum(fields[2], &id))
			insertkey(map, id, strdup(fields[0]));
	}
	Bterm(bin);
	return map;
}

void
uidfile(char *f)
{
	uidmap = idfile(f);
}

void
gidfile(char *f)
{
	gidmap = idfile(f);
}

static char*
mapuid(int id)
{
	static char s[12];
	char *p;

	if (uidmap && (p = lookupkey(uidmap, id)) != 0)
		return p;
	sprint(s, "%d", id);
	return s;
}

static char*
mapgid(int id)
{
	static char s[12];
	char *p;

	if (gidmap && (p = lookupkey(gidmap, id)) != 0)
		return p;
	sprint(s, "%d", id);
	return s;
}

int
ext2fs(Xfs *xf)
{
	SuperBlock superblock;

	/* get the super block */
	seek(xf->dev, OFFSET_SUPER_BLOCK, 0);
	if( sizeof(SuperBlock) != 
				read(xf->dev, &superblock, sizeof(SuperBlock)) ){
		chat("can't read super block %r...", xf->dev);
		errno = Eformat;
		return -1;
	}
	if( superblock.s_magic != EXT2_SUPER_MAGIC ){
		chat("Bad super block...");
		errno = Eformat;
		return -1;
	}
	if( !(superblock.s_state & EXT2_VALID_FS) ){
		chat("fs not checked...");
		errno = Enotclean;
		return -1;
	}
	
	xf->block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
	xf->desc_per_block = xf->block_size / sizeof (GroupDesc);
	xf->inodes_per_group = superblock.s_inodes_per_group;
	xf->inodes_per_block = xf->block_size / sizeof (Inode);
	xf->addr_per_block = xf->block_size / sizeof (uint);
	xf->blocks_per_group = superblock.s_blocks_per_group;

	if( xf->block_size == OFFSET_SUPER_BLOCK )
		xf->superaddr = 1, xf->superoff = 0, xf->grpaddr = 2;
	else if( xf->block_size == 2*OFFSET_SUPER_BLOCK ||
			xf->block_size == 4*OFFSET_SUPER_BLOCK )
		xf->superaddr = 0, xf->superoff = OFFSET_SUPER_BLOCK, xf->grpaddr = 1;
	else {
		chat(" blocks of %d bytes are not supported...", xf->block_size);
		errno = Eformat;
		return -1;
	}

	chat("good super block...");

	xf->ngroups = (superblock.s_blocks_count - 
				superblock.s_first_data_block + 
				superblock.s_blocks_per_group -1) / 
				superblock.s_blocks_per_group;

	superblock.s_state &= ~EXT2_VALID_FS;
	superblock.s_mnt_count++;
	seek(xf->dev, OFFSET_SUPER_BLOCK, 0);
	if( !rdonly && sizeof(SuperBlock) != 
				write(xf->dev, &superblock, sizeof(SuperBlock)) ){
		chat("can't write super block...");
		errno = Eio;
		return -1;
	}

	return 0;
}
Ext2
getext2(Xfs *xf, char type, int n)
{
	Iobuf *bd;
	Ext2 e;

	switch(type){
	case EXT2_SUPER:
		e.buf = getbuf(xf, xf->superaddr);
		if( !e.buf ) goto error;
		e.u.sb = (SuperBlock *)(e.buf->iobuf + xf->superoff);
		e.type = EXT2_SUPER;
		break;
	case EXT2_DESC:
		e.buf = getbuf(xf, DESC_ADDR(xf, n));
		if( !e.buf ) goto error;
		e.u.gd = DESC_OFFSET(xf, e.buf->iobuf, n);
		e.type = EXT2_DESC;
		break;
	case EXT2_BBLOCK:
		bd = getbuf(xf, DESC_ADDR(xf, n));
		if( !bd ) goto error;
		e.buf = getbuf(xf, DESC_OFFSET(xf, bd->iobuf, n)->bg_block_bitmap);
		if( !e.buf ){
			putbuf(bd);
			goto error;
		}
		putbuf(bd);
		e.u.bmp = (char *)e.buf->iobuf;
		e.type = EXT2_BBLOCK;
		break;
	case EXT2_BINODE:
		bd = getbuf(xf, DESC_ADDR(xf, n));
		if( !bd ) goto error;
		e.buf = getbuf(xf, DESC_OFFSET(xf, bd->iobuf, n)->bg_inode_bitmap);
		if( !e.buf ){
			putbuf(bd);
			goto error;
		}
		putbuf(bd);
		e.u.bmp = (char *)e.buf->iobuf;
		e.type = EXT2_BINODE;
		break;
	default:
		goto error;
	}
	return e;
error:
	panic("getext2");
	return e;
}
int
get_inode( Xfile *file, uint nr )
{
	unsigned long block_group, block;
	Xfs *xf = file->xf;
	Ext2 ed, es;

	es = getext2(xf, EXT2_SUPER, 0);
	if(nr > es.u.sb->s_inodes_count ){
		chat("inode number %d is too big...", nr);
		putext2(es);
		errno = Eio;
		return -1;
	}
	putext2(es);
	block_group = (nr - 1) / xf->inodes_per_group;
	if( block_group >= xf->ngroups ){
		chat("block group (%d) > groups count...", block_group);
		errno = Eio;
		return -1;
	}
	ed = getext2(xf, EXT2_DESC, block_group);
	block = ed.u.gd->bg_inode_table + (((nr-1) % xf->inodes_per_group) / 
			xf->inodes_per_block);
	putext2(ed);

	file->bufoffset = (nr-1) % xf->inodes_per_block;
	file->inbr = nr;
	file->bufaddr= block;

	return 1;
}
int
get_file( Xfile *f, char *name)
{	
	uint offset, nr, i;
	Xfs *xf = f->xf;
	Inode *inode;
	int nblock;
	DirEntry *dir;
	Iobuf *buf, *ibuf;
	
	if( !S_ISDIR(getmode(f)) )
		return -1;
	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;
	nblock = (inode->i_blocks * 512) / xf->block_size;

	for(i=0 ; (i < nblock) && (i < EXT2_NDIR_BLOCKS) ; i++){
		buf = getbuf(xf, inode->i_block[i]);
		if( !buf ){
			putbuf(ibuf);
			return -1;
		}
		for(offset=0 ; offset < xf->block_size ;  ){
			dir = (DirEntry *)(buf->iobuf + offset);
			if( dir->name_len==strlen(name) && 
					!strncmp(name, dir->name, dir->name_len) ){
				nr = dir->inode;
				putbuf(buf);
				putbuf(ibuf);
				return nr;
			}
			offset += dir->rec_len;
		}
		putbuf(buf);

	}
	putbuf(ibuf);
	errno = Enonexist;
	return -1;
}
char *
getname(Xfile *f, char *str)
{
	Xfile ft;
	int offset, i, len;
	Xfs *xf = f->xf;
	Inode *inode;
	int nblock;
	DirEntry *dir;
	Iobuf *buf, *ibuf;

	ft = *f;
	if( get_inode(&ft, f->pinbr) < 0 )
		return 0;
	if( !S_ISDIR(getmode(&ft)) )
		return 0;
	ibuf = getbuf(xf, ft.bufaddr);
	if( !ibuf )
		return 0;
	inode = ((Inode *)ibuf->iobuf) + ft.bufoffset;
	nblock = (inode->i_blocks * 512) / xf->block_size;

	for(i=0 ; (i < nblock) && (i < EXT2_NDIR_BLOCKS) ; i++){
		buf = getbuf(xf, inode->i_block[i]);
		if( !buf ){
			putbuf(ibuf);
			return 0;
		}
		for(offset=0 ; offset < xf->block_size ;  ){
			dir = (DirEntry *)(buf->iobuf + offset);
			if( f->inbr == dir->inode ){
				len = (dir->name_len < EXT2_NAME_LEN) ? dir->name_len : EXT2_NAME_LEN;
				if (str == 0)
					str = malloc(len+1);
				strncpy(str, dir->name, len);   
				str[len] = 0;
				putbuf(buf);
				putbuf(ibuf);
				return str;
			}
			offset += dir->rec_len;
		}
		putbuf(buf);
	}
	putbuf(ibuf);
	errno = Enonexist;
	return 0;
}
void
dostat(Qid qid, Xfile *f, Dir *dir )
{
	Inode *inode;
	Iobuf *ibuf;
	char *name;

	memset(dir, 0, sizeof(Dir));

	if(  f->inbr == EXT2_ROOT_INODE ){
		dir->name = estrdup9p("/");
		dir->qid = (Qid){0,0,QTDIR};
		dir->mode = DMDIR | 0777;
	}else{
		ibuf = getbuf(f->xf, f->bufaddr);
		if( !ibuf )
			return;
		inode = ((Inode *)ibuf->iobuf) + f->bufoffset;
		dir->length = inode->i_size;
		dir->atime = inode->i_atime;
		dir->mtime = inode->i_mtime;
		putbuf(ibuf);
		name = getname(f, 0);
		dir->name = name;
		dir->uid = estrdup9p(mapuid(inode->i_uid));
		dir->gid = estrdup9p(mapgid(inode->i_gid));
		dir->qid = qid;
		dir->mode = getmode(f);
		if( qid.type & QTDIR )
			dir->mode |= DMDIR;
	}

}
int 
dowstat(Xfile *f, Dir *stat)
{
	Xfs *xf = f->xf;
	Inode *inode;
	Xfile fdir;
	Iobuf *ibuf;
	char name[EXT2_NAME_LEN+1];

	/* change name */
	getname(f, name);
	if( stat->name && stat->name[0] != 0 && strcmp(name, stat->name) ){

		/* get dir */
		fdir = *f;
		if( get_inode(&fdir, f->pinbr) < 0 ){
			chat("can't get inode %d...", f->pinbr);
			return -1;
		}
	
		ibuf = getbuf(xf, fdir.bufaddr);
		if( !ibuf )
			return -1;
		inode = ((Inode *)ibuf->iobuf) +fdir.bufoffset;

		/* Clean old dir entry */
		if( delete_entry(xf, inode, f->inbr) < 0 ){
			chat("delete entry failed...");
			putbuf(ibuf);	
			return -1;
		}
		putbuf(ibuf);

		/* add the new entry */
		if( add_entry(&fdir, stat->name, f->inbr) < 0 ){
			chat("add entry failed...");	
			return -1;
		}
	
	}

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;

	if (stat->mode != ~0)
	if( (getmode(f) & 0777) != (stat->mode & 0777) ){
		inode->i_mode = (getmode(f) & ~0777) | (stat->mode & 0777);
		dirtybuf(ibuf);
	}
	if (stat->mtime != ~0)
	if(  inode->i_mtime != stat->mtime ){
		inode->i_mtime = stat->mtime;
		dirtybuf(ibuf);
	}

	putbuf(ibuf);

	return 1;
}
long
readfile(Xfile *f, void *vbuf, vlong offset, long count)
{
	Xfs *xf = f->xf;
	Inode *inode;
	Iobuf *buffer, *ibuf;
	long rcount;
	int len, o, cur_block, baddr;
	uchar *buf;

	buf = vbuf;
	
	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;

	if( offset >= inode->i_size ){
		putbuf(ibuf);
		return 0;
	}
	if( offset + count > inode->i_size )
		count = inode->i_size - offset;

	/* fast link */
	if( S_ISLNK(getmode(f)) && (inode->i_size <= EXT2_N_BLOCKS<<2) ){
		memcpy(&buf[0], ((char *)inode->i_block)+offset, count);
		putbuf(ibuf);	
		return count;
	}
	chat("read block [ ");
	cur_block = offset / xf->block_size;
	o = offset % xf->block_size;
	rcount = 0;
	while( count > 0 ){
		baddr = bmap(f, cur_block++);
		if( !baddr ){
			putbuf(ibuf);
			return -1;
		}
		buffer = getbuf(xf, baddr);
		if( !buffer ){
			putbuf(ibuf);
			return -1;
		}
		chat("%d ", baddr);
		len = xf->block_size - o;
		if( len > count )
			len = count;
		memcpy(&buf[rcount], &buffer->iobuf[o], len);
		rcount += len;
		count -= len;
		o = 0;
		putbuf(buffer);
	}
	chat("] ...");
	inode->i_atime = time(0);
	dirtybuf(ibuf);
	putbuf(ibuf);
	return rcount;
}
long
readdir(Xfile *f, void *vbuf, vlong offset, long count)
{
	int off, i, len;
	long rcount;
	Xfs *xf = f->xf;
	Inode *inode, *tinode;
	int nblock;
	DirEntry *edir;
	Iobuf *buffer, *ibuf, *tbuf;
	Dir pdir;
	Xfile ft;
	uchar *buf;
	char name[EXT2_NAME_LEN+1];
	unsigned int dirlen;
	int index;

	buf = vbuf;
	if (offset == 0)
		f->dirindex = 0;
	
	if( !S_ISDIR(getmode(f)) )
		return -1;

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;
	nblock = (inode->i_blocks * 512) / xf->block_size;
	ft = *f;
	chat("read block [ ");
	index = 0;
	for(i=0, rcount=0 ; (i < nblock) && (i < EXT2_NDIR_BLOCKS) ; i++){
		
		buffer = getbuf(xf, inode->i_block[i]);
		if( !buffer ){
			putbuf(ibuf);
			return -1;
		}
		chat("%d, ", buffer->addr);
		for(off=0 ; off < xf->block_size ;  ){
		
			edir = (DirEntry *)(buffer->iobuf + off);	
			off += edir->rec_len;
			if( (edir->name[0] == '.' ) && (edir->name_len == 1))
				continue;
			if(edir->name[0] == '.' && edir->name[1] == '.' && 
										edir->name_len == 2)
				continue;
			if( edir->inode == 0 ) /* for lost+found dir ... */
				continue;
			if( index++ < f->dirindex )
				continue;
			
			if( get_inode(&ft, edir->inode) < 0 ){
				chat("can't find ino no %d ] ...", edir->inode);
error:			putbuf(buffer);
				putbuf(ibuf);
				return -1;
			}
			tbuf = getbuf(xf, ft.bufaddr);
			if( !tbuf )
				goto error;
			tinode = ((Inode *)tbuf->iobuf) + ft.bufoffset;

			memset(&pdir, 0, sizeof(Dir));			
			
			/* fill plan9 dir struct */			
			pdir.name = name;
			len = (edir->name_len < EXT2_NAME_LEN) ? edir->name_len : EXT2_NAME_LEN;
			strncpy(pdir.name, edir->name, len);   
			pdir.name[len] = 0;
// chat("name %s len %d\n", pdir.name, edir->name_len);
			pdir.uid = mapuid(tinode->i_uid);
			pdir.gid = mapgid(tinode->i_gid);
			pdir.qid.path = edir->inode;
			pdir.mode = tinode->i_mode;
			if( edir->inode == EXT2_ROOT_INODE )
				pdir.qid.path = f->xf->rootqid.path;
			else if( S_ISDIR( tinode->i_mode) )
				pdir.qid.type |= QTDIR;
			if( pdir.qid.type & QTDIR )
				pdir.mode |= DMDIR;
			pdir.length = tinode->i_size;
			pdir.atime = tinode->i_atime;
			pdir.mtime = tinode->i_mtime;
		
			putbuf(tbuf);

			dirlen = convD2M(&pdir, &buf[rcount], count-rcount);
			if ( dirlen <= BIT16SZ ) {
				chat("] ...");
				putbuf(buffer);
				putbuf(ibuf);
				return rcount;
			}
			rcount += dirlen;
			f->dirindex++;

		}
		putbuf(buffer);
	}
	chat("] ...");
	putbuf(ibuf);
	return rcount;
}
int
bmap( Xfile *f, int block )
{
	Xfs *xf = f->xf;
	Inode *inode;
	Iobuf *buf, *ibuf;
	int addr;
	int addr_per_block = xf->addr_per_block;
	int addr_per_block_bits = ffz(~addr_per_block);
	
	if(block < 0) {
		chat("bmap() block < 0 ...");
		return 0;
	}
	if(block >= EXT2_NDIR_BLOCKS + addr_per_block +
		(1 << (addr_per_block_bits * 2)) +
		((1 << (addr_per_block_bits * 2)) << addr_per_block_bits)) {
		chat("bmap() block > big...");
		return 0;
	}

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return 0;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;

	/* direct blocks */
	if(block < EXT2_NDIR_BLOCKS){
		putbuf(ibuf);
		return inode->i_block[block];
	}
	block -= EXT2_NDIR_BLOCKS;
	
	/* indirect blocks*/
	if(block < addr_per_block) {
		addr = inode->i_block[EXT2_IND_BLOCK];
		if (!addr) goto error;
		buf = getbuf(xf, addr);
		if( !buf ) goto error;
		addr = *(((uint *)buf->iobuf) + block);
		putbuf(buf);
		putbuf(ibuf);
		return addr;	
	}
	block -= addr_per_block;
	
	/* double indirect blocks */
	if(block < (1 << (addr_per_block_bits * 2))) {
		addr = inode->i_block[EXT2_DIND_BLOCK];
		if (!addr) goto error;
		buf = getbuf(xf, addr);
		if( !buf ) goto error;
		addr = *(((uint *)buf->iobuf) + (block >> addr_per_block_bits));
		putbuf(buf);
		buf = getbuf(xf, addr);
		if( !buf ) goto error;
		addr = *(((uint *)buf->iobuf) + (block & (addr_per_block - 1)));
		putbuf(buf);
		putbuf(ibuf);
		return addr;
	}
	block -= (1 << (addr_per_block_bits * 2));

	/* triple indirect blocks */
	addr = inode->i_block[EXT2_TIND_BLOCK];
	if(!addr) goto error;
	buf = getbuf(xf, addr);
	if( !buf ) goto error;
	addr = *(((uint *)buf->iobuf) + (block >> (addr_per_block_bits * 2)));
	putbuf(buf);
	if(!addr) goto error;
	buf = getbuf(xf, addr);
	if( !buf ) goto error;
	addr = *(((uint *)buf->iobuf) +
			((block >> addr_per_block_bits) & (addr_per_block - 1)));
	putbuf(buf);
	if(!addr) goto error;
	buf = getbuf(xf, addr);
	if( !buf ) goto error;
	addr = *(((uint *)buf->iobuf) + (block & (addr_per_block - 1)));
	putbuf(buf);
	putbuf(ibuf);
	return addr;
error:
	putbuf(ibuf);
	return 0;
}
long
writefile(Xfile *f, void *vbuf, vlong offset, long count)
{
	Xfs *xf = f->xf;
	Inode *inode;
	Iobuf *buffer, *ibuf;
	long w;
	int len, o, cur_block, baddr;
	char *buf;

	buf = vbuf;

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;

	chat("write block [ ");
	cur_block = offset / xf->block_size;
	o = offset % xf->block_size;
	w = 0;
	while( count > 0 ){
		baddr = getblk(f, cur_block++);
		if( baddr <= 0 )
			goto end;
		buffer = getbuf(xf, baddr);
		if( !buffer )
			goto end;
		chat("%d ", baddr);
		len = xf->block_size - o;
		if( len > count )
			len = count;
		memcpy(&buffer->iobuf[o], &buf[w], len);
		dirtybuf(buffer);
		w += len;
		count -= len;
		o = 0;
		putbuf(buffer);
	}
end:
	if( inode->i_size < offset + w )
		inode->i_size = offset + w;
	inode->i_atime = inode->i_mtime = time(0);
	dirtybuf(ibuf);
	putbuf(ibuf);
	chat("]...");
	if( errno )
		return -1;
	return w;
}
int 
new_block( Xfile *f, int goal )
{
	Xfs *xf= f->xf;
	int group, block, baddr, k, redo;
	ulong lmap;
	char *p, *r;
	Iobuf *buf;
	Ext2 ed, es, eb;
	
	es = getext2(xf, EXT2_SUPER, 0);
	redo = 0;
 
repeat:
	
	if( goal < es.u.sb->s_first_data_block || goal >= es.u.sb->s_blocks_count )
		goal = es.u.sb->s_first_data_block;
	group = (goal - es.u.sb->s_first_data_block) / xf->blocks_per_group;

	ed = getext2(xf, EXT2_DESC, group);
	eb = getext2(xf, EXT2_BBLOCK, group);

	/* 
	 * First, test if goal block is free
	 */
	if( ed.u.gd->bg_free_blocks_count > 0 ){
		block = (goal - es.u.sb->s_first_data_block) % xf->blocks_per_group;
		
		if( !test_bit(block, eb.u.bmp) )
			goto got_block;
		
		if( block ){
			/*
			 * goal wasn't free ; search foward for a free 
			 * block within the next 32 blocks
			*/
			
			lmap = (((ulong *)eb.u.bmp)[block>>5]) >>
					((block & 31) + 1);
			if( block < xf->blocks_per_group - 32 )
				lmap |= (((ulong *)eb.u.bmp)[(block>>5)+1]) <<
					( 31-(block & 31) );
			else
				lmap |= 0xffffffff << ( 31-(block & 31) );

			if( lmap != 0xffffffffl ){
				k = ffz(lmap) + 1;
				if( (block + k) < xf->blocks_per_group ){
					block += k;
					goto got_block;
				}
			}			
		}
		/*
		 * Search in the remaider of the group
		*/
		p = eb.u.bmp + (block>>3);
		r = memscan(p, 0, (xf->blocks_per_group - block + 7) >>3);
		k = ( r - eb.u.bmp )<<3;
		if( k < xf->blocks_per_group ){
			block = k;
			goto search_back;
		}
		k = find_next_zero_bit((unsigned long *)eb.u.bmp, 
						xf->blocks_per_group>>3, block);
		if( k < xf->blocks_per_group ){
			block = k;
			goto got_block;
		}
	}

	/*
	 * Search the rest of groups
	*/
	putext2(ed); putext2(eb);
	for(k=0 ; k < xf->ngroups ; k++){
		group++;
		if( group >= xf->ngroups )
			group = 0;
		ed = getext2(xf, EXT2_DESC, group);
		if( ed.u.gd->bg_free_blocks_count > 0 )
			break;
		putext2(ed);
	}
	if( redo && group == xf->ngroups-1 ){
		putext2(ed);
		goto full;
	}
	if( k >=xf->ngroups ){
		/*
		 * All groups are full or
		 * we have retry (because the last block) and all other
		 * groups are also full.
		*/
full:	
		chat("no free blocks ...");
	 	putext2(es); 
		errno = Enospace;
		return 0;
	}
	eb = getext2(xf, EXT2_BBLOCK, group);
	r = memscan(eb.u.bmp,  0, xf->blocks_per_group>>3);
	block = (r - eb.u.bmp) <<3;
	if( block < xf->blocks_per_group )
		goto search_back;
	else
		block = find_first_zero_bit((ulong *)eb.u.bmp,
								xf->blocks_per_group>>3);
	if( block >= xf->blocks_per_group ){
		chat("Free block count courupted for block group %d...", group);
		putext2(ed); putext2(eb); putext2(es);
		errno = Ecorrupt;
		return 0;
	}


search_back:
	/*
	 * A free byte was found in the block. Now search backwards up
	 * to 7 bits to find the start of this group of free block.
	*/
	for(k=0 ; k < 7 && block > 0 && 
		!test_bit(block-1, eb.u.bmp) ; k++, block--);

got_block:

	baddr = block + (group * xf->blocks_per_group) + 
				es.u.sb->s_first_data_block;
	
	if( baddr == ed.u.gd->bg_block_bitmap ||
	     baddr == ed.u.gd->bg_inode_bitmap ){
		chat("Allocating block in system zone...");
		putext2(ed); putext2(eb); putext2(es);
		errno = Eintern;
		return 0;
	}

	if( set_bit(block, eb.u.bmp) ){
		chat("bit already set (%d)...", block);
		putext2(ed); putext2(eb); putext2(es);
		errno = Ecorrupt;
		return 0;
	}
	dirtyext2(eb);
	
	if( baddr >= es.u.sb->s_blocks_count ){
		chat("block >= blocks count...");
		errno = Eintern;
error:
		clear_bit(block, eb.u.bmp);
		putext2(eb); putext2(ed); putext2(es);
		return 0;
	}
	
	buf = getbuf(xf, baddr);
	if( !buf ){
		if( !redo ){
			/*
			 * It's perhaps the last block of the disk and 
			 * it can't be acceded because the last sector.
			 * Therefore, we try one more time with goal at 0
			 * to force scanning all groups.
			*/
			clear_bit(block, eb.u.bmp);
			putext2(eb); putext2(ed);
			goal = 0; errno = 0; redo++;
			goto repeat;
		}
		goto error;
	}
	memset(&buf->iobuf[0], 0, xf->block_size);
	dirtybuf(buf);
	putbuf(buf);

	es.u.sb->s_free_blocks_count--;
	dirtyext2(es);
	ed.u.gd->bg_free_blocks_count--;
	dirtyext2(ed);

	putext2(eb);
	putext2(ed);
	putext2(es);
	chat("new ");
	return baddr;
}
int
getblk(Xfile *f, int block)
{
	Xfs *xf = f->xf;
	int baddr;
	int addr_per_block = xf->addr_per_block;

	if (block < 0) {
		chat("getblk() block < 0 ...");
		return 0;
	}
	if(block > EXT2_NDIR_BLOCKS + addr_per_block +
			addr_per_block * addr_per_block +
			addr_per_block * addr_per_block * addr_per_block ){
		chat("getblk() block > big...");
		errno = Eintern;
		return 0;
	}
	if( block < EXT2_NDIR_BLOCKS )
		return inode_getblk(f, block);
	block -= EXT2_NDIR_BLOCKS;	
	if( block < addr_per_block ){
		baddr = inode_getblk(f, EXT2_IND_BLOCK);
		baddr = block_getblk(f, baddr, block);
		return baddr;
	}
	block -= addr_per_block;
	if( block < addr_per_block * addr_per_block  ){
		baddr = inode_getblk(f, EXT2_DIND_BLOCK);
		baddr = block_getblk(f, baddr, block / addr_per_block);
		baddr = block_getblk(f, baddr, block & ( addr_per_block-1));
		return baddr; 
	}
	block -= addr_per_block * addr_per_block;
	baddr = inode_getblk(f, EXT2_TIND_BLOCK);
	baddr = block_getblk(f, baddr, block / (addr_per_block * addr_per_block));
	baddr = block_getblk(f, baddr, (block / addr_per_block) & ( addr_per_block-1));
	return block_getblk(f, baddr, block & ( addr_per_block-1));
}
int
block_getblk(Xfile *f, int rb, int nr)
{
	Xfs *xf = f->xf;
	Inode *inode;
	int tmp, goal = 0;
	int blocks = xf->block_size / 512;
	Iobuf *buf, *ibuf;
	uint *p;
	Ext2 es;

	if( !rb )
		return 0;

	buf = getbuf(xf, rb);
	if( !buf )
		return 0;
	p = (uint *)(buf->iobuf) + nr;
	if( *p ){
		tmp = *p;
		putbuf(buf);
		return tmp;
	}

	for(tmp=nr - 1 ; tmp >= 0 ; tmp--){
		if( ((uint *)(buf->iobuf))[tmp] ){
			goal = ((uint *)(buf->iobuf))[tmp];
			break;
		}
	}
	if( !goal ){
		es = getext2(xf, EXT2_SUPER, 0);
		goal = (((f->inbr -1) / xf->inodes_per_group) *
				xf->blocks_per_group) +
				es.u.sb->s_first_data_block;
		putext2(es);
	}
	
	tmp = new_block(f, goal);
	if( !tmp ){
		putbuf(buf);
		return 0;
	}

	*p = tmp;
	dirtybuf(buf);
	putbuf(buf);
	
	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;
	inode->i_blocks += blocks;
	dirtybuf(ibuf);
	putbuf(ibuf);

	return tmp;
}
int 
inode_getblk(Xfile *f, int block)
{
	Xfs *xf = f->xf;
	Inode *inode;
	Iobuf *ibuf;
	int tmp, goal = 0;
	int blocks = xf->block_size / 512;
	Ext2 es;

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;


	if( inode->i_block[block] ){
		putbuf(ibuf);
		return inode->i_block[block];
	}

	for(tmp=block - 1 ; tmp >= 0 ; tmp--){
		if( inode->i_block[tmp] ){
			goal = inode->i_block[tmp];
			break;
		}
	}
	if( !goal ){
		es = getext2(xf, EXT2_SUPER, 0);
		goal = (((f->inbr -1) / xf->inodes_per_group) *
				xf->blocks_per_group) +
				es.u.sb->s_first_data_block;
		putext2(es);
	}

	tmp = new_block(f, goal);
	if( !tmp ){
		putbuf(ibuf);
		return 0;
	}

	inode->i_block[block] = tmp;
	inode->i_blocks += blocks;
	dirtybuf(ibuf);
	putbuf(ibuf);

	return tmp;
}
int 
new_inode(Xfile *f, int mode)
{
	Xfs *xf = f->xf;
	Inode *inode, *finode;
	Iobuf *buf, *ibuf;
	int ave,group, i, j;
	Ext2 ed, es, eb;

	group = -1;

	es = getext2(xf, EXT2_SUPER, 0);

	if( S_ISDIR(mode) ){	/* create directory inode */
		ave = es.u.sb->s_free_inodes_count / xf->ngroups;
		for(i=0 ; i < xf->ngroups ; i++){
			ed = getext2(xf, EXT2_DESC, i);
			if( ed.u.gd->bg_free_inodes_count &&
					ed.u.gd->bg_free_inodes_count >= ave ){
				if( group<0 || ed.u.gd->bg_free_inodes_count >
								ed.u.gd->bg_free_inodes_count )
					group = i;
			}
			putext2(ed);
		}

	}else{		/* create file inode */
		/* Try to put inode in its parent directory */
		i = (f->inbr -1) / xf->inodes_per_group;
		ed = getext2(xf, EXT2_DESC, i);
		if( ed.u.gd->bg_free_inodes_count ){
			group = i;
			putext2(ed);
		}else{
			/*
			 * Use a quadratic hash to find a group whith
			 * a free inode
			 */
			putext2(ed);
			for( j=1 ; j < xf->ngroups ; j <<= 1){
				i += j;
				if( i >= xf->ngroups )
					i -= xf->ngroups;
				ed = getext2(xf, EXT2_DESC, i);
				if( ed.u.gd->bg_free_inodes_count ){
					group = i;
					putext2(ed);
					break;
				}
				putext2(ed);
			}
		}
		if( group < 0 ){
			/* try a linear search */
			i = ((f->inbr -1) / xf->inodes_per_group) + 1;
			for(j=2 ; j < xf->ngroups ; j++){
				if( ++i >= xf->ngroups )
					i = 0;
				ed = getext2(xf, EXT2_DESC, i);
				if( ed.u.gd->bg_free_inodes_count ){
					group = i;
					putext2(ed);
					break;
				}
				putext2(ed);
			}
		}

	}
	if( group < 0 ){
		chat("group < 0...");
		putext2(es);
		return 0;
	}
	ed = getext2(xf, EXT2_DESC, group);
	eb = getext2(xf, EXT2_BINODE, group);
	if( (j = find_first_zero_bit(eb.u.bmp, 
			xf->inodes_per_group>>3)) < xf->inodes_per_group){
		if( set_bit(j, eb.u.bmp) ){
			chat("inode %d of group %d is already allocated...", j, group);
			putext2(ed); putext2(eb); putext2(es);
			errno = Ecorrupt;
			return 0;
		}
		dirtyext2(eb);
	}else if( ed.u.gd->bg_free_inodes_count != 0 ){
		chat("free inodes count corrupted for group %d...", group);
		putext2(ed); putext2(eb); putext2(es);
		errno = Ecorrupt;
		return 0;
	}
	i = j;
	j += group * xf->inodes_per_group + 1;
	if( j < EXT2_FIRST_INO || j >= es.u.sb->s_inodes_count ){
		chat("reserved inode or inode > inodes count...");
		errno = Ecorrupt;
error:
		clear_bit(i, eb.u.bmp);
		putext2(eb); putext2(ed); putext2(es);
		return 0;
	}
	
	buf = getbuf(xf, ed.u.gd->bg_inode_table +
			(((j-1) % xf->inodes_per_group) / 
			xf->inodes_per_block));
	if( !buf )
		goto error;
	inode = ((struct Inode *) buf->iobuf) + 
		((j-1) % xf->inodes_per_block);
	memset(inode, 0, sizeof(Inode));
	inode->i_mode = mode;
	inode->i_links_count = 1;
	inode->i_uid = DEFAULT_UID;
	inode->i_gid = DEFAULT_GID;
	inode->i_mtime = inode->i_atime = inode->i_ctime = time(0);
	dirtybuf(buf);

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf ){
		putbuf(buf);
		goto error;
	}
	finode = ((Inode *)ibuf->iobuf) + f->bufoffset;
	inode->i_flags = finode->i_flags;
	inode->i_uid = finode->i_uid;
	inode->i_gid = finode->i_gid;
	dirtybuf(ibuf);
	putbuf(ibuf);

	putbuf(buf);

	ed.u.gd->bg_free_inodes_count--;
	if( S_ISDIR(mode) )
		ed.u.gd->bg_used_dirs_count++;
	dirtyext2(ed);

	es.u.sb->s_free_inodes_count--;
	dirtyext2(es);

	putext2(eb);
	putext2(ed);
	putext2(es);

	return j;
}
int
create_file(Xfile *fdir, char *name, int mode)
{
	int inr;

	inr = new_inode(fdir, mode);
	if( !inr ){
		chat("create one new inode failed...");
		return -1;
	}
	if( add_entry(fdir, name, inr) < 0 ){
		chat("add entry failed...");	
		free_inode(fdir->xf, inr);
		return -1;
	}

	return inr;
}
void
free_inode( Xfs *xf, int inr)
{
	Inode *inode;
	ulong b, bg;
	Iobuf *buf;
	Ext2 ed, es, eb;

	bg = (inr -1) / xf->inodes_per_group;
	b = (inr -1) % xf->inodes_per_group;

	ed = getext2(xf, EXT2_DESC, bg);
	buf = getbuf(xf, ed.u.gd->bg_inode_table +
			(b / xf->inodes_per_block));
	if( !buf ){
		putext2(ed);
		return;
	}
	inode = ((struct Inode *) buf->iobuf) + 
		((inr-1) % xf->inodes_per_block);

	if( S_ISDIR(inode->i_mode) )
		ed.u.gd->bg_used_dirs_count--;
	memset(inode, 0, sizeof(Inode));
	inode->i_dtime = time(0);
	dirtybuf(buf);
	putbuf(buf);

	ed.u.gd->bg_free_inodes_count++;
	dirtyext2(ed);
	putext2(ed);

	eb = getext2(xf, EXT2_BINODE, bg);
	clear_bit(b, eb.u.bmp);
	dirtyext2(eb);
	putext2(eb);
	
	es = getext2(xf, EXT2_SUPER, 0);
	es.u.sb->s_free_inodes_count++;
	dirtyext2(es); putext2(es);
}
int
create_dir(Xfile *fdir, char *name, int mode)
{
	Xfs *xf = fdir->xf;
	DirEntry *de;
	Inode *inode;
	Iobuf *buf, *ibuf;
	Xfile tf;
	int inr, baddr;

	inr = new_inode(fdir, mode);
	if( inr == 0 ){
		chat("create one new inode failed...");
		return -1;
	}
	if( add_entry(fdir, name, inr) < 0 ){
		chat("add entry failed...");
		free_inode(fdir->xf, inr);
		return -1;
	}

	/* create the empty dir */

	tf = *fdir;
	if( get_inode(&tf, inr) < 0 ){
		chat("can't get inode %d...", inr);
		free_inode(fdir->xf, inr);
		return -1;
	}

	ibuf = getbuf(xf, tf.bufaddr);
	if( !ibuf ){
		free_inode(fdir->xf, inr);
		return -1;
	}
	inode = ((Inode *)ibuf->iobuf) + tf.bufoffset;

	
	baddr = inode_getblk(&tf, 0);
	if( !baddr ){
		putbuf(ibuf);
		ibuf = getbuf(xf, fdir->bufaddr);
		if( !ibuf ){
			free_inode(fdir->xf, inr);
			return -1;
		}
		inode = ((Inode *)ibuf->iobuf) + fdir->bufoffset;
		delete_entry(fdir->xf, inode, inr);
		putbuf(ibuf);
		free_inode(fdir->xf, inr);
		return -1;
	}	
	
	inode->i_size = xf->block_size;	
	buf = getbuf(xf, baddr);
	
	de = (DirEntry *)buf->iobuf;
	de->inode = inr;
	de->name_len = 1;
	de->rec_len = DIR_REC_LEN(de->name_len);
	strcpy(de->name, ".");
	
	de = (DirEntry *)( (char *)de + de->rec_len);
	de->inode = fdir->inbr;
	de->name_len = 2;
	de->rec_len = xf->block_size - DIR_REC_LEN(1);
	strcpy(de->name, "..");
	
	dirtybuf(buf);
	putbuf(buf);
	
	inode->i_links_count = 2;
	dirtybuf(ibuf);
	putbuf(ibuf);
	
	ibuf = getbuf(xf, fdir->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + fdir->bufoffset;

	inode->i_links_count++;

	dirtybuf(ibuf);
	putbuf(ibuf);

	return inr;
}
int
add_entry(Xfile *f, char *name, int inr)
{
	Xfs *xf = f->xf;
	DirEntry *de, *de1;
	int offset, baddr;
	int rec_len, cur_block;
	int namelen = strlen(name);
	Inode *inode;
	Iobuf *buf, *ibuf;

	ibuf = getbuf(xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;

	if( inode->i_size == 0 ){
		chat("add_entry() no entry !!!...");
		putbuf(ibuf);
		return -1;
	}
	cur_block = offset = 0;
	rec_len = DIR_REC_LEN(namelen);
	buf = getbuf(xf, inode->i_block[cur_block++]);
	if( !buf ){
		putbuf(ibuf);
		return -1;
	}
	de = (DirEntry *)buf->iobuf;
	
	for(;;){
		if( ((char *)de) >= (xf->block_size + buf->iobuf) ){
			putbuf(buf);
			if( cur_block >= EXT2_NDIR_BLOCKS ){
				errno = Enospace;
				putbuf(ibuf);
				return -1;
			}
			if( (baddr = inode_getblk(f, cur_block++)) == 0 ){
				putbuf(ibuf);
				return -1;
			}
			buf = getbuf(xf, baddr);
			if( !buf ){
				putbuf(ibuf);
				return -1;
			}
			if( inode->i_size <= offset ){
				de  = (DirEntry *)buf->iobuf;
				de->inode = 0;
				de->rec_len = xf->block_size;
				dirtybuf(buf);
				inode->i_size = offset + xf->block_size;
				dirtybuf(ibuf);
			}else{
				de = (DirEntry *)buf->iobuf;
			}
		}
		if( de->inode != 0 && de->name_len == namelen &&
				!strncmp(name, de->name, namelen) ){
			errno = Eexist;
			putbuf(ibuf); putbuf(buf);
			return -1;
		}
		offset += de->rec_len;
		if( (de->inode == 0 && de->rec_len >= rec_len) ||
				(de->rec_len >= DIR_REC_LEN(de->name_len) + rec_len) ){
			if( de->inode ){
				de1 = (DirEntry *) ((char *)de + DIR_REC_LEN(de->name_len));
				de1->rec_len = de->rec_len - DIR_REC_LEN(de->name_len);
				de->rec_len = DIR_REC_LEN(de->name_len);
				de = de1;
			}	
			de->inode = inr;
			de->name_len = namelen;
			memcpy(de->name, name, namelen);
			dirtybuf(buf);
			putbuf(buf);
			inode->i_mtime = inode->i_ctime = time(0);
			dirtybuf(ibuf);
			putbuf(ibuf);
			return 0;
		}
		de = (DirEntry *)((char *)de + de->rec_len);
	}
	/* not reached */
}
int
unlink( Xfile *file )
{
	Xfs *xf = file->xf;	
	Inode *dir;
	int bg, b;
	Inode *inode;
	Iobuf *buf, *ibuf;
	Ext2 ed, es, eb;

	if( S_ISDIR(getmode(file)) && !empty_dir(file) ){
			chat("non empty directory...");
			errno = Eperm;
			return -1;
	}

	es = getext2(xf, EXT2_SUPER, 0);

	/* get dir inode */
	if( file->pinbr >= es.u.sb->s_inodes_count ){
    		chat("inode number %d is too big...",  file->pinbr);
		putext2(es);
		errno = Eintern;
    		return -1;
	}
	bg = (file->pinbr - 1) / xf->inodes_per_group;
	if( bg >= xf->ngroups ){
		chat("block group (%d) > groups count...", bg);
		putext2(es);
		errno = Eintern;
		return -1;
	}
	ed = getext2(xf, EXT2_DESC, bg);
	b = ed.u.gd->bg_inode_table +
			(((file->pinbr-1) % xf->inodes_per_group) / 
			xf->inodes_per_block);
	putext2(ed);
	buf = getbuf(xf, b);
	if( !buf ){	
		putext2(es);	
		return -1;
	}
	dir = ((struct Inode *) buf->iobuf) + 
		((file->pinbr-1) % xf->inodes_per_block);

	/* Clean dir entry */
	
	if( delete_entry(xf, dir, file->inbr) < 0 ){
		putbuf(buf);
		putext2(es);
		return -1;
	}
	if( S_ISDIR(getmode(file)) ){
		dir->i_links_count--;
		dirtybuf(buf);
	}
	putbuf(buf);
	
	/* clean blocks */
	ibuf = getbuf(xf, file->bufaddr);
	if( !ibuf ){
		putext2(es);
		return -1;
	}
	inode = ((Inode *)ibuf->iobuf) + file->bufoffset;

	if( !S_ISLNK(getmode(file)) || 
		(S_ISLNK(getmode(file)) && (inode->i_size > EXT2_N_BLOCKS<<2)) )
		if( free_block_inode(file) < 0 ){
			chat("error while freeing blocks...");
			putext2(es);
			putbuf(ibuf);
			return -1;
		}
	

	/* clean inode */	
	
	bg = (file->inbr -1) / xf->inodes_per_group;
	b = (file->inbr -1) % xf->inodes_per_group;

	eb = getext2(xf, EXT2_BINODE, bg);
	clear_bit(b, eb.u.bmp);
	dirtyext2(eb);
	putext2(eb);

	inode->i_dtime = time(0);
	inode->i_links_count--;
	if( S_ISDIR(getmode(file)) )
		inode->i_links_count = 0;

	es.u.sb->s_free_inodes_count++;
	dirtyext2(es);
	putext2(es);

	ed = getext2(xf, EXT2_DESC, bg);
	ed.u.gd->bg_free_inodes_count++;
	if( S_ISDIR(getmode(file)) )
		ed.u.gd->bg_used_dirs_count--;
	dirtyext2(ed);
	putext2(ed);

	dirtybuf(ibuf);
	putbuf(ibuf);

	return 1;
}
int
empty_dir(Xfile *dir)
{
	Xfs *xf = dir->xf;
	int nblock;
	uint offset, i,count;
	DirEntry *de;
	Inode *inode;
	Iobuf *buf, *ibuf;
	
	if( !S_ISDIR(getmode(dir)) )
		return 0;

	ibuf = getbuf(xf, dir->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + dir->bufoffset;
	nblock = (inode->i_blocks * 512) / xf->block_size;

	for(i=0, count=0 ; (i < nblock) && (i < EXT2_NDIR_BLOCKS) ; i++){
		buf = getbuf(xf, inode->i_block[i]);
		if( !buf ){
			putbuf(ibuf);
			return 0;
		}
		for(offset=0 ; offset < xf->block_size ;  ){
			de = (DirEntry *)(buf->iobuf + offset);
			if(de->inode)
				count++;
			offset += de->rec_len;
		}
		putbuf(buf);
		if( count > 2 ){
			putbuf(ibuf);
			return 0;
		}
	}
	putbuf(ibuf);
	return 1;
}
int 
free_block_inode(Xfile *file)
{
	Xfs *xf = file->xf;
	int i, j, k;
	ulong b, *y, *z;
	uint *x;
	int naddr;
	Inode *inode;
	Iobuf *buf, *buf1, *buf2, *ibuf;

	ibuf = getbuf(xf, file->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + file->bufoffset;

	for(i=0 ; i < EXT2_IND_BLOCK ; i++){
		x = inode->i_block + i;
		if( *x == 0 ){ putbuf(ibuf); return 0; }
		free_block(xf, *x);
	}
	naddr = xf->addr_per_block;

	/* indirect blocks */
	
	if( (b=inode->i_block[EXT2_IND_BLOCK]) ){
		buf = getbuf(xf, b);
		if( !buf ){ putbuf(ibuf); return -1; }
		for(i=0 ; i < naddr ; i++){
			x = ((uint *)buf->iobuf) + i;
			if( *x == 0 ) break;
			free_block(xf, *x);
		}
		free_block(xf, b);
		putbuf(buf);
	}

	/* double indirect block */

	if( (b=inode->i_block[EXT2_DIND_BLOCK]) ){
		buf = getbuf(xf, b);
		if( !buf ){ putbuf(ibuf); return -1; }
		for(i=0 ; i < naddr ; i++){
			x = ((uint *)buf->iobuf) + i;
			if( *x== 0 ) break;
			buf1 = getbuf(xf, *x);
			if( !buf1 ){ putbuf(buf); putbuf(ibuf); return -1; }
			for(j=0 ; j < naddr ; j++){
				y = ((ulong *)buf1->iobuf) + j;
				if( *y == 0 ) break;
				free_block(xf, *y);
			}
			free_block(xf, *x);
			putbuf(buf1);
		}
		free_block(xf, b);
		putbuf(buf);
	}
	
	/* triple indirect block */
	
	if( (b=inode->i_block[EXT2_TIND_BLOCK]) ){
		buf = getbuf(xf, b);
		if( !buf ){ putbuf(ibuf); return -1; }
		for(i=0 ; i < naddr ; i++){
			x = ((uint *)buf->iobuf) + i;
			if( *x == 0 ) break;
			buf1 = getbuf(xf, *x);
			if( !buf1 ){ putbuf(buf); putbuf(ibuf); return -1; }
			for(j=0 ; j < naddr ; j++){
				y = ((ulong *)buf1->iobuf) + j;
				if( *y == 0 ) break;
				buf2 = getbuf(xf, *y);
				if( !buf2 ){ putbuf(buf); putbuf(buf1); putbuf(ibuf); return -1; }
				for(k=0 ; k < naddr ; k++){
					z = ((ulong *)buf2->iobuf) + k;
					if( *z == 0 ) break;
					free_block(xf, *z);
				}
				free_block(xf, *y);
				putbuf(buf2);
			}
			free_block(xf, *x);
			putbuf(buf1);
		}
		free_block(xf, b);
		putbuf(buf);
	}

	putbuf(ibuf);
	return 0;
}
void free_block( Xfs *xf, ulong block )
{
	ulong bg;
	Ext2 ed, es, eb;

	es = getext2(xf, EXT2_SUPER, 0);

	bg = (block - es.u.sb->s_first_data_block) / xf->blocks_per_group;
	block = (block - es.u.sb->s_first_data_block) % xf->blocks_per_group;

	eb = getext2(xf, EXT2_BBLOCK, bg);
	clear_bit(block, eb.u.bmp);
	dirtyext2(eb);
	putext2(eb);

	es.u.sb->s_free_blocks_count++;
	dirtyext2(es);
	putext2(es);

	ed = getext2(xf, EXT2_DESC, bg);
	ed.u.gd->bg_free_blocks_count++;
	dirtyext2(ed);
	putext2(ed);

}
int 
delete_entry(Xfs *xf, Inode *inode, int inbr)
{
	int nblock = (inode->i_blocks * 512) / xf->block_size;
	uint offset, i;
	DirEntry *de, *pde;
	Iobuf *buf;
	
	if( !S_ISDIR(inode->i_mode) )
		return -1;

	for(i=0 ; (i < nblock) && (i < EXT2_NDIR_BLOCKS) ; i++){
		buf = getbuf(xf, inode->i_block[i]);
		if( !buf )
			return -1;
		pde = 0;
		for(offset=0 ; offset < xf->block_size ;  ){
			de = (DirEntry *)(buf->iobuf + offset);
			if( de->inode == inbr ){
				if( pde )
					pde->rec_len += de->rec_len;
				de->inode = 0;
				dirtybuf(buf);
				putbuf(buf);
				return 1;
			}
			offset += de->rec_len;
			pde = de;
		}
		putbuf(buf);

	}
	errno = Enonexist;
	return -1;
}
int
truncfile(Xfile *f)
{
	Inode *inode;
	Iobuf *ibuf;
	chat("trunc(fid=%d) ...", f->fid);
	ibuf = getbuf(f->xf, f->bufaddr);
	if( !ibuf )
		return -1;
	inode = ((Inode *)ibuf->iobuf) + f->bufoffset;
	
	if( free_block_inode(f) < 0 ){
		chat("error while freeing blocks...");
		putbuf(ibuf);
		return -1;
	}
	inode->i_atime = inode->i_mtime = time(0);
	inode->i_blocks = 0;
	inode->i_size = 0;
	memset(inode->i_block, 0, EXT2_N_BLOCKS*sizeof(ulong));
	dirtybuf(ibuf);
	putbuf(ibuf);
	chat("trunc ok...");
	return 0;
}
long
getmode(Xfile *f)
{
	Iobuf *ibuf;
	long mode;

	ibuf = getbuf(f->xf, f->bufaddr);
	if( !ibuf )
		return -1;
	mode = (((Inode *)ibuf->iobuf) + f->bufoffset)->i_mode;
	putbuf(ibuf);
	return mode;
}
void
CleanSuper(Xfs *xf)
{
	Ext2 es;

	es = getext2(xf, EXT2_SUPER, 0);
	es.u.sb->s_state = EXT2_VALID_FS;
	dirtyext2(es);
	putext2(es);
}
int 
test_bit(int i, void *data)
{
	char *pt = (char *)data;

	return pt[i>>3] & (0x01 << (i&7));
}

int
set_bit(int i, void *data)
{
  	char *pt;

  	if( test_bit(i, data) )
    		return 1; /* bit already set !!! */
  
  	pt = (char *)data;
  	pt[i>>3] |= (0x01 << (i&7));

  	return 0;
}

int 
clear_bit(int i, void *data)
{
	char *pt;

  	if( !test_bit(i, data) )
    		return 1; /* bit already clear !!! */
  
 	 pt = (char *)data;
  	pt[i>>3] &= ~(0x01 << (i&7));
	
	return 0;
}
void *
memscan( void *data, int c, int count )
{
	char *pt = (char *)data;

	while( count ){
		if( *pt == c )
			return (void *)pt;
		count--;
		pt++;
	}
	return (void *)pt;
}

int 
find_first_zero_bit( void *data, int count /* in byte */)
{
  char *pt = (char *)data;
  int n, i;
  
  n = 0;

  while( n < count ){
    for(i=0 ; i < 8 ; i++)
      if( !(*pt & (0x01 << (i&7))) )
	return (n<<3) + i;
    n++; pt++;
  }
  return n << 3;
}

int 
find_next_zero_bit( void *data, int count /* in byte */, int where)
{
  char *pt = (((char *)data) + (where >> 3));
  int n, i;
  
  n = where >> 3;
  i = where & 7;

  while( n < count ){
    for(; i < 8 ; i++)
      if( !(*pt & (0x01 << (i&7))) )
	return (n<<3) + i;
    n++; pt++; i=0;
  }
  return n << 3;
}
int
ffz( int x )
{
	int c = 0;
	while( x&1 ){
		c++;
		x >>= 1;
	}
	return c;
}
