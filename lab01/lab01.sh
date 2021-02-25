#!/bin/zsh
rm -rf group9091
mkdir -p group9091/fedotov && cd group9091/fedotov
date > dmitriy.txt
date --date="next Mon" > filedate.txt
cat dmitriy.txt filedate.txt > result.txt
cat result.txt
