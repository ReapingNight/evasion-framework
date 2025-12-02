#!/bin/bash

objpath=$1
tagspath=$2
filename=$(basename $1)

./funcsigs64.pl $objpath $tagspath
cd inject/
make elf64
aarch64-linux-gnu-ld -r inject.o ../$objpath -o my-$filename

echo "Finish"
