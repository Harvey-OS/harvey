#!/bin/sh

echo Building the build tool...

GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -u github.com/Harvey-OS/ninep/cmd/ufs
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -f -u bldy.build/bldy
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get harvey/cmd/...


# this will make booting a VM easier
mkdir -p tmp

cat <<EOF
# We support RISC-V, but the default is x86_64 (which we call amd64 for historical reasons):
export ARCH=amd64
# And build:
./util/build
# See \`build -h' for more information on the build tool.

To build with bldy just type bldy //:kernel 
To enable access to files, create a harvey and none user:
sudo useradd harvey
sudo useradd none

none is only required for drawterm/cpu access
EOF
