#!/bin/sh

set -e

: ${CC?"Need to set CC as clang, gcc or a specific version like clang-3.6"}
: ${ARCH?"Need to set ARCH as aarch64, amd64 or riscv"}

./bootstrap.sh
ls -l */bin util

go env 


echo $GOPATH
echo $GOROOT
go env GOPATH

PATH=$PATH:$(pwd)/$(go env GOHOSTOS)_$(go env GOHOSTARCH)/bin:$(pwd)/util
echo $PATH
which build
build build.json
