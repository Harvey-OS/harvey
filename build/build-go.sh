#!/bin/sh
# Fetches the Go source and compiles it for Plan 9
cd ../sys/
if [ ! -e go ]
then
	git clone --depth 1 --branch go1.16.6 https://github.com/golang/go.git
fi
cd go/src
GOROOT_FINAL=/sys/go GOOS=plan9 GOARCH=386 ./make.bash
GOROOT_FINAL=/sys/go GOOS=plan9 GOARCH=amd64 ./make.bash
rm -rf ../bin/linux_*
rm -rf ../pkg/linux_*
rm -rf ../pkg/tool/linux_*
