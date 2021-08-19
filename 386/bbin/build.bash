#!/bin/bash

echo "go version 1.17, makebb from github.com/u-root/gobusybox/src/cmds/makebb, latest u-root"
GOOS=plan9 GOARCH=386 makebb -o bb \
~/go/src/github.com/u-root/u-root/cmds/*/basename \
~/go/src/github.com/u-root/u-root/cmds/*/cat \
~/go/src/github.com/u-root/u-root/cmds/*/chmod \
~/go/src/github.com/u-root/u-root/cmds/*/cmp \
~/go/src/github.com/u-root/u-root/cmds/*/console \
~/go/src/github.com/u-root/u-root/cmds/*/cp \
~/go/src/github.com/u-root/u-root/cmds/*/cpio \
~/go/src/github.com/u-root/u-root/cmds/*/date \
~/go/src/github.com/u-root/u-root/cmds/*/dd \
~/go/src/github.com/u-root/u-root/cmds/*/dirname \
~/go/src/github.com/u-root/u-root/cmds/*/echo \
~/go/src/github.com/u-root/u-root/cmds/*/false \
~/go/src/github.com/u-root/u-root/cmds/*/find \
~/go/src/github.com/u-root/u-root/cmds/*/gpgv \
~/go/src/github.com/u-root/u-root/cmds/*/gpt \
~/go/src/github.com/u-root/u-root/cmds/*/grep \
~/go/src/github.com/u-root/u-root/cmds/*/gzip \
~/go/src/github.com/u-root/u-root/cmds/*/hexdump \
~/go/src/github.com/u-root/u-root/cmds/*/hostname \
~/go/src/github.com/u-root/u-root/cmds/*/init \
~/go/src/github.com/u-root/u-root/cmds/*/io \
~/go/src/github.com/u-root/u-root/cmds/*/ls \
~/go/src/github.com/u-root/u-root/cmds/*/mkdir \
~/go/src/github.com/u-root/u-root/cmds/*/mv \
~/go/src/github.com/u-root/u-root/cmds/*/ping \
~/go/src/github.com/u-root/u-root/cmds/*/printenv \
~/go/src/github.com/u-root/u-root/cmds/*/pwd \
~/go/src/github.com/u-root/u-root/cmds/*/rm \
~/go/src/github.com/u-root/u-root/cmds/*/rush \
~/go/src/github.com/u-root/u-root/cmds/*/seq \
~/go/src/github.com/u-root/u-root/cmds/*/sleep \
~/go/src/github.com/u-root/u-root/cmds/*/sshd \
~/go/src/github.com/u-root/u-root/cmds/*/tail \
~/go/src/github.com/u-root/u-root/cmds/*/tee \
~/go/src/github.com/u-root/u-root/cmds/*/truncate \
~/go/src/github.com/u-root/u-root/cmds/*/wc \
~/go/src/github.com/u-root/u-root/cmds/*/wget \
~/go/src/github.com/u-root/u-root/cmds/*/which \
~/go/src/github.com/rjkroege/edwood 
