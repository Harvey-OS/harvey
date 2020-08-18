#!/bin/sh

set -e

: ${CC?"Need to set CC as clang, gcc or a specific version like clang-3.6"}
: ${ARCH?"Need to set ARCH as aarch64, amd64 or riscv"}

GO111MODULE=on GOBIN="$(pwd)/util" go get ./util/src/harvey/cmd/...
PATH=$PATH:$(pwd)/$(go env GOHOSTOS)_$(go env GOHOSTARCH)/bin:$(pwd)/util
echo $PATH
which build
build build.json
