#!/bin/bash
set -e

if [ "${COVERITY_SCAN_BRANCH}" != 1 ]; then
	export SH=`which rc`
	git clean -x -d -f
	(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

	build -v //.:kernel
fi
