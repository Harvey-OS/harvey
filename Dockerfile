FROM alpine:3.5

RUN apk update; apk add git git-perl go gcc bison musl-dev qemu qemu-system-x86_64 curl bash
ENV HARVEY=/harvey ARCH=amd64 CC=gcc
RUN adduser -S harvey && adduser -S none

ADD . /harvey
WORKDIR /harvey

RUN ./bootstrap.sh && /harvey/util/build

ENTRYPOINT ["/harvey/util/GO9PCPUDOCKER"]
