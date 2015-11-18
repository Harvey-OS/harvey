#!/bin/bash
echo Setting up git...
curl -Lso .git/hooks/commit-msg http://review.gerrithub.io/tools/hooks/commit-msg
chmod u+x .git/hooks/commit-msg
git config remote.origin.push HEAD:refs/for/master
git submodule init
git submodule update
git config remote.origin.receivepack "git receive-pack --reviewer rminnich --reviewer cross --reviewer elbing --reviewer anyrhine --reviewer 0intro --reviewer giacomo --reviewer john@jfloren.net --reviewer rafael.fernandez"
echo Building the build tool...
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go get -d harvey/cmd/... # should really vendor these bits
GOBIN="$(pwd)/util" GOPATH="$(pwd)/util/third_party:$(pwd)/util" go install github.com/rminnich/go9p/ufs harvey/cmd/...
cat <<EOF

Now just set ARCH, modify PATH, and build:

	export ARCH=amd64
	export PATH="$(pwd)/util:\$PATH"
	build all

See \`build -h' for more information on the build tool.
EOF
