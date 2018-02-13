#!/bin/sh
: ${CC?"Need to set CC as clang, gcc or a specific version like clang-3.6"}
: ${ARCH?"Need to set ARCH as aarch64, amd64 or riscv"}

GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get harvey/cmd/...
./util/build build.json
