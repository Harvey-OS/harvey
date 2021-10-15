FROM ubuntu:latest

RUN apt-get -y update
RUN apt-get -y upgrade
RUN apt-get install -y build-essential git

# Only use layer cache if the git repo hasn't changed.  This ensures we
# always have the latest code, but make use of the cache where possible.
ARG USER=9fans
ARG REPO=plan9port
ARG BRANCH=master
ADD https://api.github.com/repos/$USER/$REPO/git/refs/heads/$BRANCH version.json
RUN mkdir -p /usr/local/plan9port \
    && cd /usr/local \
    && git clone -b $BRANCH https://github.com/$USER/$REPO.git \
    && cd plan9port \
    && ./INSTALL

ENV PLAN9=/usr/local/plan9port
ENV PATH="${PATH}:${PLAN9}/bin"

# Fetch latest harvey (using cache if possible)
# TODO Populate these args from CI
ARG USER=Harvey-OS
ARG REPO=harvey
ARG BRANCH=main
ADD https://api.github.com/repos/$USER/$REPO/git/refs/heads/$BRANCH version.json
RUN mkdir -p /usr/local/harvey \
    && cd /usr/local \
    && git clone -b $BRANCH https://github.com/$USER/$REPO.git

# Build 6 and 8 toolchains
WORKDIR /usr/local/harvey/sys/src/cmd
RUN cd 6a && bash BUILD
RUN cd 6c && bash BUILD
RUN cd 6l && bash BUILD
RUN cd 8a && bash BUILD
RUN cd 8c && bash BUILD
RUN cd 8l && bash BUILD

# TODO Build harvey
#ENV FORCERCFORMK=1
#WORKDIR /usr/local/harvey/build/scripts
#RUN PATH="${PATH}:${PWD}" rc build
