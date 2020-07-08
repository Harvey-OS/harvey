#!/bin/sh

echo Building the build tool...

GO111MODULE=on GOBIN="$(pwd)/util" go get -u github.com/Harvey-OS/ninep/cmd/ufs
# GO111MODULE=on GOBIN="$(pwd)/util" go get -f -u bldy.build/bldy
GO111MODULE=on GOBIN="$(pwd)/util" go get ./util/src/harvey/cmd/...

GOOS=plan9 GOARCH=amd64 GO111MODULE=on GOBIN="$(pwd)/util" go get golang.org/dl/gotip


# this will make booting a VM easier
mkdir -p tmp

cat <<EOF
# We support RISC-V, but the default is x86_64 (which we call amd64 for historical reasons):
export ARCH=amd64
# You also need to export your C compiler flavor (gcc, clang, gcc-7...)
export CC=gcc
# And build:
./util/build
# See \`build -h' for more information on the build tool.

To enable access to files, create a harvey and none user:
sudo useradd harvey
sudo useradd none

none is only required for drawterm/cpu access
EOF
