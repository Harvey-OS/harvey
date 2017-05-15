#!/bin/sh

# Prints out a list of all UFS files that cannot be fully parsed by spatch

for file in ../{ufs,ffs}/*.[ch]; do
	if [[ $(spatch --parse-c $file 2>&1 | grep ^bad:) ]]; then
		echo $file
	fi
done
