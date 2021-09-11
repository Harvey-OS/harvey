#!/bin/sh
grep git: commands | while read line
do
	dest=`echo $line | awk '{print $3}'`
	cmd=`echo $line | awk '{print $2 " " $3}'`
	if [ ! -e $dest ]
	then
		echo git clone --depth 1 $cmd
		git clone --depth 1 $cmd
	fi
done
