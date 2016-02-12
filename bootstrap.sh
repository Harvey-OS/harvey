#!/bin/bash
if [ -n "$(git remote -v | awk '$1=="origin"{print $2}' | grep gerrithub)" ]; then
	echo Setting up git...
	curl -Lso .git/hooks/commit-msg http://review.gerrithub.io/tools/hooks/commit-msg
	chmod u+x .git/hooks/commit-msg
	# this is apparently the preferred method and might even work over HTTP
	# https://gerrit-review.googlesource.com/Documentation/cmd-receive-pack.html
	git config remote.origin.push HEAD:refs/for/master%r=rminnich@gmail.com,r=crossd@gmail.com,r=elbingmiss@gmail.com,r=anyrhine@gmail.com,r=0intro@gmail.com,r=hdonnay@gmail.com,r=john@jfloren.net,r=rafael.fernandez@taisis.com,r=keith.poole@gmail.com,r=s@sevki.org
fi
git submodule init
git submodule update
echo Building the build tool...
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -u sevki.org/build/cmd/build
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -d harvey/cmd/... # should really vendor these bits
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go install github.com/rminnich/ninep harvey/cmd/...

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF

Now just set your environment variables and build:

	source envsetup
	build

See \`build -h' for more information on the build tool.
EOF
