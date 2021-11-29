# First build the container:
# docker build -t harvey:Dockerfile .
# (or:
#   docker build --progress=plain -t harvey:Dockerfile .
#  if you want to see the output from commands (useful for debugging build))
#
# Now run container to build.
# If you want to build from the repo (i.e. for CI), run:
#   docker run --rm -it --cap-add SYS_ADMIN --privileged harvey:Dockerfile
# If you want to build from a local folder (e.g. the current directoy), run:
#   docker run --rm -it --mount type=bind,source="$(pwd)"/..,target=/usr/local/harvey_local,readonly --cap-add SYS_ADMIN --privileged harvey:Dockerfile

# TODO mahjonng doesn't build becuase of problems with cpp - fix cpp!

FROM ubuntu:21.10

RUN apt-get -y update
RUN apt-get -y upgrade
RUN apt-get install -y build-essential git sudo

ENV HARVEY=/usr/local/harvey
ENV HARVEY_LINUX_BIN=/usr/local/harvey_linux/bin
RUN mkdir -p $HARVEY_LINUX_BIN


########################################################################
# Only use layer cache if the git repo hasn't changed.  This ensures we
# always have the latest code, but make use of the cache where possible.
ARG NINEFANS_OWNER=9fans
ARG NINEFANS_REPO=plan9port
ARG NINEFANS_BRANCH=master
ADD https://api.github.com/repos/$NINEFANS_OWNER/$NINEFANS_REPO/git/refs/heads/$NINEFANS_BRANCH version.json
RUN cd /usr/local \
    && git clone -b $NINEFANS_BRANCH https://github.com/$NINEFANS_OWNER/$NINEFANS_REPO.git \
    && cd plan9port \
    && ./INSTALL

ENV PLAN9=/usr/local/plan9port
ENV PATH="${PATH}:${PLAN9}/bin"

# To make rc scripts run
RUN cp ${PLAN9}/bin/rc /bin/rc


########################################################################
# Build ar from 9-cc
# Do this early since it's less likely to change than the harvey repo,
# allowing us to use the cached layers.
ARG NINECC_OWNER=0intro
ARG NINECC_REPO=9-cc
ARG NINECC_BRANCH=master
ADD https://api.github.com/repos/$NINECC_OWNER/$NINECC_REPO/git/refs/heads/$NINECC_BRANCH version.json
RUN cd /usr/local \
    && git clone -b $NINECC_BRANCH https://github.com/$NINECC_OWNER/$NINECC_REPO.git
ENV NINECC=/usr/local/9-cc
WORKDIR $NINECC

COPY 9-cc.patch $NINECC/9-cc.patch
RUN git apply $NINECC/9-cc.patch

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
RUN cp ${NINECC}/src/cmd/iar/o.out $HARVEY_LINUX_BIN/ar

#TEMP!
COPY BUILD_IN_DOCKER /usr/local/

ENTRYPOINT ["bash", "/usr/local/BUILD_IN_DOCKER"]
