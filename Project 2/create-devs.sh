#!/bin/bash

CH=(a b c d)
for i in 0 1 2 3
do
        rm -f /dev/osprd${CH[$i]}
        mknod /dev/osprd${CH[$i]} b 222 $i || exit
        chmod 666 /dev/osprd${CH[$i]}
done