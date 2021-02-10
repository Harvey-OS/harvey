#!/bin/sh

set -e

HOSTBIN=$(go env GOHOSTOS)_$(go env GOHOSTARCH)/bin
export GOBIN=$(pwd)/$HOSTBIN
echo GOBIN is now $GOBIN

echo Building harvey-os.org commands into $HOSTBIN
GO111MODULE=off go get -d -u harvey-os.org/cmd/...
GO111MODULE=off go install harvey-os.org/cmd/...

echo Building u-root into $HOSTBIN
# Download u-root sources into $GOPATH because that's what u-root expects.
# See https://github.com/u-root/u-root/issues/805
# and https://github.com/u-root/u-root/issues/583
GO111MODULE=off go get -d -u github.com/u-root/u-root
GO111MODULE=off go install github.com/u-root/u-root

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF

To build for x86_64 (CC=clang is also supported):
  export HARVEY=$(pwd)
  PATH=\$PATH:\$HARVEY/$HOSTBIN CC=gcc ARCH=amd64 build

To enable access to files, create a harvey and none user (none is only required for drawterm/cpu access):
  sudo useradd harvey
  sudo useradd none

To run in qemu in cpu mode:
  ./util/GO9PCPU
or to run in terminal mode:
  ./util/GO9PTERM

To netboot, follow the instructions here: https://github.com/Harvey-OS/harvey/wiki/Booting-Harvey-on-real-hardware-I-(TFTP)

You may want to run the following to get APEX installed:

	OS=linux ARCH=amd64 CC=gcc sh getapex.sh
EOF
