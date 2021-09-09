#!/bin/sh
while read line
do
	dest=`echo $line | awk '{print $2}'`
	if [ ! -e $dest ]
	then
		echo git clone --depth 1 $line
		git clone --depth 1 $line
	fi
done < commands
