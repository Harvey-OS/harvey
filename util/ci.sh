#!/bin/sh

set -e

: ${CC?"Need to set CC as clang, gcc or a specific version like clang-3.6"}
: ${ARCH?"Need to set ARCH as aarch64, amd64 or riscv"}

# make sure Go is gone
sudo rm -rf /opt/hostedtoolcache/go/
# fetch official Go toolchain release as per https://golang.org/doc/install
TARFILE="go$GO_VERSION.$OS-$ARCH.tar.gz"
wget "https://golang.org/dl/$TARFILE" -nv # too verbose otherwise
sudo tar -C /usr/local -xzf "$TARFILE"
# For some reason, GOROOT was set in GitHub's CI, so we enforce it
export GOROOT=/usr/local/go/
export PATH=/usr/local/go/bin:$PATH

# please keep me for debugging CI ;)
which go
go env
pwd # in case you get lost

# bootstrap Harvey build utilities - what you do on your host OS otherwise
./bootstrap.sh

GO_MOD_PATH_UROOT=`grep github.com/u-root/u-root go.mod | cut -d " " -f 2`
GO_MOD_PATH_HARVEY=`grep harvey-os.org go.mod | cut -d " " -f 2`
echo u-root version: $GO_MOD_PATH_UROOT
echo Harvey version: $GO_MOD_PATH_HARVEY

# TODO Is all of this still necessary?
# not that we really want this, but yea... Go modules
mkdir -p ~/go/src/github.com/u-root/
cp -r ~/go/pkg/mod/github.com/u-root/u-root@$GO_MOD_PATH_UROOT ~/go/src/github.com/u-root/u-root
chmod +w ~/go/src/github.com/u-root/u-root -R
#(cd ~/go/src/github.com/u-root/u-root && dep ensure)

cp -r ~/go/pkg/mod/harvey-os.org@$GO_MOD_PATH_HARVEY ~/go/src/harvey-os.org
chmod +w ~/go/src/harvey-os.org -R

# extend PATH to include the build tool and u-root
PATH=$PATH:$(pwd)/$(go env GOHOSTOS)_$(go env GOHOSTARCH)/bin:~/go/bin

# finally build Harvey :-)
build build.json
