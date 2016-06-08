#!/bin/bash

git submodule init
git submodule update

GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -d harvey/cmd/... # should really vendor these bits
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go install github.com/rminnich/ninep/srv/examples/ufs harvey/cmd/...

echo Downloading the blaze tool...
curl -L http://sevki.co/get-build -o util/blaze
chmod +x util/blaze


# this will make booting a VM easier
mkdir -p tmp

cat <<EOF
# We support RISC-V, but the default is x86_64 (which we call amd64 for historical reasons):
export ARCH=amd64
# And build:
./util/build
# See \`build -h' for more information on the build tool.

or try the experimental build 
./util/blaze -v //:kernel

To enable access to files, create a harvey and none user:
sudo useradd harvey
sudo useradd none

none is only required for drawterm/cpu access
EOF
