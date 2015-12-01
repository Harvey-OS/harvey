#!/bin/bash
set -e

if [ "${COVERITY_SCAN_BRANCH}" != 1 ]; then
	git clean -x -d -f
	./BUILD utils

	echo
	echo "Vendorized code verification..."
	echo
	for v in `find|grep vendor.json`; do
		echo "cd `dirname $v`"
		(cd `dirname $v`; vendor -check)
	done

	./BUILD all
fi
