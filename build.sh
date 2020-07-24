#!/bin/sh -e
cd extracted
${CC-cc} -I. *.c -o xzdec
echo Done.
