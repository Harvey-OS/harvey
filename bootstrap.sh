#!/bin/sh

set -e

export GOBIN=$(pwd)/$(go env GOHOSTOS)_$(go env GOHOSTARCH)/bin
echo GOBIN is now $GOBIN

echo Building the build tool...
GO111MODULE=on go get ./util/src/harvey/cmd/...

echo Fetching u-root and building it...
GO111MODULE=on go get github.com/u-root/u-root@c370a343c8b0b01faac358c1dafb409e5576ae1a
# Download u-root sources into $GOPATH because that's what u-root expects.
# See https://github.com/u-root/u-root/issues/805
# and https://github.com/u-root/u-root/issues/583
GO111MODULE=off go get -d github.com/u-root/u-root

echo Fetch harvey-os.org commands and build them into $GOBIN
GO111MODULE=on go get harvey-os.org/cmd/...@8978eaed48985e0d89d36e383ece0a6a382eb6b8

echo FIXME -- once we get more architectures, this needs to be done in sys/src/cmds/build.json
echo Build tmpfs command into amd64 plan 9 bin
GO111MODULE=on GOOS=plan9 GOARCH=amd64 go build -o plan9_amd64/bin/tmpfs harvey-os.org/cmd/tmpfs
GO111MODULE=on GOOS=plan9 GOARCH=amd64 go build -o plan9_amd64/bin/decompress harvey-os.org/cmd/decompress

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF

# We support RISC-V, but the default is x86_64 (which we call amd64 for historical reasons):
export ARCH=amd64
# You also need to export your C compiler flavor (gcc, clang, gcc-7...)
export CC=gcc
# And build:
$GOBIN/build
# See \`build -h' for more information on the build tool.

To enable access to files, create a harvey and none user:
sudo useradd harvey
sudo useradd none

none is only required for drawterm/cpu access

Also:
export HARVEY=$(pwd)
add $GOBIN to your path
EOF
