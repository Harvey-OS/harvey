#!/bin/bash -eu
#
# Copyright (C) 2018 Harvey OS
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#!/bin/bash
set -e

if [ $OLD_BUILD == true ]; then
	export SH=`which rc`
	git clean -x -d -f
	(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

	build all
else

	(cd $TRAVIS_BUILD_DIR && ./bootstrap.sh)

	if [[  ($CC == "clang-3.8") && ("${TRAVIS_PULL_REQUEST}" = "false" ) ]]; then

			scan-build-3.8 ./util/bldy //.:kernel

			curl -L http://sevki.co/4qf_NS -o util/scanscan
			chmod +x util/scanscan

			#scanscan finds and uploads scan-build files
			./util/scanscan
 	else
		./util/bldy //.:kernel

	fi
fi
