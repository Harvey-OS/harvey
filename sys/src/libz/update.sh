#!/bin/sh
if [ -z "$1" ]; then
	echo need URL >&2
	exit 1
fi

rm -rf upstream/*
curl -L# "$1" | tar xz -C upstream --strip-components 1 \
	--exclude amiga \
	--exclude as400 \
	--exclude contrib \
	--exclude msdos \
	--exclude nintendods \
	--exclude old \
	--exclude watcom \
	--exclude win32 \
	--exclude qnx
