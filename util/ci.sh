#!/bin/sh
: ${CC?"Need to set CC as clang, gcc or a specific version like clang-3.6"}
: ${ARCH?"Need to set ARCH as aarch64, amd64 or riscv"}

GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get harvey/cmd/...
./util/build build.json

RESULTS=usr/harvey/tmp/runallresults.txt
rm -f $RESULTS

./util/GO9PCPUREGRESS

if [ ! -f $RESULTS ]; then
	echo Regression tests failed
	exit 1
fi

cat $RESULTS

if tail -1 ./usr/harvey/tmp/runallresults.txt | grep -vq DONE; then
	echo Regression tests failed
	exit 1
fi

echo Regression tests passed
