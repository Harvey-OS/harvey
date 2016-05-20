#!/bin/bash

git submodule init
git submodule update

GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -d harvey/cmd/... # should really vendor these bits
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go install github.com/rminnich/ninep/srv/examples/ufs harvey/cmd/...

if [ $OLD_BUILD != true ]; then
	echo Downloading the build tool...
	curl -L http://sevki.co/get-build -o util/build
	chmod +x util/build
fi

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF
# We support RISC-V, but the default is x86_64 (which we call amd64 for historical reasons):
export ARCH=amd64
# And build:
./util/build -v //:kernel
# See \`build -h' for more information on the build tool.
EOF
