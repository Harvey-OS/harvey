#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

enum {
	Index,
	Type,
	Flags,
	DPL,
	Base,
	Limit,
	Nfields,
};

static int
descempty(struct linux_user_desc *info)
{
	return info->base_addr==0 && info->limit==0 &&
		info->contents==0 && info->read_exec_only==1 &&
		info->seg_32bit==0 && info->limit_in_pages==0 &&
		info->seg_not_present==1 && info->useable==0;
}

int sys_set_thread_area(void *pinfo)
{
	struct linux_user_desc *info = pinfo;
	char buf[1024];
	char *p, *e, *f[Nfields];
	int n, fd, idx, err;

	trace("sys_set_thread_area(%p)", pinfo);

	err = -ENOSYS;
	if((fd = open("/dev/gdt", ORDWR)) < 0)
		goto out;

	idx = info->entry_number;
	if(idx == -1){
		err = -ESRCH;
		if((n = read(fd, buf, sizeof(buf)-1)) <= 0)
			goto out;
		buf[n] = 0;
		p = buf;
		while(e = strchr(p, '\n')){
			*e = 0;
			if(getfields(p, f, nelem(f), 1, " ") != nelem(f))
				goto out;
			idx = strtoul(f[Index], nil, 16);
			if(idx >= 8*sizeof(current->tlsmask))
				break;
			if((current->tlsmask & (1<<idx)) == 0)
				goto found;
			p = e+1;
		}
		goto out;
	}

found:
	err = -EINVAL;
	if(idx < 0 || idx >= 8*sizeof(current->tlsmask))
		goto out;

	buf[0] = 0;
	if(!info->seg_not_present)
		strcat(buf, "P");
	if(info->limit_in_pages)
		strcat(buf, "G");
	if(info->useable)
		strcat(buf, "U");
	if(info->contents & 2){
		/* code segment */
		if(info->contents & 1)
			strcat(buf, "C");
		if(info->seg_32bit)
			strcat(buf, "D");
		if(!info->read_exec_only)
			strcat(buf, "R");
		if(buf[0] == 0)
			strcat(buf, "-");

		if(fprint(fd, "%x code %s 3 %lux %lux\n",
			idx, buf, (ulong)info->base_addr, (ulong)info->limit) < 0)
			goto out;
	} else {
		/* data segment */
		if(info->contents & 1)
			strcat(buf, "E");
		if(info->seg_32bit)
			strcat(buf, "B");
		if(!info->read_exec_only)
			strcat(buf, "W");
		if(buf[0] == 0)
			strcat(buf, "-");

		if(fprint(fd, "%x data %s 3 %lux %lux\n",
			idx, buf, (ulong)info->base_addr, (ulong)info->limit) < 0)
			goto out;
	}

	err = 0;
	info->entry_number = idx;
	if(!descempty(info)){
		current->tlsmask |= 1<<idx;
	} else {
		current->tlsmask &= ~(1<<idx);
	}

out:
	if(fd >= 0)
		close(fd);
	return err;
}

int sys_get_thread_area(void *pinfo)
{
	struct linux_user_desc *info = pinfo;
	int err, n, fd, idx;
	char buf[1024];
	char *p, *e, *f[Nfields];

	trace("sys_get_thread_area(%p)", pinfo);

	err = -ENOSYS;
	if((fd = open("/dev/gdt", OREAD)) < 0)
		goto out;

	err = -EINVAL;
	if((n = read(fd, buf, sizeof(buf)-1)) <= 0)
		goto out;
	buf[n] = 0;
	p = buf;
	while(e = strchr(p, '\n')){
		*e = 0;
		if(getfields(p, f, nelem(f), 1, " ") != nelem(f))
			goto out;
		idx = strtoul(f[Index], nil, 16);
		if(idx >= 8*sizeof(current->tlsmask))
			break;
		if(idx == info->entry_number)
			goto found;
		p = e+1;
	}
	goto out;

found:
	info->contents = 0;
	if(strcmp(f[Type], "code") == 0)
		info->contents |= 2;
	info->seg_not_present = 1;
	info->limit_in_pages = 0;
	info->seg_32bit = 0;
	info->read_exec_only = 1;
	info->useable = 0;
	for(p = f[Flags]; *p; p++){
		switch(*p){
		case 'P':
			info->seg_not_present = 0;
			break;
		case 'G':
			info->limit_in_pages = 1;
			break;
		case 'B':
		case 'D':
			info->seg_32bit = 1;
			break;
		case 'W':
		case 'R':
			info->read_exec_only = 0;
			break;
		case 'U':
			info->useable = 1;
			break;
		case 'E':
		case 'C':
			info->contents |= 1;
			break;
		}
	}

	info->base_addr = strtoul(f[Base], nil, 16);
	info->limit = strtoul(f[Limit], nil, 16);

	err = 0;

out:
	if(fd >= 0)
		close(fd);
	return err;
}

static void
cleardesc(struct linux_user_desc *info)
{
	info->base_addr=0;
	info->limit=0;
	info->contents=0;
	info->read_exec_only=1;
	info->seg_32bit=0;
	info->limit_in_pages=0;
	info->seg_not_present=1;
	info->useable=0;
}

void inittls(void)
{
	struct linux_user_desc info;
	int i;

	for(i=0; i<8*sizeof(current->tlsmask); i++){
		if((current->tlsmask & (1 << i)) == 0)
			continue;
		cleardesc(&info);
		info.entry_number = i;
		sys_set_thread_area(&info);
	}
	current->tlsmask = 0;
}

void clonetls(Uproc *new)
{
	new->tlsmask = current->tlsmask;
}

int sys_modify_ldt(int func, void *data, int count)
{
	trace("sys_modify_ldt(%d, %p, %x)", func, data, count);

	return -ENOSYS;
}
