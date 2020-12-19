#!/bin/sh

set -e

HOSTBIN=$(go env GOHOSTOS)_$(go env GOHOSTARCH)/bin
export GOBIN=$(pwd)/$HOSTBIN
echo GOBIN is now $GOBIN

echo Building the build tool into $HOSTBIN
GO111MODULE=on go get ./util/src/harvey/cmd/...

echo Building u-root into $HOSTBIN
GO111MODULE=on go get github.com/u-root/u-root@c370a343c8b0b01faac358c1dafb409e5576ae1a
# Download u-root sources into $GOPATH because that's what u-root expects.
# See https://github.com/u-root/u-root/issues/805
# and https://github.com/u-root/u-root/issues/583
GO111MODULE=off go get -d github.com/u-root/u-root

echo Building harvey-os.org commands into $HOSTBIN
GO111MODULE=on go get harvey-os.org/cmd/...@fdc9a5f1c6ed2f37b77c16e82b80b1ed6df8fa17

echo FIXME -- once we get more architectures, this needs to be done in sys/src/cmds/build.json
echo Building tmpfs command into plan9_amd64/bin
GO111MODULE=on GOOS=plan9 GOARCH=amd64 go build -o plan9_amd64/bin/tmpfs harvey-os.org/cmd/tmpfs

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
EOF
