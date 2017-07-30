#!/bin/bash
../neat/neatcc/ncc -I../neat/neatlibc test/a.c -o test/a.o
../neat/ld/nld -f -o a.bin -L ../neat/neatlibc -lc test/a.o
