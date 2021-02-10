#!/bin/sh
set -e

# We assume you're running this from the Harvey dir
HARVEY=`pwd`

if [ -z "$OS" ] || [ -z "$ARCH" ]
then
	echo We need \$OS and \$ARCH set.
	exit
fi

if [ ! -e apex ]
then
	git clone https://github.com/Harvey-OS/apex.git
fi

cd apex
APEX=`pwd`
cd src
APEX=$APEX HARVEY=$HARVEY OS=$OS ARCH=$ARCH make
cd ../..
