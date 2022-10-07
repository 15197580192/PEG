#!/bin/bash
db=$1
query=$2

./khop $2 > queryfile

cd ..
./build/PEG_Query $db ./script/queryfile
