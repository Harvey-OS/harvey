#!/bin/bash

[ x"$HARVEY" != x ] && cd $HARVEY/web
mkdocs serve && exit 0 
echo "#### we need mkdocs: use pip install mkdocs"


