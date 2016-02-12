#!/bin/bash
echo "compiling $1 in $2 mode"
cmode=none
if [[ $2 == debug ]]
then
	cmode=ggdb
elif [[ $2 == release ]]
then
	cmode=O2
else
	cmode=ggdb
fi


if [[ $1 == *.c ]]
then
    gcc -$cmode `pkg-config --cflags opencv` -o `basename $1 .c` $1 `pkg-config --libs opencv` -lboost_filesystem -lboost_system;
elif [[ $1 == *.cpp ]]
then
    g++ -$cmode `pkg-config --cflags opencv` -o `basename $1 .cpp` $1 `pkg-config --libs opencv` -lboost_filesystem -lboost_system;
else
    echo "Please compile only .c or .cpp files"
fi
echo "Output file => ${1%.*}"
