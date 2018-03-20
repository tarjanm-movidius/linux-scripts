#!/bin/sh

IFS='
'

for i in `find . -name "*.mp3"`; do
	if ! id3info "$i" | grep -q 'TIT\|TT2'; then
		NAME="`basename $i | sed 's/.mp3//g' | sed 's/.* - //g'`"
		id3tag -1 -s"$NAME" "$i"
	fi
done
