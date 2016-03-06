#!/bin/bash

echo Downloading the build tool...

curl -L http://sevki.co/get-build -o util/build
chmod +x util/build

# this will make booting a VM easier
mkdir -p tmp

cat <<EOF

And build:
./util/build //.:kernel

See \`build -h' for more information on the build tool.
EOF
