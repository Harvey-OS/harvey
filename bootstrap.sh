#!/bin/sh

echo Building the build tool...

GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get github.com/Harvey-OS/ninep/cmd/ufs harvey/cmd/...


echo Downloading the blaze tool...
curl -L http://sevki.co/get-build -o util/nuke
chmod +x util/nuke

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF
# We support RISC-V, but the default is x86_64 (which we call amd64 for historical reasons):
export ARCH=amd64
# And build:
./util/build
# See \`build -h' for more information on the build tool.

To enable access to files, create a harvey and none user:
sudo useradd harvey
sudo useradd none

none is only required for drawterm/cpu access
EOF
