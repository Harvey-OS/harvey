#!/bin/bash
set -e

if [ "${COVERITY_SCAN_BRANCH}" != 1 ]; then
	if [ $OLD_BUILD == true ]; then
		export SH=`which rc`
		git clean -x -d -f
		(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

		echo
		echo "Vendorized code verification..."
		echo
		for v in `find|grep vendor.json`; do
			echo "cd `dirname $v`"
			(cd `dirname $v`; vendor -check)
		done
		echo

		build all
	else
		curl -L http://sevki.co/get-build -o util/build
		chmod +x util/build

		./util/build -v //.:kernel
	fi
fi