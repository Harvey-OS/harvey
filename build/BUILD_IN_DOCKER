#!/bin/bash

# We can build with docker, using the a local repo by binding the local
# directory to $HARVEY_LOCAL.  This will then be copied to $HARVEY.
# If $HARVEY_LOCAL doesn't exist, we'll clone the harvey repo to $HARVEY.
HARVEY_LOCAL=/usr/local/harvey_local
if [ -d $HARVEY_LOCAL ]
then
        echo "Copying $HARVEY_LOCAL -> $HARVEY"
        mkdir -p $HARVEY
        time cp -R $HARVEY_LOCAL/* $HARVEY/
else
        # Clone harvey (shallow for a little speed)
        echo "Cloning https://github.com/$HARVEY_OWNER/$HARVEY_REPO.git branch $HARVEY_BRANCH -> $HARVEY"
        HARVEY_OWNER=Harvey-OS
        HARVEY_REPO=harvey
        HARVEY_BRANCH=main
        git clone --depth 1 --single-branch --branch $HARVEY_BRANCH https://github.com/$HARVEY_OWNER/$HARVEY_REPO.git $HARVEY
fi

########################################################################
# Copy/build toolchains for linux host
# TODO true->size is a hack to make size not fail when building - best to remove this sometime
# TODO need to make our bind script handle binds of single files
cp /bin/true $HARVEY_LINUX_BIN/size
cp $NINECC/src/cmd/iar/o.out $HARVEY_LINUX_BIN/ar
cp $PLAN9/bin/{sed,yacc,lex,awk,date,dd,rm,basename,ls} $HARVEY_LINUX_BIN
cp $HARVEY/build/bind /bin

# TODO Merge cmd/BUILD and BUILDTOOLCHAIN?
(cd $HARVEY/sys/src && bash BUILDTOOLCHAIN)
# cpp was built by BUILDTOOLCHAIN - we need to copy to /bin because pcc execs /bin/cpp
cp $HARVEY_LINUX_BIN/cpp /bin
# build pcc
(cd $HARVEY/sys/src/cmd && bash BUILD)

# Modify some mkfiles to handle binaries in subfolders
# TODO not sure if this is great, since it'll alter the mkfiles in the image - maybe undo after?
sed -i 's/aux\/mklatinkbd/mklatinkbd/' $HARVEY/sys/src/9/port/portmkfile $HARVEY/sys/src/9k/mk/portmkfile
sed -i 's/aux\/data2s/data2s/' $HARVEY/sys/src/9/port/{mkroot,mkrootall} $HARVEY/sys/src/9k/mk/{mkroot,mkrootall}

export USER=root
export PATH="${HARVEY_LINUX_BIN}:${PATH}:/rc/bin:."
export objtype=amd64
export cputype=unix
export FORCERCFORMK=1
export CGO_ENABLED=0

(cd $HARVEY/sys/src && bash RUN_BUILD_IN_PRIVATE_NAMESPACE)
#(cd $HARVEY/sys/src && bash)
