#!/bin/sh

IFS='
'

for i in `find . -name "*.mp3"`; do
	NAME="`basename $i | sed 's/.mp3//g' | sed 's/.* - //g'`"
	id3tag -1 -s"$NAME" "$i"
done
