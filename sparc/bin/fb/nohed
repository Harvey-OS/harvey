#!/bin/rc
# nohed -- delete the header from a picture file
dd -bs `{sed '/^$/q' $1|wc -c} -skip 1 >[2] /dev/null <$1
