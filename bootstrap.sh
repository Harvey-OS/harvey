#!/bin/bash

git submodule init
git submodule update
echo Building the build tool...
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -d -v harvey/cmd/... # should really vendor these bits
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go install github.com/rminnich/ninep/srv/examples/ufs harvey/cmd/...

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF
For now, we have on architecture:
export ARCH=amd64
And build:
./util/build
See \`build -h' for more information on the build tool.
EOF
