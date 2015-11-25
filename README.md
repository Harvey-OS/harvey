[![Build Status](https://travis-ci.org/Harvey-OS/harvey.svg?branch=master)](https://travis-ci.org/Harvey-OS/harvey)
[![Coverity Badge](https://scan.coverity.com/projects/5328/badge.svg)](https://scan.coverity.com/projects/5328)

This is Plan 9 for amd64 built with gcc (and soon, I hope, clang).

This file is a quick list of instructions to get you started quickly.


Prerequisites
=============

To build harvey and play with it, you need to have git, golang, qemu, gcc,
binutils and bison installed. On a Debian, Ubuntu or other .deb system,
you should be able to get going with

	sudo aptitude install git golang build-essential bison qemu-system

If you are on a Mac, you should install macports (https://www.macports.org/) and do

	port install x86_64-elf-gcc
	port install x86_64-elf-binutils
	port install qemu
	export TOOLPREFIX=x86_64-elf-

or if you use homebrew (https://http://brew.sh/)

 	brew tap sevki/gcc_cross_compilers
	brew install sevki/gcc_cross_compilers/x86_64-elf-gcc
	brew install qemu
	export TOOLPREFIX=x86_64-elf-

GERRIT
======

We use gerrithub.io for code-review. If you want to submit changes, go to

	https://review.gerrithub.io/#/admin/projects/Harvey-OS/harvey

and check out the repository from gerrithub rather than github. The clone
command will probably look something like this:

	git clone https://USERNAME@review.gerrithub.io/Harvey-OS/harvey

You'll need to bootstrap everything the first time:

	./bootstrap.sh

This will set up the git repo for code review and build the `build` tool.
You should now be able to build everything by running

	export ARCH=amd64
	export PATH="$(pwd)/util:$PATH"
	build all

Once building is complete, you can try booting the kernel with qemu

	(cd sys/src/9/k10 && sh ../../../../util/QRUN)

Next you should find a bug somewhere in harvey and fix it. In general, the
util/build tool "just works" in any subdirectory, so you can also build just
the stuff you are looking at, too, eg.

	cd sys/src/cmd/aux
	build aux.json

Let's say you found a bug and the files you needed to change were
sys/src/9/ip/tcp.c and sys/src/9/ip/ipaux.c. To submit this for review, you do

	git add sys/src/9/ip/tcp.c
	git add sys/src/9/ip/ipaux.c
	git diff --staged # to check that the patch still makes sense
	git commit -m 'your description of the patch'
	git push

Note the lack of qualifiers in the last push command. It is important,
because it needs to be pushed to "origin HEAD:refs/for/master" for review
(and not to master). This will generate a code-review change request, others
will review it, and if it looks good we will merge it to the mainline repo
using gerrithub.io.

If your patch needs further work (you notice something wrong with it yourself,
or someone suggests changes), you can just edit the affected files and then
amend the change list as follows

	git add sys/src/9/ip/tcp.c
	git commit --amend
	git push

More information on using Gerrit can be found on the gerrithub.io website.


Getting go9p to serve your files
================================

The currently recommended way of doing this is to run go9p/ufs as the
file server for harvey. It is provided as a submodule in util/third_party
and should be automatically compiled & copied to util/ when you run
'BUILD utils'.

Once it's built, you can run this:

	(export HARVEY=$(pwd) && cd sys/src/9/k10 && sh ../../../../util/GO9PRUN)

to boot with ufs serving the harvey directory for your harvey instance. Once
harvey is up, you can telnet onto it with

	util/telnet localhost:5555

Where 5555 is forwarded to the harvey instance. This gives you a prompt
without any security. Once you have the prompt, you can mount the harvey
directory as your root like this (10.0.2.2 is what qemu has as the host)

	srv tcp!10.0.2.2!5640 k
	mount -a /srv/k /
