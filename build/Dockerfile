# docker build -t harvey:Dockerfile .
# docker run --rm -it --cap-add SYS_ADMIN --privileged harvey:Dockerfile

# TODO probably need to pass in aarch64 as an env var
# TODO mahjonng doesn't build becuase of problems with cpp - fix cpp!

FROM ubuntu:21.10

RUN apt-get -y update
RUN apt-get -y upgrade
RUN apt-get install -y build-essential git sudo

# Only use layer cache if the git repo hasn't changed.  This ensures we
# always have the latest code, but make use of the cache where possible.
ARG OWNER=9fans
ARG REPO=plan9port
ARG BRANCH=master
ADD https://api.github.com/repos/$OWNER/$REPO/git/refs/heads/$BRANCH version.json
RUN cd /usr/local \
    && git clone -b $BRANCH https://github.com/$OWNER/$REPO.git \
    && cd plan9port \
    && ./INSTALL

ENV PLAN9=/usr/local/plan9port
ENV PATH="${PATH}:${PLAN9}/bin"

# Fetch latest harvey (using cache if possible)
# TODO Populate these args from CI
ARG OWNER=gmacd
ARG REPO=harvey
ARG BRANCH=build_on_unix_in_docker
ADD https://api.github.com/repos/$OWNER/$REPO/git/refs/heads/$BRANCH version.json
RUN cd /usr/local \
    && git clone -b $BRANCH https://github.com/$OWNER/$REPO.git

# Build toolchains
WORKDIR /usr/local/harvey/sys/src

# true->size is a hack to make size not fail when building - best to remove this sometime

RUN if [ `uname -p` = "aarch64" ]; then \
    sed -i 's/linux_amd64/linux_arm64/' BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/6a/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/6c/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/6l/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/8a/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/8c/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/8l/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/cpp/BUILD; \
    sed -i 's/linux_amd64/linux_arm64/' cmd/aux/BUILD; \
    mkdir -p /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/sed /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/yacc /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/lex /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/awk /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/date /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/dd /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/rm /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/basename /usr/local/harvey/linux_arm64/bin; \
    cp ${PLAN9}/bin/ls /usr/local/harvey/linux_arm64/bin; \
    cp /bin/true /usr/local/harvey/linux_arm64/bin/size; \
  else \
    mkdir -p /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/sed /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/yacc /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/lex /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/awk /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/date /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/dd /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/rm /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/basename /usr/local/harvey/linux_amd64/bin; \
    cp ${PLAN9}/bin/ls /usr/local/harvey/linux_amd64/bin; \
    cp /bin/true /usr/local/harvey/linux_amd64/bin/size; \
  fi

# To make rc scripts run
RUN cp ${PLAN9}/bin/rc /bin/rc

RUN bash BUILDTOOLCHAIN
WORKDIR /usr/local/harvey/sys/src/cmd
RUN bash BUILD

# Build ar from 9-cc
ARG OWNER=0intro
ARG REPO=9-cc
ARG BRANCH=master
ADD https://api.github.com/repos/$OWNER/$REPO/git/refs/heads/$BRANCH version.json
RUN cd /usr/local \
    && git clone -b $BRANCH https://github.com/$OWNER/$REPO.git
WORKDIR /usr/local/9-cc

COPY 9-cc.patch /usr/local/9-cc/9-cc.patch
RUN git apply /usr/local/9-cc/9-cc.patch

RUN if [ `uname -p` = "aarch64" ]; then \
    mkdir -p Linux/arm/bin; \
    mkdir -p Linux/arm/include; \
    mkdir -p Linux/arm/lib; \
  else \
    mkdir -p Linux/amd64/bin; \
    mkdir -p Linux/amd64/include; \
    mkdir -p Linux/amd64/lib; \
  fi

# Hack up 9-cc to build ar for amd64 (or arm)
RUN ./configure && . ./env

RUN if [ `uname -p` = "aarch64" ]; then \
    cp Linux/amd64/include/lib9.h Linux/arm/include/lib9.h; \
    cp Linux/amd64/include/fpuctl.h Linux/arm/include/fpuctl.h; \
    sed -i 's/arm-gcc/gcc/' mkfiles/mkfile-Linux-arm; \
  fi

RUN mk

RUN if [ `uname -p` = "aarch64" ]; then \
    cp src/cmd/iar/o.out /usr/local/harvey/linux_arm64/bin/ar; \
  else \
    cp src/cmd/iar/o.out /usr/local/harvey/linux_amd64/bin/ar; \
  fi

# Last bits of prep for harvey image to allow building inside the container
WORKDIR /usr/local/harvey

# TODO should really be in the build scripts
RUN mkdir -p amd64/lib
RUN mkdir -p acme/bin/amd64
RUN mkdir -p acme/lib
RUN mkdir -p amd64/bin/auth
RUN mkdir -p amd64/bin/aux
RUN mkdir -p amd64/bin/dial
RUN mkdir -p amd64/bin/disk
RUN mkdir -p amd64/bin/fossil
RUN mkdir -p amd64/bin/fs
RUN mkdir -p amd64/bin/games
RUN mkdir -p amd64/bin/ip
RUN mkdir -p amd64/bin/ip/httpd
RUN mkdir -p amd64/bin/ndb
RUN mkdir -p amd64/bin/replica
RUN mkdir -p amd64/bin/upas/
RUN mkdir -p amd64/bin/usb
RUN mkdir -p amd64/bin/venti
RUN mkdir -p lib/audio
RUN mkdir -p mail/lib
RUN mkdir -p rc/bin

# Modify some mkfiles to handle binaries in subfolders
# TODO not sure if this is great, since it'll alter the mkfiles in the image
RUN sed -i 's/aux\/mklatinkbd/mklatinkbd/' sys/src/9/port/portmkfile sys/src/9k/mk/portmkfile
RUN sed -i 's/aux\/data2s/data2s/' sys/src/9/port/mkroot sys/src/9/port/mkrootall sys/src/9k/mk/mkroot sys/src/9k/mk/mkrootall

WORKDIR /usr/local/harvey/sys/src

# Is this correct?
ENV USER=root
ENV HARVEY=/usr/local/harvey

ENV PATH="${PATH}:/rc/bin"

ENTRYPOINT ["bash", "RUN_BUILD_IN_PRIVATE_NAMESPACE"]
