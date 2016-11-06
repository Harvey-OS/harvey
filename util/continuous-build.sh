#!/bin/bash
set -e

export SH=`which rc`
git clean -x -d -f
(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

if [ $OLD_BUILD == true ]; then
	build all
else
	if [[  ($CC == "clang-3.8") && ("${TRAVIS_PULL_REQUEST}" = "false" ) ]]; then

			scan-build ./util/bldy //.:kernel

			curl -L http://sevki.co/4qf_NS -o util/scanscan
			chmod +x util/scanscan

			#scanscan finds and uploads scan-build files
			./util/scanscan
 	else
		./util/bldy -v //.:kernel
	fi
fi
