#!/bin/bash
set -e

if [ $OLD_BUILD == true ]; then
	export SH=`which rc`
	git clean -x -d -f
	(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

	build all
else

	curl -L http://sevki.co/get-build -o util/bldy
	chmod +x util/bldy

	if [[  ($CC == "clang") && ("${TRAVIS_PULL_REQUEST}" = "false" ) ]]; then

			scan-build ./util/bldy //.:kernel

			curl -L http://sevki.co/4qf_NS -o util/scanscan
			chmod +x util/scanscan

			#scanscan finds and uploads scan-build files
			./util/scanscan
 	else
		./util/bldy //.:kernel

	fi
fi
