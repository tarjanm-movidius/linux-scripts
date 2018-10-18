#!/bin/bash

[ $# -lt 2 ] && echo "Usage: $0 <old_git> <new_git>" 1>&2 && exit 1
FN1=`find . -name "*git${1}*.txt"`
FN2=`find . -name "*git${2}*.txt"`
[ -e "$FN1" -a -e "$FN2" ] || exit 2

diff <(cut -c 39- $FN1) \
     <(cut -c 39- $FN2)
