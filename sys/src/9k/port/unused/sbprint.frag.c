#pragma varargck argpos se 2

static char *
sbprint(Block *bp, char *fmt, ...)
{
	char *end;
	va_list args;

	va_start(args, fmt);
	end = vseprint((char *)bp->wp, (char *)bp->lim, fmt, args);
	va_end(args);
	bp->wp = (uchar *)end;
	return end;
}
