#!/bin/bash

cat $1 | sed -e "s/\t/:/g"  | cut -d ':' -f2 | datamash mean 1
cat $1 | sed -e "s/\t/:/g"  | cut -d ':' -f3 | datamash mean 1
cat $1 | sed -e "s/\t/:/g"  | cut -d ':' -f4 | datamash mean 1
