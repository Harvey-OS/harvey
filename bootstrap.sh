#!/bin/sh
set -e
HARVEY="$(git rev-parse --show-toplevel)"
hook="${HARVEY}/.git/hooks/commit-msg"
hookURL='http://review.gerrithub.io/tools/hooks/commit-msg'
{ curl -sL "$hookURL" || wget -qO- "$hookURL"; } > "$hook"
chmod u+x "$hook"
git config remote.origin.push HEAD:refs/for/master
git submodule init
git submodule update
echo git configured

if ! which go >/dev/null 2>&1; then
	printf "go needs to be installed for the build tools\n" >&2
	exit 99
fi
(
	export GOBIN=${HARVEY}/util
	export GOPATH="${HARVEY}/util/third_party:${HARVEY}/util"
	go get -d all
	go install github.com/rminnich/go9p/ufs harvey/cmd/...
)
echo build tool built: util/build
