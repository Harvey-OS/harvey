#!/bin/sh
# Fetches the Go source and compiles it for Plan 9
# Run this from the top level of the Harvey tree.
git clone --depth 1 --branch go1.16.6 https://github.com/golang/go.git
cd go/src
GOOS=plan9 GOARCH=amd64 ./make.bash
