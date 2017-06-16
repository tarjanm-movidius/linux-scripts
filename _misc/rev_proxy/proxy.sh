#!/bin/sh

[ -e f1 ] || mkfifo f1
[ -e f2 ] || mkfifo f2
#(while true; do stdbuf -i 0 -o 0 cat < f1 | nc -lp 59001 > f2; done) &
#(while true; do stdbuf -i 0 -o 0 cat < f2 | nc -lp 59002 > f1; done) &
(while true; do cat f1 | nc -lp 59001 > f2; done) &
(while true; do cat f2 | nc -lp 59002 > f1; done) &
