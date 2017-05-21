#!/bin/bash

for file in ../ufs/*.[ch]; do
	# Angle brackets to double quotes
	sed -i '/^#include <[uf]fs/s/[<>]/"/g' $file

	# ufs/ufs/ to local
	sed -i 's%^#include "ufs/ufs/%#include "%' $file

	# ufs/ffs/ to ../
	sed -i 's%^#include "ufs/ffs/%#include "../ffs/%' $file
done

for file in ../ffs/*.[ch]; do
	# Angle brackets to double quotes
	sed -i '/^#include <[uf]fs/s/[<>]/"/g' $file

	# ufs/ufs/ to ../
	sed -i 's%^#include "ufs/ufs/%#include "../ufs/%' $file

	# ufs/ffs/ to local
	sed -i 's%^#include "ufs/ffs/%#include "%' $file
done
