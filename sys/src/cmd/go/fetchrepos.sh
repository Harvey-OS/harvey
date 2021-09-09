#!/bin/sh
while read line
do
	echo git clone --depth 1 $line
	git clone --depth 1 $line
done < commands
