#!/bin/bash
export > exports.$$
# what a mess.
export > buildnamespace
sudo unshare -m bash BUILD_SETUP_NAMESPACE $USER exports.$$
