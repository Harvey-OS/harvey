#!/bin/bash
set -e

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

	if [[  ($CC == "clang") && ("${TRAVIS_PULL_REQUEST}" = "false" ) ]]; then 
		
			scan-build ./util/build -v //.:kernel

			curl -L http://sevki.co/4qf_NS -o util/scanscan
			chmod +x util/scanscan

			#scanscan finds and uploads scan-build files
			./util/scanscan
 	else 
		./util/build -v //.:kernel

	fi
fi

