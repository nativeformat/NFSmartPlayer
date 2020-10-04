#!/bin/bash


#mainlib=$1
#shift
while [[ $# > 0 ]] ; do
    dep=$1
    if [ "$dep" -nt "$mainlib" ]; then
#        echo "'$dep' is newer than '$mainlib'"
#        rm -f $mainlib
        find $TEMP_DIR -name "*-master.o" -print -delete
        exit 0
    fi
    shift
done
