FROM ubuntu:latest

RUN apt-get -y update
RUN apt-get -y upgrade
RUN apt-get install -y build-essential git

RUN mkdir -p /usr/local/plan9port \
    && cd /usr/local \
    && git clone https://github.com/9fans/plan9port.git \
    && cd plan9port \
    && ./INSTALL

ENV PLAN9=/usr/local/plan9port
ENV PATH="${PATH}:${PLAN9}/bin"

RUN mkdir -p /usr/local/harvey \
    && cd /usr/local \
    && git clone https://github.com/Harvey-OS/harvey.git

RUN cd /usr/local/harvey/sys/src/cmd/6a && bash BUILD
RUN cd /usr/local/harvey/sys/src/cmd/6c && bash BUILD
RUN cd /usr/local/harvey/sys/src/cmd/6l && bash BUILD
RUN cd /usr/local/harvey/sys/src/cmd/8a && bash BUILD
RUN cd /usr/local/harvey/sys/src/cmd/8c && bash BUILD
RUN cd /usr/local/harvey/sys/src/cmd/8l && bash BUILD
